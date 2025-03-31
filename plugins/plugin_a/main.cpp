// pluginA.cpp
#include "plugin_api.h"
#include <stdio.h>

#include <imgui.h>

EMSCRIPTEN_KEEPALIVE void RenderA() {
  ImGui::Text("Hello from Plugin A!");
  ImGui::Text("This is a simple plugin that does nothing.");
  ImGui::Text("You can add your own functionality here.");
}

EMSCRIPTEN_KEEPALIVE int pluginMain() {
  printf("[Plugin A] Hello from Plugin A!\n");

  RegisterRenderable([] -> void {
    RenderA();
  });

  return 42;
}
