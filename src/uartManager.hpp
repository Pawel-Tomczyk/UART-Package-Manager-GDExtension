#pragma once

//c++ libraries
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>

#ifdef _WIN32
    // Dla Windowsa
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    typedef HANDLE SerialPortHandle;
    #define INVALID_SERIAL_HANDLE INVALID_HANDLE_VALUE
#else
    // Dla Linuxa / macOS (systemy POSIX)
    #include <termios.h>
    #include <fcntl.h>
    #include <unistd.h>
    typedef int SerialPortHandle;
    #define INVALID_SERIAL_HANDLE -1
#endif

#include "uartPacket.hpp"

//Godot libraries
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

class UartManager : public godot::Node{
    GDCLASS(UartManager, godot::Node)
public:
    UartManager();
    ~UartManager();

    bool openPort(godot::String portName, int baudrate); // otwiera i zaczyna nasłuchiwać na danym porcie
    bool close_port(); // kończy słuchać 
    bool send_packet(int id, godot::PackedByteArray data); // wysyła widomość o podanym id

    void _process(double delta) override;
protected:
    static void _bind_methods();

private:
    godot::String portName;

    std::queue<UartPacket> msgQueue;
    uint8_t rx_buffer[512]; // buffor na surowe dane... 

    SerialPortHandle hSerial = INVALID_SERIAL_HANDLE;

    std::thread workerThread;
    std::mutex queueMutex;
    std::atomic<bool> keepRunningFlag;

    void threadLoop();
};
