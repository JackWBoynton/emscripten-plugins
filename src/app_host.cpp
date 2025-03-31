#include "lib/app_host.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <iostream>
#include <chrono>

static GLFWwindow* gWindow = nullptr;
static AppHost* gStaticHost = nullptr;

static uint64_t GetTimeMs()
{
return std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();
}

static void ShowPluginManagerWindow()
{
    ImGui::Begin("Plugin Manager");
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
    ImGui::End();
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

int AppHost::run()
{
    printf("[AppHost] Starting application...\n");
#ifdef EMSCRIPTEN
    // Emscripten environment
    if (!glfwInit()) {
        std::cerr << "[Emscripten] Failed to init GLFW.\n";
        return 1;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    gWindow = glfwCreateWindow(1280, 720, "Emscripten + Plugins", NULL, NULL);
    if (!gWindow) {
        std::cerr << "[Emscripten] Failed to create GLFW window.\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(gWindow);
    glfwSwapInterval(1);

    setupCommonImGuiState();
    PluginManager::getInstance().fetchPluginList();

    emscripten_set_main_loop(emscriptenFrameLoop, 0, true);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(gWindow);
    glfwTerminate();
    return 0;
#else
    if (!glfwInit()) {
        std::cerr << "[Native] Failed to init GLFW.\n";
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    gWindow = glfwCreateWindow(1280, 720, "Native + Plugins", NULL, NULL);
    if (!gWindow) {
        std::cerr << "[Native] Failed to create GLFW window.\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(gWindow);
    glfwSwapInterval(1);

    setupCommonImGuiState();
    PluginManager::getInstance().fetchPluginList();

    while (!glfwWindowShouldClose(gWindow)) {
        pollAndRender();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(gWindow);
    glfwTerminate();
    PluginManager::getInstance().unloadAll();
    return 0;
#endif
}

void AppHost::setupCommonImGuiState()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
#ifdef EMSCRIPTEN
    ImGui_ImplGlfw_InitForOpenGL(gWindow, true);
    ImGui_ImplOpenGL3_Init("#version 300 es");
#else
    ImGui_ImplGlfw_InitForOpenGL(gWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330");
#endif
}

void AppHost::pollAndRender()
{
    if (!m_loadedDownloadedPlugins) {
        if (!PluginManager::getInstance().getPluginList().empty() || GetTimeMs() > m_serverTimeoutMs) {
            PluginManager::getInstance().loadPreDownloadedPlugins();
            m_loadedDownloadedPlugins = true;
        }
    }

    glfwPollEvents();

    int display_w, display_h;
    glfwGetFramebufferSize(gWindow, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ShowPluginManagerWindow();
    ShowRenderables();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(gWindow);
}

#ifdef EMSCRIPTEN
void AppHost::emscriptenFrameLoop()
{
    gStaticHost->pollAndRender();
    if (glfwWindowShouldClose(gWindow)) {
        emscripten_cancel_main_loop();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(gWindow);
        glfwTerminate();
        PluginManager::getInstance().unloadAll();
    }
}
#endif
