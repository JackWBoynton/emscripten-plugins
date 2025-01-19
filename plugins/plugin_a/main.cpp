// pluginA.cpp
#include "plugin_api.h"
#include <stdio.h>

int pluginMain() {
  printf("[Plugin A] Hello from Plugin A!\n");
  return 42;
}
