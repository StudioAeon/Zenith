# Zenith
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey.svg)]()
[![Bugs](https://sonarcloud.io/api/project_badges/measure?project=StudioAeon_Zenith&metric=bugs)](https://sonarcloud.io/summary/new_code?id=StudioAeon_Zenith)
[![Code Smells](https://sonarcloud.io/api/project_badges/measure?project=StudioAeon_Zenith&metric=code_smells)](https://sonarcloud.io/summary/new_code?id=StudioAeon_Zenith)

A modular, lightweight C++ game engine targeting 3D and (eventually) lightweight 2D games.

## Overview
Zenith Engine is built with modern C++20/C++23 standards and designed for clean, scalable architecture. The engine uses SDL3 for cross-platform window and input abstraction, with support for multiple rendering backends including OpenGL and Vulkan.

**Current Version:** v0.1.0


### Key Features
- **LayerStack System** - Flexible layer management for game and editor systems
- **Type-Safe Event System** - Custom event dispatching with compile-time safety
- **Input Abstraction** - Unified input handling with per-frame state updates
- **Vulkan Rendering** - Modern, high-performance Vulkan-only graphics pipeline
- **Timestep Management** - Consistent delta time system for smooth updates
- **Memory Tracking** - Optional memory profiling and leak detection
- **Integrated Profiling** - Built-in Tracy profiler support

### Hardware Requirements
Vulkan Support Required:
- **GPU:** Vulkan 1.2+ compatible graphics card
  - NVIDIA GTX 10 Series (Pascal) or newer
  - AMD RX 400 Series (Polaris) or newer
  - Intel Arc or Iris Xe Graphics
- **Drivers:** Up-to-date graphics drivers with Vulkan 1.2+ supportw

### Building
Zenith Engine is designed to be straightforward to build with minimal dependencies. Currently tested on:
- **Windows 10/11** with Visual Studio 2020+
- **Linux** with GCC 11+ or Clang 12+

### Prerequisites
Make sure you have the following installed:
- [CMake](https://cmake.org/download/) 3.24 or higher
- [Git](https://git-scm.com/downloads)
- [Vulkan SDK](https://vulkan.lunarg.com/) Vulkan SDK 1.3.275+ (REQUIRED)
- A C++20 compatible compiler
- Vulkan-compatible graphics drivers

### Vulkan SDK Installation
1. Download and install the latest Vulkan SDK from [LunarG](https://vulkan.lunarg.com/)
2. Ensure the ```VULKAN_SDK``` environment variable is set correctly
3. Verify installation by running ```vulkaninfo``` command

### Build Steps
1. **Clone the repository:**
	```bash
	git clone --recursive https://github.com/StudioAeon/Zenith.git
	cd Zenith
	```
2. **Configure and build:**
	```bash
	cmake -B build -DCMAKE_BUILD_TYPE=Release
	cmake --build build --config Release
	```
3. **Run the editor**
	- On Linux
		```bash
		./build/Editor/Zenith-Editor
		```
	- On Windows (CMD or PowerShell)
		```bash
		.\build\Editor\Release\Zenith-Editor.exe
		```

### Build Options
- ```ZENITH_TRACK_MEMORY=ON/OFF``` - Enable memory tracking (default: ON, disabled in Release)
- ```ZENITH_TESTS=ON/OFF``` - Build unit tests (default: ON)
- ```CMAKE_BUILD_TYPE=Debug/Release``` - Build configuration

### Testing
Run the test suite:
```bash
cmake --build build --target run-tests
```

### Roadmap
- 3D rendering pipeline
- Entity-Component-System (ECS)
- Scene Serialization and management
- Asset pipeline and hot-reload
- Scripting integration (Lua)
- Physics integration
- Audio System

### Contributing
This is currently a personal development project. If you're interested in contributing or have questions, please reach out!

### License
This project is licensed under the MIT License.
