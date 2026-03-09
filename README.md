Godot UART Manager (GDExtension)
This repository provides a cross-platform GDExtension in C++ for Godot Engine 4, enabling seamless and non-blocking serial (UART) communication with microcontrollers like Arduino, STM32, ESP32, and other hardware.

It handles asynchronous communication on a separate thread and implements a custom binary packet protocol to ensure data integrity without freezing your game.

This project was built using the user-friendly Godot Plus Plus template, which simplifies the entire C++ compilation process with a single setup.py script.

🚀 Features & Who Should Use This?
Hardware Integration: Perfect for anyone wanting to connect external custom controllers, sensors, or microcontrollers to their Godot games.

Cross-Platform: Works on Windows (windows.h) and POSIX systems like Linux/macOS (termios.h).

High Performance: Uses std::thread for asynchronous reading. The main Godot thread is never blocked.

Built-in Packet Parser: Automatically filters noise and verifies data integrity using a checksum and start bytes (0xAB, 0xCD).

🎮 How to use the Plugin (GDScript)
Once you download the compiled plugin (or compile it yourself), simply add it to your Godot project. The extension exposes a custom UartManager node.

Here is a quick example of how to use it in GDScript:

GDScript
extends Node

var uart_manager: UartManager

func _ready():
    uart_manager = UartManager.new()
    add_child(uart_manager)
    
    # Connect the signal emitted when a valid packet is parsed
    uart_manager.packet_received.connect(_on_packet_received)
    
    # Open port (Use "COM3" for Windows, "ttyUSB0" or "ttyACM0" for Linux)
    var success = uart_manager.open_port("COM3", 115200)
    if success:
        print("UART Port opened successfully!")
    else:
        print("Failed to open UART port.")

func _on_packet_received(id: int, data: PackedByteArray):
    print("Received packet! ID: ", id, " Data length: ", data.size())

func send_test_message():
    var data_to_send = PackedByteArray([0x01, 0x02, 0x03])
    # Sends a packet with ID 10 and the data array
    uart_manager.send_packet(10, data_to_send)

func _exit_tree():
    if uart_manager:
        uart_manager.close_port()
🛠️ How to Compile from Source (For Contributors)
If you want to modify the C++ code of this plugin, the development workflow is fully automated.

Requirements
Git installed on your machine.

Python latest version (ensure it's in your system environment PATH).

Scons latest version (ensure it's in your system environment PATH).

Windows: pip install scons

macOS/Linux: python3 -m pip install scons

C++ compiler:

Windows: MSVC (Microsoft Visual C++) via Visual Studio or Build Tools.

macOS: Clang (included with Xcode or Xcode Command Line Tools).

Linux: GCC or Clang.

Visual Studio Code with the clangd extension (disable the default Microsoft C/C++ extension for better IntelliSense).

Development Setup
Clone this repository to your local machine.

Open Visual Studio Code inside the cloned directory (where setup.py is located).

Run the setup.py script:

Open your command line terminal.

Run python setup.py and follow the interactive menu.

Press 4 to compile the plugin debug build.
(Note: The first compilation takes a while as it builds the godot-cpp bindings. Subsequent builds are much faster).

When making changes to the C++ code, keep your Godot test project open. After compiling, simply reload the project in Godot (Project -> Reload Current Project) to see your changes.

🌍 Cross-Platform Compilation (GitHub Actions)
This repository includes GitHub Actions workflows to automatically compile the plugin for Windows, macOS, and Linux.

Commit and push your C++ changes to GitHub.

Go to the Actions tab.

Run the "Build GDExtension Cross Platform Plugin" workflow.

Choose debug to test if it compiles on all OS, or full_plugin_compilation to generate both debug and release builds.

Once finished, download the generated .zip artifact – it's ready to be dropped into any Godot project!

📦 How Does Someone Download And Install The Plugin?
If you just want to use the plugin without writing C++ code:

Go to the Releases tab of this repository.

Download the latest .zip file.

Unzip it directly into your Godot project's folder. The plugin will work out of the box!

📜 Credits & License
Plugin developed by [Your Name/Handle].

This project was built using the godot-plus-plus GDExtension template by nikoladevelops.

Licensed under the MIT License.
