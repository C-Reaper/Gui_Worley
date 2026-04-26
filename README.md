# Project README

## Overview
The project is a C application that creates an interactive visual representation of Worley noise using the Window Library (Window.h). The main functionality includes drawing points, circles, and updating them based on user input.

## Features
- Interactive visualization of Worley noise.
- Drawing and moving points with mouse events.
- Clearing the canvas.
- Running on Linux, Windows, Wine, and WebAssembly environments.

## Project Structure
```
GuiWorley/
├── build/              # .exe files produced by Main.c
├── src/                # source code
│   ├── Main.c          # Entry point
│   └── Window.h        # Header for window library
├── Makefile.linux      # Linux Build configuration
├── Makefile.windows    # Windows Build configuration
├── Makefile.wine       # Wine Build configuration
├── Makefile.web        # Emscripten Build configuration
└── README.md           # This file
```

### Prerequisites
- C/C++ Compiler and Debugger (GCC, Clang)
- Make utility
- Standard development tools
- Libraries needed in specific projects:
  - **Linux**: X11 for window system and PNG/jpeg for image handling.
  - **Windows**: WINAPI for window system, GDI32 for graphics, and WinMM for multimedia functions.
  - **Wine**: WINEPREFIX and WINEARCH environment variables for cross-compilation to Windows.
  - **WebAssembly**: Emscripten for compiling C code to WebAssembly.

## Build & Run
### Linux
To build and run the project on Linux:
```sh
cd GuiWorley/
make -f Makefile.linux all
make -f Makefile.linux exe
```

### Windows
To build and run the project on Windows:
```sh
cd GuiWorley/
make -f Makefile.windows all
make -f Makefile.windows exe
```

### Wine
To build and run the project on Wine (Linux cross-compilation for Windows):
```sh
cd GuiWorley/
make -f Makefile.wine all
make -f Makefile.wine exe
```

### WebAssembly
To build and run the project as WebAssembly:
```sh
cd GuiWorley/
make -f Makefile.web all
make -f Makefile.web exe
```

These commands will compile the source code, link it with necessary libraries, and produce an executable file. Running `exe` will execute the application based on your operating system.