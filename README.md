# UART Manager GDExtension for Godot 4

Cross-platform UART/serial communication for Godot 4 using GDExtension and C++.

`UartManager` runs serial reads on a background thread, parses packets with framing + checksum, and emits data back to Godot through a signal.

## Features

- Non-blocking serial read loop (`std::thread`)
- Packet framing with start bytes (`0xAB`, `0xCD`)
- XOR checksum validation
- Signal-based delivery to GDScript: `packet_received(id, data)`
- Cross-platform implementation for Windows and POSIX (Linux/macOS)

## Repository Layout

- `src/uartManager.cpp`, `src/uartManager.hpp`: main Godot node + serial handling
- `src/uartPacket.cpp`, `src/uartPacket.hpp`: packet encoding/decoding helpers
- `src/register_types.cpp`: GDExtension class registration (`UartManager`)
- `test_project/uartmanager/uartmanager.gdextension`: extension config used by Godot
- `SConstruct`: build entrypoint for all targets

## Godot API

The extension registers one class:

- `UartManager` (inherits `Node`)

Methods:

- `open_port(portName: String, baudrate: int) -> bool`
- `close_port() -> bool`
- `send_packet(id: int, data: PackedByteArray) -> bool`

Signal:

- `packet_received(id: int, data: PackedByteArray)`

## Packet Protocol

Packet format used by `send_packet` and parser:

`[0xAB][0xCD][ID][LEN][DATA...][CHECKSUM]`

- `ID`: 1 byte
- `LEN`: 1 byte (`0..255`)
- `DATA`: `LEN` bytes
- `CHECKSUM`: XOR of `ID ^ LEN ^ each(DATA byte)`

## Custom Packets Without Recompiling

You cannot add a new C++ `struct` at runtime (that always needs recompilation), but you can define packet layout in GDScript and serialize/deserialize to `PackedByteArray`.

This repository includes a helper codec:

- `test_project/uartmanager/uart_packet_codec.gd`

When used in a Godot project, preload it as:

```gdscript
const UartPacketCodec = preload("res://uartmanager/uart_packet_codec.gd")
```

Example for runtime IMU packet (`id = 0x20`):

```gdscript
const PKT_IMU_DATA := 0x20

func send_imu(uart: UartManager) -> void:
    var payload := UartPacketCodec.pack_by_schema(
        {"x": 1.0, "y": 2.0, "z": 3.0, "mag": 4.0},
        UartPacketCodec.IMU_SCHEMA,
        true # big_endian
    )
    uart.send_packet(PKT_IMU_DATA, payload)

func _on_packet_received(id: int, data: PackedByteArray) -> void:
    if id == PKT_IMU_DATA:
        var imu := UartPacketCodec.unpack_by_schema(data, UartPacketCodec.IMU_SCHEMA, true)
        print("IMU:", imu)
```

Important:

- Both sides (Godot and MCU/PC peer) must use the same schema order and numeric types.
- Both sides must use the same endianness (`big_endian=true` in the example).
- `double` in your C++ idea maps to `f64` in the schema.

## Quick Start in GDScript

```gdscript
extends Node

var uart: UartManager

func _ready() -> void:
    uart = UartManager.new()
    add_child(uart)

    # _process is overridden in C++; enabling processing ensures queued packets are emitted.
    uart.set_process(true)

    uart.packet_received.connect(_on_packet_received)

    # Windows: "COM3"
    # Linux/macOS: "ttyUSB0", "ttyACM0", "tty.usbserial-..." (without /dev/)
    var ok := uart.open_port("ttyUSB0", 115200)
    print("UART open:", ok)

    var payload := PackedByteArray([0x01, 0x02, 0x03])
    uart.send_packet(10, payload)

func _on_packet_received(id: int, data: PackedByteArray) -> void:
    print("Packet", id, data)

func _exit_tree() -> void:
    if uart:
        uart.close_port()
```

## Installation

### Option 1: Download Pre-built Release (Recommended)

Download the latest `uartmanager.zip` from the [Releases](https://github.com/Pawel-Tomczyk/UART-Package-GDExtension/releases) page.

1. Extract the `uartmanager` folder from the zip
2. Copy it into your Godot project
3. Restart Godot to load the extension
4. You can now use `UartManager` in your scripts

The release includes pre-compiled binaries for Linux, Windows, and macOS.

### Option 2: Build from Source

### Requirements

- Python 3
- SCons
- C++ toolchain
  - Linux: GCC or Clang
  - macOS: Xcode command line tools
  - Windows: MSVC build tools

Install SCons:

```bash
python3 -m pip install scons
```

Initialize submodules:

```bash
git submodule update --init --recursive
```

### Build (current host platform)

From repository root:

```bash
scons compiledb=yes
```

This produces host binaries in `bin/<platform>/` and updates `compile_commands.json`.

### Build with setup menu (optional)

You can also run the helper menu:

```bash
python3 setup.py
```

Then choose `4` to compile a debug build.

### Using Your Built Extension in a Godot Project

After building:

1. Copy the `uartmanager` extension folder (including `uartmanager.gdextension` and `bin/`) into your Godot project.
2. Ensure paths in `uartmanager.gdextension` match your shipped binaries.
3. Instantiate `UartManager` from script and call `open_port`.

The sample config in `test_project/uartmanager/uartmanager.gdextension` shows expected library naming per platform.

## Compatibility

- `compatibility_minimum = "4.5"` in the current `.gdextension` file.

## License

MIT License. See `LICENSE`.
