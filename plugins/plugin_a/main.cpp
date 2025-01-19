// pluginA.cpp
#include "plugin_api.h"
#include <stdio.h>

#if defined(EMSCRIPTEN)
#include "emscripten.h"
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

EMSCRIPTEN_KEEPALIVE int pluginMain() {
  printf("[Plugin A] Hello from Plugin A!\n");
  return 42;
}
