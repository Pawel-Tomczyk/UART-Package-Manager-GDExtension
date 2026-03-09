#include "uartManager.hpp"
#include <cstdint>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

UartManager::UartManager() {
    hSerial = INVALID_SERIAL_HANDLE;
    keepRunningFlag = false;
}

UartManager::~UartManager() {
    close_port();
}

void UartManager::_bind_methods() {
    // Rejestracja funkcji dla GDScript
    godot::ClassDB::bind_method(godot::D_METHOD("open_port", "portName", "baudrate"), &UartManager::openPort);
    godot::ClassDB::bind_method(godot::D_METHOD("close_port"), &UartManager::close_port);
    godot::ClassDB::bind_method(godot::D_METHOD("send_packet", "id", "data"), &UartManager::send_packet);

    // Rejestracja sygnału, który wyślemy do Godota, gdy nadejdzie pełny pakiet
    ADD_SIGNAL(godot::MethodInfo("packet_received", 
        godot::PropertyInfo(godot::Variant::INT, "id"), 
        godot::PropertyInfo(godot::Variant::PACKED_BYTE_ARRAY, "data")));
}


bool UartManager::openPort(godot::String port_name, int baudrate) {
#ifdef _WIN32
    // ==========================================
    // KOD DLA WINDOWSA (ten, który już widziałeś)
    // ==========================================
    godot::String full_port_name = "\\\\.\\" + port_name;
    hSerial = CreateFileA(full_port_name.utf8().get_data(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (hSerial == INVALID_SERIAL_HANDLE) return false;

    DCB dcb = {0};
    dcb.DCBlength = sizeof(dcb);
    GetCommState(hSerial, &dcb);
    dcb.BaudRate = baudrate;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity = NOPARITY;
    SetCommState(hSerial, &dcb);

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    SetCommTimeouts(hSerial, &timeouts);

#else
    // ==========================================
    // KOD DLA LINUXA / macOS (POSIX)
    // ==========================================
    // Linux używa ścieżek typu "/dev/ttyUSB0" lub "/dev/ttyACM0"
    godot::String full_port_name = "/dev/" + port_name;
    
    // Otwarcie portu: Odczyt/Zapis | Nie bądź terminalem sterującym | Nie blokuj
    hSerial = open(full_port_name.utf8().get_data(), O_RDWR | O_NOCTTY | O_NDELAY);
    
    if (hSerial == INVALID_SERIAL_HANDLE) {
        godot::UtilityFunctions::print("Nie udalo sie otworzyc portu (Linux): ", port_name);
        return false;
    }

    struct termios tty;
    if (tcgetattr(hSerial, &tty) != 0) {
        close(hSerial);
        return false;
    }

    // Ustawienie baudrate
    cfsetospeed(&tty, baudrate);
    cfsetispeed(&tty, baudrate);

    // Konfiguracja 8N1 (8 bitów, brak parzystości, 1 bit stopu)
    tty.c_cflag &= ~PARENB; // Brak parzystości
    tty.c_cflag &= ~CSTOPB; // 1 bit stopu
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;     // 8 bitów danych
    
    // Ustawienia lokalne i odczytu
    tty.c_cflag |= CREAD | CLOCAL; 
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Tryb surowy (raw)
    tty.c_oflag &= ~OPOST; // Brak przetwarzania wyjścia

    // Timeouty (odczyt blokuje się maksymalnie na 0.5s)
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 5; 

    tcsetattr(hSerial, TCSANOW, &tty);
#endif

    // ==========================================
    // WSPÓLNY KOD DLA OBU SYSTEMÓW
    // ==========================================
    godot::UtilityFunctions::print("Port otwarty pomyslnie na tym systemie!");
    keepRunningFlag = true;
    workerThread = std::thread(&UartManager::threadLoop, this);

    return true;
}

bool UartManager::close_port() {
    keepRunningFlag = false;
    if (workerThread.joinable()) {
        workerThread.join();
    }

    if (hSerial != INVALID_SERIAL_HANDLE) {
#ifdef _WIN32
        CloseHandle(hSerial);
#else
        close(hSerial);
#endif
        hSerial = INVALID_SERIAL_HANDLE;
    }
    return true;
}

void UartManager::threadLoop() {
    uint8_t read_buf[256];
    int bytes_read = 0;

    // Zmienne stanu dla naszego parsera
    int parse_state = 0; 
    UartPacket current_packet;
    int data_bytes_read = 0;

    while (keepRunningFlag) {
        bytes_read = 0;

        // 1. ODCZYT Z PORTU (Wieloplatformowo)
#ifdef _WIN32
        DWORD dwRead;
        if (ReadFile(hSerial, read_buf, sizeof(read_buf), &dwRead, NULL)) {
            bytes_read = dwRead;
        }
#else
        bytes_read = read(hSerial, read_buf, sizeof(read_buf));
#endif

        // 2. PARSOWANIE (Maszyna stanów)
        if (bytes_read > 0) {
            for (int i = 0; i < bytes_read; i++) {
                uint8_t b = read_buf[i];

                switch (parse_state) {
                    case 0: // Szukamy START_BYTE_1 (0xAB)
                        if (b == START_BYTE_1) parse_state = 1;
                        break;

                    case 1: // Szukamy START_BYTE_2 (0xCD)
                        if (b == START_BYTE_2) parse_state = 2;
                        else if (b == START_BYTE_1) parse_state = 1; // Zabezpieczenie przed dublem
                        else parse_state = 0;
                        break;

                    case 2: // Czytamy ID
                        current_packet.id = b;
                        parse_state = 3;
                        break;

                    case 3: // Czytamy Length
                        current_packet.length = b;
                        data_bytes_read = 0;
                        if (current_packet.length == 0) parse_state = 5; // Skok do checksumy, jeśli brak danych
                        else parse_state = 4;
                        break;

                    case 4: // Czytamy Data
                        current_packet.data[data_bytes_read] = b;
                        data_bytes_read++;
                        if (data_bytes_read >= current_packet.length) {
                            parse_state = 5; // Zebraliśmy wszystkie dane
                        }
                        break;

                    case 5: // Czytamy Checksum i weryfikujemy
                        current_packet.checksum = b;
                        
                        if (calculateChecksum(&current_packet) == current_packet.checksum) {
                            // PAKIET JEST POPRAWNY! Wrzucamy do kolejki
                            std::lock_guard<std::mutex> lock(queueMutex);
                            msgQueue.push(current_packet);
                        } else {
                            godot::UtilityFunctions::print("Blad CRC pakietu!");
                        }
                        parse_state = 0; // Wracamy do szukania nowego pakietu
                        break;
                }
            }
        } else {
            // Zapobiega "zjadaniu" 100% CPU przez wątek, jeśli nie ma danych
            std::this_thread::sleep_for(std::chrono::milliseconds(5)); 
        }
    }
}

bool UartManager::send_packet(int id, godot::PackedByteArray data) {
    if (hSerial == INVALID_SERIAL_HANDLE) return false;

    UartPacket pkt;
    pkt.id = id;
    int dataLength = data.size();
    
    // Zabezpieczenie przed przepełnieniem
    if (pkt.length > 255) pkt.length  = 255;
    else pkt.length = uint8_t(dataLength);
    
    for (int i = 0; i < pkt.length; i++) {
        pkt.data[i] = data[i];
    }

    uint8_t raw_buffer[262]; // 2 (start) + 1 (id) + 1 (len) + 256 (max data) + 1 (crc)
    unsigned int total_size = makePacket(&pkt, raw_buffer); // Twoja funkcja z uartPacket.hpp

    // WYSYŁKA (Wieloplatformowo)
#ifdef _WIN32
    DWORD bytesWritten;
    WriteFile(hSerial, raw_buffer, total_size, &bytesWritten, NULL);
    return bytesWritten == total_size;
#else
    int bytesWritten = write(hSerial, raw_buffer, total_size);
    return bytesWritten == total_size;
#endif
}

void UartManager::_process(double delta) {
    std::queue<UartPacket> localQueue;

    // 1. Szybkie zablokowanie, skopiowanie i wyczyszczenie głównej kolejki
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        std::swap(localQueue, msgQueue); 
        // Po wyjściu z tych nawiasów {} mutex jest automatycznie zwalniany!
    }
    
    // 2. Przetwarzanie i wysyłanie sygnałów (kolejka główna jest już odblokowana dla wątku UART)
    while (!localQueue.empty()) {
        UartPacket pkt = localQueue.front();
        localQueue.pop();

        godot::PackedByteArray godot_data;
        godot_data.resize(pkt.length);
        for (int i = 0; i < pkt.length; i++) {
            godot_data[i] = pkt.data[i];
        }

        emit_signal("packet_received", pkt.id, godot_data);
    }
}
