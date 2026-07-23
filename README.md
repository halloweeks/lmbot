# Lords Mobile Bot

A modular, high-performance Lords Mobile bot written in C.

The project is designed with a strong focus on clean architecture, efficient packet processing, and configurable automation. It aims to provide a stable, maintainable, and extensible codebase that can be easily expanded as new game features and automation modules are developed.

> **Project Status**
>
> This project is under active development. Features, APIs, and internal structures may change between versions.

## Features

- Modular architecture
- High-performance binary protocol implementation
- Efficient packet processing
- Configurable automation system
- Event-driven networking
- Cross-platform build system (CMake)
- MIT licensed

## Requirements

- CMake
- A C compiler:
  - **Linux / macOS:** GCC or Clang
  - **Windows:** MinGW-w64 GCC (recommended) or MSVC

## Building

Clone the repository:

```bash
git clone https://github.com/halloweeks/lords-mobile-bot.git
cd lords-mobile-bot
```

The build system is cross-platform. The resulting executable is named `client` on
Linux/macOS and `client.exe` on Windows.

### Linux / macOS

```bash
mkdir build
cd build

cmake ..
cmake --build .
```

Or simply use the provided build script:

```bash
./build.sh
```

### Windows (MinGW-w64)

Using CMake:

```bat
mkdir build
cd build

cmake -G "MinGW Makefiles" ..
cmake --build .
```

Or simply use the provided build script:

```bat
build.bat
```

Or compile directly with GCC:

```bat
gcc -O2 -Iinclude src\main.c src\connection.c src\log.c src\protocol.c src\des.c src\map_point.c src\command.c src\config.c -o client.exe -lws2_32
```

The Windows build links against `ws2_32` (Winsock); CMake and `build.bat` handle
this automatically.

## Getting Started

> On Windows, use `client.exe` instead of `./client` in the commands below.

Generate a default configuration file:

```bash
./client --create-config
```

This creates a `config.cfg` file in the current directory.

Open `config.cfg` and replace the example values with your own account information:

```ini
# Replace the example values below with your own account information.
account.igg_id = 1234567890
account.device_uuid = 12345678-1234-1234-1234-123456789abc
account.access_key = YOUR_ACCESS_KEY_HERE
```

Save the file, then start the bot:

```bash
./client config.cfg
```

## Command Line Options

Display help:

```bash
./client --help
```

Generate a default configuration file:

```bash
./client --create-config
```

Display version information:

```bash
./client --version
```

Run the bot:

```bash
./client config.cfg
```

## Project Structure

```
include/        Header files
src/            Source code
CMakeLists.txt  CMake build configuration
build.sh        Build helper script (Linux / macOS)
build.bat       Build helper script (Windows / MinGW)
```

## Documentation

Additional documentation will be added as the project continues to develop.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.