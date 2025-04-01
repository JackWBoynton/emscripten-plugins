// pluginA.cpp
#include "lib/plugin_api.h"
#include "lib/plugin_manager.h"
#include <stdio.h>

#include <memory>

#include <imgui.h>
#include <hello_imgui/hello_imgui.h>

  static std::shared_ptr<HelloImGui::DockableWindow> window;

EMSCRIPTEN_KEEPALIVE int pluginMain() {
  printf("[Plugin A] Hello from Plugin A!\n");


  if (!window) {
    window = std::make_shared<HelloImGui::DockableWindow>();
    window->label = "Plugin A";
    window->dockSpaceName = "MainDockSpace";
    window->GuiFunction = [] {
      ImGui::Text("Plugin A GUI");
      ImGui::Text("This is a simple plugin that does nothing.");
      ImGui::Text("You can add your own functionality here.");
    };
    window->isVisible = true;

    HelloImGui::AddDockableWindow(window);
  }

  return 42;
}
