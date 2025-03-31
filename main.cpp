#include <cstring>
#include <dlfcn.h>
#include <iostream>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"

#include "plugin_api.h"

static std::vector<RenderableFunc> gRenderables;
static std::vector<void *> gPluginHandles;

EMSCRIPTEN_KEEPALIVE void RegisterRenderable(RenderableFunc func) {
  gRenderables.push_back(std::move(func));

  printf("[Host] Registered renderable\n");
}

static void ShowRenderables() {
  for (auto &renderable : gRenderables) {
    if (ImGui::Begin("Plugin Renderable")) {
      renderable();
    }
    ImGui::End();
  }
}

// ------------- Plugin Discovery + Download Code -------------

static std::vector<std::string> gPluginList;
static int gSelectedPluginIndex = -1;

static const char *PLUGIN_LIST_URL = "http://localhost:8000/plugins";
static const char *PLUGIN_BASE_URL = "http://localhost:8000/plugins/";

// Forward declarations
void fetchPluginList();
void downloadAndLoadPlugin(const std::string &pluginName);
static int LoadPluginFromFile(const char *path);

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/fetch.h>

// Emscripten: fetch plugin list
static void onFetchListSuccess(emscripten_fetch_t *fetch) {
  std::string data((char *)fetch->data, fetch->numBytes);
  emscripten_fetch_close(fetch);

  gPluginList.clear();
  // Naive JSON parse for ["pluginA.wasm","pluginB.wasm"]
  size_t pos = 0;
  while ((pos = data.find('"', pos)) != std::string::npos) {
    size_t start = pos + 1;
    size_t end = data.find('"', start);
    if (end == std::string::npos)
      break;
    gPluginList.push_back(data.substr(start, end - start));
    pos = end + 1;
  }
  std::cout << "[Web] Found " << gPluginList.size() << " plugins.\n";
}

static void onFetchListFailed(emscripten_fetch_t *fetch) {
  std::cerr << "[Web] Plugin list fetch failed, status=" << fetch->status
            << "\n";
  emscripten_fetch_close(fetch);
}

void fetchPluginList() {
  std::cout << "[Web] Fetching plugin list...\n";
  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
  attr.onsuccess = onFetchListSuccess;
  attr.onerror = onFetchListFailed;
  emscripten_fetch(&attr, PLUGIN_LIST_URL);
}

static void onPluginFetchSuccess(emscripten_fetch_t *fetch) {
  struct DownloadCtx {
    std::string localPath;
  };
  auto *ctx = (DownloadCtx *)fetch->userData;

  FILE *fp = fopen(ctx->localPath.c_str(), "wb");
  if (fp) {
    fwrite(fetch->data, 1, fetch->numBytes, fp);
    fclose(fp);
    // Now load it
    LoadPluginFromFile(ctx->localPath.c_str());
  } else {
    std::cerr << "[Web] Could not create file: " << ctx->localPath << "\n";
  }

  emscripten_fetch_close(fetch);
  delete ctx;
}

static void onPluginFetchFailed(emscripten_fetch_t *fetch) {
  std::cerr << "[Web] Plugin fetch failed, status=" << fetch->status << "\n";
  struct DownloadCtx {
    std::string localPath;
  };
  auto *ctx = (DownloadCtx *)fetch->userData;
  delete ctx;
  emscripten_fetch_close(fetch);
}

void downloadAndLoadPlugin(const std::string &pluginName) {
  if (pluginName.empty())
    return;
  struct DownloadCtx {
    std::string localPath;
  };
  auto *ctx = new DownloadCtx();
  ctx->localPath = pluginName;

  std::string url = std::string(PLUGIN_BASE_URL) + pluginName;
  std::cout << "[Web] Downloading " << url << "\n";

  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_PERSIST_FILE;
  attr.onsuccess = onPluginFetchSuccess;
  attr.onerror = onPluginFetchFailed;
  attr.userData = ctx;
  emscripten_fetch(&attr, url.c_str());
}

#else // Native

#include <curl/curl.h>
#include <fstream>

void fetchPluginList() {
  std::cout << "[Native] Fetching plugin list...\n";
  CURL *curl = curl_easy_init();
  if (!curl) {
    std::cerr << "[Native] Curl init failed.\n";
    return;
  }
  std::string response;
  curl_easy_setopt(curl, CURLOPT_URL, PLUGIN_LIST_URL);
  curl_easy_setopt(
      curl, CURLOPT_WRITEFUNCTION,
      +[](char *ptr, size_t size, size_t nmemb, void *userdata) -> size_t {
        auto *resp = (std::string *)userdata;
        resp->append(ptr, size * nmemb);
        return size * nmemb;
      });
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    std::cerr << "[Native] Plugin list fetch failed: "
              << curl_easy_strerror(res) << "\n";
    return;
  }
  gPluginList.clear();
  // Naive parse of JSON array
  size_t pos = 0;
  while ((pos = response.find('"', pos)) != std::string::npos) {
    size_t start = pos + 1;
    size_t end = response.find('"', start);
    if (end == std::string::npos)
      break;
    gPluginList.push_back(response.substr(start, end - start));
    pos = end + 1;
  }
  std::cout << "[Native] Found " << gPluginList.size() << " plugins.\n";
}

void downloadAndLoadPlugin(const std::string &pluginName) {
  if (pluginName.empty())
    return;
  std::string url = std::string(PLUGIN_BASE_URL) + pluginName;
  std::cout << "[Native] Downloading " << url << "\n";

  CURL *curl = curl_easy_init();
  if (!curl) {
    std::cerr << "[Native] curl init failed.\n";
    return;
  }
  std::ofstream out(pluginName, std::ios::binary);
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(
      curl, CURLOPT_WRITEFUNCTION,
      +[](char *ptr, size_t size, size_t nmemb, void *userdata) -> size_t {
        std::ofstream *fout = (std::ofstream *)userdata;
        fout->write(ptr, size * nmemb);
        return size * nmemb;
      });
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  out.close();

  if (res != CURLE_OK) {
    std::cerr << "[Native] Download failed: " << curl_easy_strerror(res)
              << "\n";
    return;
  }
  LoadPluginFromFile(pluginName.c_str());
}

#endif // end native

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

static int LoadPluginFromFile(const char *path) {
  std::cout << "[Host] Loading plugin: " << path << "...\n";

#ifdef _WIN32
  HMODULE handle = LoadLibraryA(path);
  if (!handle) {
    DWORD errorCode = GetLastError();
    std::cerr << "LoadLibrary error: " << errorCode << "\n";
    return -1;
  }

  typedef int (*PluginMainFunc)();
  PluginMainFunc func = (PluginMainFunc)GetProcAddress(handle, "pluginMain");
  if (!func) {
    DWORD errorCode = GetLastError();
    std::cerr << "GetProcAddress error: " << errorCode << "\n";
    FreeLibrary(handle);
    return -1;
  }
#else
  std::cout << "[Host] dlopen(" << path << ")...\n";
  void *handle = dlopen(path, RTLD_NOW);
  if (!handle) {
    std::cerr << "dlopen error: " << dlerror() << "\n";
    return -1;
  }

  typedef int (*PluginMainFunc)();
  PluginMainFunc func = (PluginMainFunc)dlsym(handle, "pluginMain");
  char *error = dlerror();
  if (error != NULL) {
    std::cerr << "dlsym error: " << error << "\n";
    dlclose(handle);
    return -1;
  }
#endif

  int ret = func();
  std::cout << "[Host] pluginMain() returned " << ret << "\n";

  gPluginHandles.push_back(handle);

  return ret;
}

static void UnloadPlugins() {
  gRenderables.clear();
  for (auto handle : gPluginHandles) {
    std::cout << "[Host] Unloading plugin...\n";
    #ifdef _WIN32
      FreeLibrary(handle);
    #else
      dlclose(handle);
    #endif
  }
}

// ------------- ImGui UI -------------
static void ShowPluginManagerWindow() {
  ImGui::Begin("Plugin Manager");

  if (ImGui::Button("Refresh Plugin List")) {
    fetchPluginList();
    gSelectedPluginIndex = -1;
  }

  ImGui::Text("Available Plugins:");
  for (int i = 0; i < (int)gPluginList.size(); i++) {
    bool selected = (gSelectedPluginIndex == i);
    if (ImGui::Selectable(gPluginList[i].c_str(), selected)) {
      gSelectedPluginIndex = i;
    }
  }

  if (ImGui::Button("Download & Load") && gSelectedPluginIndex >= 0) {
    downloadAndLoadPlugin(gPluginList[gSelectedPluginIndex]);
  }

  ImGui::End();
}

// ------------- Main Loop Setup With GLFW + OpenGL3 -------------
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
GLFWwindow *gWindow = nullptr;

static void FrameLoop() {
  // Poll events (GLFW)
  glfwPollEvents();
  if (glfwWindowShouldClose(gWindow)) {
    emscripten_cancel_main_loop();
    return;
  }
  // Get framebuffer size
  int display_w, display_h;
  glfwGetFramebufferSize(gWindow, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // Start ImGui frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  // Show our plugin manager
  ShowPluginManagerWindow();

  ShowRenderables();

  // Render
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  glfwSwapBuffers(gWindow);
}

int main() {
  // Initialize GLFW
  if (!glfwInit()) {
    std::cerr << "[Web] Failed to init GLFW.\n";
    return 1;
  }
  // Setup GLFW for OpenGL ES 2 or 3 in Emscripten
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  gWindow =
      glfwCreateWindow(1280, 720, "ImGui + GLFW + Plugins (Web)", NULL, NULL);
  if (!gWindow) {
    std::cerr << "[Web] Failed to create GLFW window.\n";
    glfwTerminate();
    return 1;
  }
  glfwMakeContextCurrent(gWindow);
  glfwSwapInterval(1);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  // ImGuiIO &io = ImGui::GetIO(); (void)io;
  ImGui::StyleColorsDark();

  // Setup Platform/Renderer bindings
  ImGui_ImplGlfw_InitForOpenGL(gWindow, true);
  // Use OpenGL ES 3 shader version
  ImGui_ImplOpenGL3_Init("#version 300 es");

  // Run main loop
  emscripten_set_main_loop(FrameLoop, 0, true);

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(gWindow);
  glfwTerminate();

  UnloadPlugins();
  return 0;
}
#else
// Native
int main() {
  if (!glfwInit()) {
    std::cerr << "[Native] Failed to init GLFW.\n";
    return 1;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  // Use core profile if desired
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(
      1280, 720, "ImGui + GLFW + Plugins (Native)", NULL, NULL);
  if (!window) {
    std::cerr << "[Native] Failed to create GLFW window.\n";
    glfwTerminate();
    return 1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330"); // GL 3.3 on desktop

  curl_global_init(CURL_GLOBAL_DEFAULT);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Our plugin manager UI
    ShowPluginManagerWindow();

    ShowRenderables();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
  }

  curl_global_cleanup();
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();

  UnloadPlugins();
  return 0;
}
#endif
