#pragma once

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
