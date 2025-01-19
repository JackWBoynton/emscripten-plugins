#include "plugin_api.h"

#include <dlfcn.h>
#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

extern "C" {
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
int LoadPlugin(const char *path) {
  std::cout << "[Host] Attempting to load plugin: " << path << std::endl;

  void *handle = dlopen(path, RTLD_NOW);
  if (!handle) {
    std::cerr << "[Host] dlopen failed: " << dlerror() << std::endl;
    return -1;
  }
  auto pluginFunc = (int (*)())dlsym(handle, "pluginMain");
  if (!pluginFunc) {
    std::cerr << "[Host] dlsym failed: " << dlerror() << std::endl;
    return -1;
  }
  int result = pluginFunc();
  std::cout << "[Host] Plugin returned " << result << std::endl;

  dlclose(handle);
  return 0;
}
} // extern "C"

int main() {
  std::cout << "[Host] Starting up.\n";

#ifndef __EMSCRIPTEN__
  LoadPlugin("./plugins/plugin_a.plugin");
#endif

  return 0;
}
