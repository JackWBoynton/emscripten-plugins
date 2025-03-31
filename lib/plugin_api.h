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

using RenderableFunc = void (*)();
EMSCRIPTEN_KEEPALIVE void RegisterRenderable(RenderableFunc func);
//__attribute__((weak)) void RegisterRenderable(RenderableFunc);

#ifdef __cplusplus
}
#endif
