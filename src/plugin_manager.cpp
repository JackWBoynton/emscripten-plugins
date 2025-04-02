#include "lib/plugin_manager.h"
#include "lib/plugin_api.h"

// Ensure standard library headers are included
#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <utility>
#include <filesystem>
#include <cassert>
#include <nlohmann/json.hpp>

#include "lib/tiny_sha1.hpp"

#ifdef EMSCRIPTEN
#include <emscripten/fetch.h>
#else
#include <curl/curl.h>
#endif

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

std::string GetAPIURL() {
    std::string API_URL = "";
    if (!std::getenv("API_URL")) {
        printf("API_URL not set, using default %s\n", "https://plugins-dev-4cd350c041fa.herokuapp.com/api");
        API_URL = "https://plugins-dev-4cd350c041fa.herokuapp.com/api";
    } else {
        API_URL = std::string(std::getenv("API_URL"));
    }

    return API_URL;
}

static std::string GetPluginListUrl() {
    return GetAPIURL() + "/plugins/" + getPluginArchitecture();
}

static std::string GetPluginBaseUrl() {
    return GetPluginListUrl() + "/";
}

#if defined(__APPLE__)
static const char* PLUGIN_DEST = "~/Library/Application Support/plugin_dev/plugins/";
#elif defined(_WIN32)
static const char* PLUGIN_DEST = "C:\\Users\\<username>\\AppData\\Roaming\\plugin_dev\\plugins\\";
#elif defined(EMSCRIPTEN)
static const char* PLUGIN_DEST = "/plugins/";
#elif defined(__linux__)
static const char* PLUGIN_DEST = "~/.local/share/plugin_dev/plugins/";
#else
#error "Unknown platform"
#endif

#include <format>
#include <string>
#include <cstdint>

std::string sha1FileHex(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Could not open file: " + filePath);
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        throw std::runtime_error("Could not read file: " + filePath);
    }
    
    sha1::SHA1 sha;
    sha.processBytes(buffer.data(), static_cast<size_t>(size));
    
    unsigned char digest[20];
    sha.getDigestBytes(digest);
    
    std::string sha1Hash;
    for (size_t i = 0; i < 20; ++i) {
        sha1Hash += std::format("{:02x}", digest[i]);
    }
    return sha1Hash;
}

PluginManager& PluginManager::getInstance() {
    static PluginManager instance;
    if (!std::filesystem::exists(PLUGIN_DEST)) {
        std::filesystem::create_directories(PLUGIN_DEST);
    }
    return instance;
}

void PluginManager::log(const std::string& msg)
{
    std::cout << "[PluginManager] " << msg << std::endl;
}

void PluginManager::registerRenderable(RenderableFunc func)
{
    renderables_.push_back(std::move(func));
    log("Registered renderable function.");
}

std::vector<LoadablePlugin>& PluginManager::getPluginList()
{
    return pluginList_;
}

const std::vector<RenderableFunc>& PluginManager::getRenderables() const
{
    return renderables_;
}

void PluginManager::parsePluginList(const std::string &jsonData)
{
    pluginList_.clear();
    
    try {
        auto json = nlohmann::json::parse(jsonData);
        if (json.is_array()) {
            for (const auto& item : json) {
                if (item.is_object()) {
                    pluginList_.emplace_back(LoadablePlugin{
                        item.value("name", ""),
                        item.value("size", 0UL),
                        item.value("sha1", ""),
                        item.value("version", "")
                    });
                }
            }
        } else {
            log("Invalid JSON format: expected an array.");
            return;
        }
    } catch (const nlohmann::json::parse_error& e) {
        log("JSON parse error: " + std::string(e.what()));
        return;
    }
    
    log("Parsed plugin list: " + std::to_string(pluginList_.size()) + " plugin(s).");
}

#ifdef EMSCRIPTEN

static void onFetchListSuccess(emscripten_fetch_t *fetch)
{
    // Retrieve PluginManager pointer from fetch->userData
    auto *manager = reinterpret_cast<PluginManager*>(fetch->userData);
    std::string data((char *)fetch->data, fetch->numBytes);
    emscripten_fetch_close(fetch);

    manager->log("Fetch plugin list success.");
    manager->parsePluginList(data);
}

static void onFetchListFailed(emscripten_fetch_t *fetch)
{
    auto *manager = reinterpret_cast<PluginManager*>(fetch->userData);
    manager->log("Fetch plugin list failed, status=" + std::to_string(fetch->status));
    emscripten_fetch_close(fetch);
}

void PluginManager::fetchPluginList()
{
    log("Fetching plugin list (Emscripten)...");
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = onFetchListSuccess;
    attr.onerror   = onFetchListFailed;
    attr.userData  = this;
    emscripten_fetch(&attr, GetPluginListUrl().c_str());
}

#else  // Native
void PluginManager::fetchPluginList()
{
    log("Fetching plugin list (Native)...");
    CURL* curl = curl_easy_init();
    if (!curl) {
        log("Curl init failed. Cannot fetch plugin list.");
        return;
    }
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, GetPluginListUrl().c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
      +[](char* ptr, size_t size, size_t nmemb, void* userdata)->size_t {
         auto* resp = (std::string*)userdata;
         resp->append(ptr, size * nmemb);
         return size * nmemb;
      });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        log(std::string("Plugin list fetch failed: ") + curl_easy_strerror(res));
        return;
    }

    parsePluginList(response);
}
#endif // EMSCRIPTEN

#ifdef EMSCRIPTEN
// Define a struct to hold the file list.
typedef struct {
    int count;     // Number of files
    char **paths;  // Pointer to an array of C strings
  } FileList;
  
  // EM_JS function that traverses a given IDBFS directory recursively and returns a FileList pointer.
  // It allocates memory on the WASM heap for the FileList struct, the array of char pointers, and each string.
  EM_JS(FileList*, listFilesArray, (const char* path), {
    // Convert the C string to a JavaScript string.
    var pathStr = UTF8ToString(path);
  
    // Recursive directory traversal.
    function traverseDir(dir) {
      var results = [];
      var entries = FS.readdir(dir);
      entries.forEach(function(entry) {
        if (entry === '.' || entry === '..') return;
        var fullPath = dir + '/' + entry;
        try {
          var stat = FS.stat(fullPath);
          if (FS.isDir(stat.mode)) {
            results = results.concat(traverseDir(fullPath));
          } else {
            results.push(fullPath);
          }
        } catch (e) {
          console.error("Error reading " + fullPath + ": " + e);
        }
      });
      return results;
    }
  
    // Get the list of file paths starting from the given path.
    var files = traverseDir(pathStr);
  
    // Allocate memory for the FileList struct (assuming 32-bit pointers, 8 bytes total).
    var structSize = 8;
    var fileListPtr = _malloc(structSize);
  
    // Allocate memory for the array of char* pointers.
    var ptrSize = 4; // size of a pointer in 32-bit WASM
    var arrayPtr = _malloc(files.length * ptrSize);
  
    // For each file, allocate memory for the string and store its pointer.
    for (var i = 0; i < files.length; i++) {
      var fileStr = files[i];
      var len = lengthBytesUTF8(fileStr) + 1;  // +1 for the null terminator
      var strPtr = _malloc(len);
      stringToUTF8(fileStr, strPtr, len);
      // Write the pointer into the array.
      setValue(arrayPtr + i * ptrSize, strPtr, 'i32');
    }
  
    // Write the file count and the pointer to the array into the FileList struct.
    setValue(fileListPtr, files.length, 'i32');         // count at offset 0
    setValue(fileListPtr + 4, arrayPtr, 'i32');           // pointer to array at offset 4
  
    return fileListPtr;
  });
#endif


void PluginManager::loadPreDownloadedPlugins() {
    // scan for downloaded pluings in PLUGIN_DEST and load them
    auto& pluginList = getPluginList();
    printf("Checking for pre-downloaded plugins in %s\n", PLUGIN_DEST);
    for (const auto& entry : std::filesystem::directory_iterator(PLUGIN_DEST)) {
        printf("Found file: %s\n", entry.path().string().c_str());
        if (entry.is_regular_file()) {
            // find the plugin in the plugin list
            auto it = std::find_if(pluginList.begin(), pluginList.end(),
                [&entry](const LoadablePlugin& plugin) {
                    return entry.path().filename() == plugin.name;
                });
            if (it == pluginList.end()) {
                // we have a plugin that is not in the list
                // TODO: validate sha with server if we have connection
                std::string pluginPath = entry.path().string();
                log("Loading pre-downloaded plugin: " + pluginPath);
                loadPluginFromFile(pluginPath);
            } else {
                // we should validate
                it->downloadedPath = entry.path().string();
                loadPlugin(*it);
            }
        }
    }

}

#ifdef EMSCRIPTEN

struct DownloadCtx {
    std::string localPath;
    PluginManager* manager;
    LoadablePlugin* plugin;
};

static void onPluginFetchSuccess(emscripten_fetch_t *fetch)
{
    auto* ctx = reinterpret_cast<DownloadCtx*>(fetch->userData);
    PluginManager* manager = ctx->manager;
    auto& plugin = *ctx->plugin;

    FILE* fp = std::fopen(ctx->localPath.c_str(), "wb");
    if (fp) {
        fwrite(fetch->data, 1, fetch->numBytes, fp);
        fclose(fp);
        plugin.downloadedPath = ctx->localPath;
        manager->loadPlugin(plugin);
    } else {
        manager->log("Could not create file: " + ctx->localPath);
    }

    emscripten_fetch_close(fetch);
    delete ctx;
}

static void onPluginFetchFailed(emscripten_fetch_t *fetch)
{
    auto* ctx = reinterpret_cast<DownloadCtx*>(fetch->userData);
    std::cout << "Plugin fetch failed, status=" + std::to_string(fetch->status) << std::endl;
    delete ctx;
    emscripten_fetch_close(fetch);
}

namespace idb {
static void success(emscripten_fetch_t *fetch) {
    printf("IDB store succeeded.\n");
    emscripten_fetch_close(fetch);
  }
  
  static void failure(emscripten_fetch_t *fetch) {
    printf("IDB store failed.\n");
    emscripten_fetch_close(fetch);
  }
  
  static void persistFileToIndexedDB(const char *outputFilename, uint8_t *data, size_t numBytes) {
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "EM_IDB_STORE");
    attr.attributes = EMSCRIPTEN_FETCH_REPLACE | EMSCRIPTEN_FETCH_PERSIST_FILE;
    attr.requestData = (char *)data;
    attr.requestDataSize = numBytes;
    attr.onsuccess = success;
    attr.onerror = failure;
    emscripten_fetch(&attr, outputFilename);
  }
}

static void fetchPluginSuccess(emscripten_fetch_t *fetch) {
    auto* ctx = reinterpret_cast<DownloadCtx*>(fetch->userData);
  
    FILE *fp = fopen(ctx->localPath.c_str(), "wb");
    if (fp) {
      fwrite(fetch->data, 1, fetch->numBytes, fp);
      fclose(fp);
        idb::persistFileToIndexedDB(ctx->localPath.c_str(), (uint8_t *)fetch->data, fetch->numBytes);
        // sync
        EM_ASM({
            FS.syncfs(false, function(err) {
                assert(!err);
                Module.print("end file sync..");
                Module.syncdone = 1;
            });
        });

        std::cout << "[Web] Plugin fetch success, writing to file: " << ctx->localPath << "\n";
        auto& plugin = *ctx->plugin;
        plugin.downloadedPath = "/plugins/" + plugin.name;
        ctx->manager->loadPlugin(plugin);
    } else {
      std::cerr << "[Web] Could not create file: " << ctx->localPath << "\n";
    }
  
    emscripten_fetch_close(fetch);
    delete ctx;
  }
  
  static void fetchPluginFail(emscripten_fetch_t *fetch) {
    std::cerr << "[Web] Plugin fetch failed, status=" << fetch->status << "\n";
    struct DownloadCtx {
      std::string localPath;
    };
    auto *ctx = (DownloadCtx *)fetch->userData;
    delete ctx;
    emscripten_fetch_close(fetch);
  }

void PluginManager::downloadAndLoadPlugin(LoadablePlugin &plugin)
{
    if (plugin.name.empty()) return;

    auto* ctx = new DownloadCtx();
    ctx->manager = this;

    std::string url = GetPluginBaseUrl() + plugin.name;
    std::string localPath = PLUGIN_DEST + plugin.name;
    std::cout << "Downloading plugin from: " + url + " to " + localPath << std::endl;
    ctx->localPath = localPath;
    ctx->plugin = &plugin;

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    strcpy(attr.requestMethod, "GET");
    attr.onsuccess    = fetchPluginSuccess;
    attr.onerror    = fetchPluginFail;
    attr.userData   = ctx;
    emscripten_fetch(&attr, url.c_str());
}


#else // Native
void PluginManager::downloadAndLoadPlugin(LoadablePlugin &plugin)
{
    if (plugin.name.empty()) return;
    std::string url = GetPluginBaseUrl() + plugin.name;
    std::filesystem::path output_loc = PLUGIN_DEST + plugin.name;
    log("Downloading plugin from: " + url + " to " + output_loc.string());

    CURL* curl = curl_easy_init();
    if (!curl) {
        log("Curl init failed. Cannot download plugin.");
        return;
    }
    std::ofstream out(output_loc, std::ios::binary);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
      +[](char* ptr, size_t size, size_t nmemb, void* userdata)->size_t {
         std::ofstream* fout = (std::ofstream*)userdata;
         fout->write(ptr, size * nmemb);
         return size * nmemb;
      });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    out.close();

    if (res != CURLE_OK) {
        log(std::string("Download failed: ") + curl_easy_strerror(res));
        return;
    }
    plugin.downloadedPath = output_loc.string();
    loadPlugin(plugin);
}
#endif // EMSCRIPTEN

int PluginManager::loadPlugin(LoadablePlugin &plugin)
{
    // validate that the downloaded plugin matches what we expect
    if (plugin.name.empty() || plugin.size == 0 || plugin.sha1.empty()) {
        log("Invalid plugin data.");
        return -1;
    }

    std::string pluginPath = PLUGIN_DEST + plugin.name;
    std::ifstream file(pluginPath, std::ios::binary | std::ios::ate);
    if (!file) {
        log("Could not open plugin file: " + pluginPath);
        return -1;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);

    // read the file so we can check the sha1
    if (!file.read(buffer.data(), size)) {
        log("Could not read plugin file: " + pluginPath);
        return -1;
    }

    file.close();

    sha1::SHA1 sha;
    sha.processBytes(buffer.data(), size);
    unsigned char digest[20];
    sha.getDigestBytes(digest);
    std::string sha1Hash;
    for (size_t i = 0; i < 20; ++i) {
        sha1Hash += std::format("{:02x}", digest[i]);
    }

    if (sha1Hash != plugin.sha1) {
        log("SHA1 mismatch for plugin: " + plugin.name);
        return -1;
    }


    log("SHA1 match for plugin: " + plugin.name);

    int res = loadPluginFromFile(plugin.downloadedPath.string());
    if (res >= 0) {
        plugin.loaded = true;
    }

    return res;
}

int PluginManager::loadPluginFromFile(const std::string &path)
{
    log("Loading plugin file: " + path);

#if defined(_WIN32)
    HMODULE handle = LoadLibraryA(path.c_str());
    if (!handle) {
        DWORD errorCode = GetLastError();
        log(std::string("LoadLibrary error: ") + std::to_string(errorCode));
        return -1;
    }

    using PluginMainFunc = int (*)();
    PluginMainFunc func = reinterpret_cast<PluginMainFunc>(GetProcAddress(handle, "pluginMain"));
    if (!func) {
        DWORD errorCode = GetLastError();
        log(std::string("GetProcAddress error: ") + std::to_string(errorCode));
        FreeLibrary(handle);
        return -1;
    }
#else
    void* handle = dlopen(path.c_str(), RTLD_NOW);
    if (!handle) {
        log(std::string("dlopen error: ") + dlerror());
        return -1;
    }
    using PluginMainFunc = int (*)();
    PluginMainFunc func = reinterpret_cast<PluginMainFunc>(dlsym(handle, "pluginMain"));
    char* error = dlerror();
    if (error != nullptr) {
        log(std::string("dlsym error: ") + error);
        dlclose(handle);
        return -1;
    }
#endif

    int ret = func();
    log(std::string("pluginMain returned: ") + std::to_string(ret));
    pluginHandles_.push_back(handle);
    return ret;
}

void PluginManager::unloadAll()
{
    log("Unloading all plugins...");
    renderables_.clear();
#if defined(_WIN32)
    for (auto handle : pluginHandles_) {
        FreeLibrary(static_cast<HMODULE>(handle));
    }
#else
    for (auto handle : pluginHandles_) {
        dlclose(handle);
    }
#endif
    pluginHandles_.clear();
    pluginList_.clear();
}
