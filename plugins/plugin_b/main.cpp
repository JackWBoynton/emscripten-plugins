// pluginB.cpp
#include "lib/plugin_api.h"
#include <stdio.h>

#if defined(EMSCRIPTEN)
#include "emscripten.h"
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

EMSCRIPTEN_KEEPALIVE int pluginMain() {
  printf("[Plugin B] Hello from Plugin B!\n");
  return 77;
}
