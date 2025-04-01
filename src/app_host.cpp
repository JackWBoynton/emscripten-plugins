#include "lib/app_host.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#include <iostream>
#include <chrono>

#include <hello_imgui/hello_imgui.h>
#include "hello_imgui/runner_params.h"

static AppHost* gStaticHost = nullptr;

static uint64_t GetTimeMs()
{
return std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();
}

void AppHost::ShowPluginManagerWindow()
{
    if (!m_loadedDownloadedPlugins) {
        if (!PluginManager::getInstance().getPluginList().empty() || GetTimeMs() > m_serverTimeoutMs) {
            PluginManager::getInstance().loadPreDownloadedPlugins();
            m_loadedDownloadedPlugins = true;
        }
    }


    if (ImGui::Button("Refresh Plugin List")) {
        PluginManager::getInstance().fetchPluginList();
    }

    ImGui::Text("Available Plugins:");
    auto& list = PluginManager::getInstance().getPluginList();
    static int selectedPlugin = -1;
    for (int i = 0; i < (int)list.size(); i++) {
        bool selected = (selectedPlugin == i);
        if (ImGui::Selectable(list[i].name.c_str(), selected, list[i].loaded ? ImGuiSelectableFlags_Disabled : 0)) {
            selectedPlugin = i;
        }
    }

    if (ImGui::Button("Download & Load") && selectedPlugin >= 0) {
        PluginManager::getInstance().downloadAndLoadPlugin(list[selectedPlugin]);
    }
}

static void ShowRenderables()
{
    for (auto& renderable : PluginManager::getInstance().getRenderables()) {
        if (ImGui::Begin("Plugin Renderable")) {
            renderable();
        }
        ImGui::End();
    }
}

AppHost::AppHost()
{
    gStaticHost = this;
    m_serverTimeoutMs = GetTimeMs() + 1000;

#ifdef EMSCRIPTEN
    EM_ASM(
        FS.mkdir('/plugins'); 
        FS.mount(IDBFS,{},'/plugins');
 
        FS.syncfs(true, function(err) {
            assert(!err);
            Module.print("end file sync..");
            Module.syncdone = 1;
       });
    );

    EM_ASM({
        FS.readdir('/plugins').forEach(function(file) {
            if (file.endsWith('.wasm')) {
                var path = '/plugins/' + file;
                Module['preDownloadedPlugins'].push(path);
            }
        });
    });

#endif
}

static std::shared_ptr<HelloImGui::DockableWindow> s_featuresDemoWindow;

void AppHost::CreateDockableWindows()
{
    static bool first = true;
    if (first)
    {
        first = false;
        s_featuresDemoWindow = std::make_shared<HelloImGui::DockableWindow>();
        s_featuresDemoWindow->label = "Plugin Manager";
        s_featuresDemoWindow->dockSpaceName = "MainDockSpace";
        s_featuresDemoWindow->GuiFunction = [&] { ShowPluginManagerWindow(); };
        s_featuresDemoWindow->isVisible = true;

        HelloImGui::AddDockableWindow(s_featuresDemoWindow, true);

        printf("[AppHost] Created dockable window: %s\n", s_featuresDemoWindow->label.c_str());
    }
}

HelloImGui::DockingParams AppHost::CreateDefaultLayout()
{
    CreateDockableWindows();

    HelloImGui::DockingParams dockingParams;
    dockingParams.dockingSplits = {
        { "MainDockSpace", "CommandSpace", ImGuiDir_Down, 0.5f },
        { "MainDockSpace", "MiscDockSpace", ImGuiDir_Right, 0.5f }
    };
    return dockingParams;
}

int AppHost::run()
{
    printf("[AppHost] Starting application...\n");

    HelloImGui::RunnerParams runnerParams;
    runnerParams.appWindowParams.windowTitle = "Plugin Host";
    runnerParams.appWindowParams.windowGeometry.size = {1280, 720};
    runnerParams.appWindowParams.restorePreviousGeometry = true;

    runnerParams.dockingParams = CreateDefaultLayout();

    runnerParams.imGuiWindowParams.showMenuBar = true;

    runnerParams.imGuiWindowParams.defaultImGuiWindowType = HelloImGui::DefaultImGuiWindowType::ProvideFullScreenDockSpace;

    runnerParams.imGuiWindowParams.enableViewports = false;

    runnerParams.dockingParams.layoutCondition = HelloImGui::DockingLayoutCondition::FirstUseEver;
    runnerParams.iniFolderType = HelloImGui::IniFolderType::AppUserConfigFolder;
    runnerParams.iniFilename = "plugins-dev/plugins-dev.ini";

    PluginManager::getInstance().fetchPluginList();

    HelloImGui::Run(runnerParams);

    PluginManager::getInstance().unloadAll();

    return EXIT_SUCCESS;
}