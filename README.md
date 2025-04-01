# Plugin-Based C++ and ImGui Application

This project makes it simple to build a plugin-based framework that dynamically loads modules in C++ and renders UI elements in [Dear ImGui](https://github.com/ocornut/imgui). It works on both desktop and Emscripten/WebAssembly targets. Below are expanded instructions to guide you in using this repository as a starting point for your own plugin system application.

## Table of Contents
- [Overview](#overview)
- [Core Components](#core-components)
  - [AppHost](#apphost)
  - [PluginManager](#pluginmanager)
  - [Plugins](#plugins)
- [Project Layout](#project-layout)
- [Dependencies](#dependencies)
- [Quick Start](#quick-start)
- [Building](#building)
  - [Native](#native)
  - [Emscripten (WebAssembly)](#emscripten-webassembly)
- [Running](#running)
  - [Native Desktop Usage](#native-desktop-usage)
  - [Browser Usage](#browser-usage)
- [Adding or Modifying Plugins](#adding-or-modifying-plugins)
  - [Local Plugins](#local-plugins)
  - [Remote Plugins (for Emscripten)](#remote-plugins-for-emscripten)
- [Troubleshooting and Tips](#troubleshooting-and-tips)
- [Docker Usage](#docker-usage)

---

## Overview

This framework is built around a simple concept:  
1. A host application, which manages the main event loop and ImGui setup.  
2. A plugin manager, which can load and unload plugin code.  
3. Individual plugins, each implementing the `pluginMain()` function, optionally adding interactive UI elements via ImGui.

By organizing your application in this way, you can focus on developing new features as plugins that won’t require you to recompile or modify the host each time.

## Core Components

### AppHost
Located in [src/app_host.cpp](src/app_host.cpp) and declared in [inc/lib/app_host.h](inc/lib/app_host.h).  
- Sets up a [GLFW](https://www.glfw.org) window (or an Emscripten/OpenGL ES context in the browser).  
- Initializes ImGui (colors, render loops, etc.).  
- Requests a list of plugins from the `PluginManager` and orchestrates loading them.  
- On each frame, calls any registered render callbacks, letting plugins draw their own UI.

### PluginManager
Declared in [inc/lib/plugin_manager.h](inc/lib/plugin_manager.h) and implemented in [src/plugin_manager.cpp](src/plugin_manager.cpp).  
- Maintains a list of available plugins (`LoadablePlugin` structs).  
- Responsible for retrieving plugin metadata (local or remote) and actually loading plugin binaries.  
- Offers `registerRenderable(std::function<void()>)`, so plugins can add custom UI blocks to the ImGui interface.  
- In Emscripten builds, can download plugin `.wasm` modules from a remote server, then load them dynamically.  
- Also handles plugin unloading when the application closes.

### Plugins
Found in the `plugins/` directory.  
- Each plugin implements `pluginMain()`.  
- Plugins can optionally add an ImGui callback by calling `PluginManager::getInstance().registerRenderable(...)`.  
- Example: **plugin_a** shows how to add your own UI text in an ImGui window; **plugin_b** logs to console only.

## Project Layout
```
.
├── cmake/
├── external/
├── inc/
│   └── lib/
│       ├── app_host.h
│       ├── plugin_api.h
│       ├── plugin_manager.h
│       └── tiny_sha1.hpp
├── plugins/
│   ├── plugin_a/
│   └── plugin_b/
├── src/
│   ├── app_host.cpp
│   └── plugin_manager.cpp
├── web/
├── CMakeLists.txt
├── Dockerfile
├── main.cpp
└── server.py
```

## Dependencies
- C++17+ compiler (e.g. g++, clang++).  
- [CMake 3.10+](https://cmake.org).  
- [GLFW](https://github.com/glfw/glfw) for window/context creation.  
- [Dear ImGui](https://github.com/ocornut/imgui) included in `external/`.  
- [Emscripten](https://emscripten.org/) if targeting WebAssembly.  

## Quick Start

1. Clone this repository.  
2. Build using CMake.  
3a. Run `python3 server.py build` to start a local server for hosting plugins.
3b. Run `./native/host`, and watch the console output.  
4. Modify or create plugins in `plugins/`.  
5. Rebuild and re-run to see your new functionality in action!

## Building

### Native
1. Make a build directory and run CMake:
   ```
   git clone --recursive https://github.com/jackwboynton/emscripten-plugins.git
   cd emscripten-plugins
   mkdir build && cd build
   cmake ..
   cmake --build .
   ```
2. The compiled executable (e.g. `native/host`) will appear in `build/`.  

### Emscripten (WebAssembly)
1. Install [Emscripten](https://emscripten.org/docs/getting_started/downloads.html).  
2. From a new `build-emscripten` folder:
   ```
   emcmake cmake ..
   emmake make
   cp ../web/* ./web/
   python3 ../server.py .
   ```
3. This produces `.wasm` and supporting JS artifacts. Combine them with a simple HTML page for deployment (e.g. use the example in `web/`).

## Running

### Native Desktop Usage
From the build directory:
```
./native/host
```
- Press the close button on your window to exit.
- Check the console for messages from loaded plugins.

### Browser Usage
1. Serve the `index.html` (in `web/`) with `python3 server.py <build_dir>` to server plugins and the HTML for emscripten builds. 
2. Visit http://localhost:8000/ (adjust port as needed).  
3. The plugin manager can fetch `.wasm` plugins from a remote path. Check your console logs for plugin load messages.

## Adding or Modifying Plugins

### Local Plugins
1. Create a new folder under `plugins/` (e.g. `plugin_features/`).
2. Add a `main.cpp` with a `pluginMain()` function:
   ```cpp
   #include "lib/plugin_api.h"
   #include "lib/plugin_manager.h"
   #include <stdio.h>

   int pluginMain() {
     printf("[NewPlugin] Hello from my new plugin!\n");
     PluginManager::getInstance().registerRenderable([]() {
       ImGui::Text("Hello, this is my new plugin's UI!");
     });
     return 0;
   }
   ```
3. Add a `CMakeLists.txt` referencing this folder as a plugin target.  
4. Rebuild the project, then run. Your plugin should load automatically if configured in the top-level CMake.

### Remote Plugins (for Emscripten)
- In a browser environment, `PluginManager` can fetch a plugin list JSON and download corresponding `.wasm` binaries to the virtual filesystem.  
- Configure the server to serve plugin `.wasm` files under a certain path (e.g. `/plugins`).
- Updating the plugin list or reloading the page will sync the new plugins.

## Troubleshooting and Tips
1. **Check console output** for errors. The plugin manager logs details about downloads, loads, and fails.  
2. **Watch for file paths**. Make sure your plugins (native or `.wasm`) are in a location accessible by the application.  
3. **Verify ImGui version**. If you encounter UI rendering issues, confirm Dear ImGui is updated.  
4. **Linking**. On some platforms, you may need special compiler/linker flags for dynamically loading plugins.  
5. **Use the Dockerfile** if local environment setup is complex.

## Docker Usage
To avoid local dependency issues:  
```
docker build -t plugin_app_container .
docker run --rm -it plugin_app_container
```
The Docker container will compile and run the example application. Adjust the Dockerfile if you need additional libraries or a different environment.