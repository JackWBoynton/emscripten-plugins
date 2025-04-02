#pragma once

#include <string>

#if defined(EMSCRIPTEN)
#include "emscripten.h"
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

#ifdef __cplusplus
extern "C" {
#endif


int pluginMain();


#ifdef __cplusplus
}
#endif

inline std::string getPluginArchitecture() {
  #if defined(EMSCRIPTEN)
    return "emscripten";
  #elif defined(__APPLE__)
    #if defined(__arm64__) || defined(__aarch64__)
      return "macos_arm64";
    #else
      return "macos_x86_64";
    #endif
  #elif defined(_WIN32)
    return "windows";
  #elif defined(__linux__)
    #if defined(__x86_64__) || defined(_M_X64)
      return "linux_x86_64";
    #elif defined(__i386__) || defined(_M_IX86)
      return "linux_x86";
    #elif defined(__aarch64__) || defined(_M_ARM64)
      return "linux_arm64";
    #else
      #error "Unknown architecture"
    #endif

  #else
    #error "Unknown platform"

  #endif
}
