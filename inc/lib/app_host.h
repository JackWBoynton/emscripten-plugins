#pragma once

#include <lib/plugin_manager.h>
#include <hello_imgui/hello_imgui.h>

#include <vector>

class AppHost {
public:
    AppHost();
    int run();
    void ShowPluginManagerWindow();
    void CreateDockableWindows();
    HelloImGui::DockingParams CreateDefaultLayout();

private:

    bool m_loadedDownloadedPlugins = false;
    uint64_t m_serverTimeoutMs;
};
