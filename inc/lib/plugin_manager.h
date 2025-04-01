#pragma once

#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <filesystem>

#ifdef EMSCRIPTEN
#include "emscripten.h"
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

// RenderableFunc is a callback for rendering a plugin UI in ImGui
using RenderableFunc = std::function<void()>;

struct LoadablePlugin {
    std::string name;
    uint64_t size;
    std::string sha1;
    std::string version;
    std::filesystem::path downloadedPath = "";
    bool loaded = false;
};

class PluginManager {
public:
    EMSCRIPTEN_KEEPALIVE static PluginManager& getInstance();

    void parsePluginList(const std::string &jsonData);
    int loadPlugin(LoadablePlugin &plugin);

    void fetchPluginList();
    void downloadAndLoadPlugin(LoadablePlugin &plugin);

    std::vector<LoadablePlugin>& getPluginList();

    const std::vector<RenderableFunc>& getRenderables() const;

    void unloadAll();

    EMSCRIPTEN_KEEPALIVE void registerRenderable(RenderableFunc func);

    static void log(const std::string &msg);

    void loadPreDownloadedPlugins();

private:
    PluginManager() = default;
    ~PluginManager() = default;

    std::vector<LoadablePlugin> pluginList_;
    std::vector<RenderableFunc> renderables_;

    std::vector<void*> pluginHandles_;

    int loadPluginFromFile(const std::string &path);
};
