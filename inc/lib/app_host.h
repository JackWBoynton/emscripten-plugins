#pragma once

#include "plugin_manager.h"

class AppHost {
public:
    AppHost();
    int run();

private:
    void setupCommonImGuiState();
    void pollAndRender();

    bool m_loadedDownloadedPlugins = false;
    uint64_t m_serverTimeoutMs;

#ifdef __EMSCRIPTEN__
    static void emscriptenFrameLoop();
#endif
};
