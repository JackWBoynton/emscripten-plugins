// pluginA.cpp
#include "lib/plugin_api.h"
#include "lib/plugin_manager.h"
#include <stdio.h>

#include <imgui.h>

EMSCRIPTEN_KEEPALIVE int pluginMain() {
  printf("[Plugin A] Hello from Plugin A!\n");

  PluginManager::getInstance().registerRenderable([] -> void {
    ImGui::Text("Hello from Plugin A!");
    ImGui::Text("This is a simple plugin that does nothing.");
    ImGui::Text("You can add your own functionality here.");
  });

  return 42;
}
