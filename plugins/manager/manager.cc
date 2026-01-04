#include <iostream>
#include <string>
#include "../../plugin_api.h"

static PluginInfo INFO = {
    "manager",
    "1.0.0",
    ABI_V1
};

static PluginHost* g_host = nullptr;

static void onLoadEvent(const char* eventName, const char* payload) {
    std::cout << "[manager] Loading plugin: " << payload << std::endl;
    if (!g_host->load_plugin(payload)) {
        std::cerr << "[manager] Failed to load plugin: " << payload << std::endl;
    }
}

static void onUnloadEvent(const char* eventName, const char* payload) {
    std::cout << "[manager] Unloading plugin: " << payload << std::endl;
    if (!g_host->unload_plugin(payload)) {
        std::cerr << "[manager] Failed to unload plugin: " << payload << std::endl;
    }
}

extern "C" {

__declspec(dllexport)
const PluginInfo* plugin_get_info() {
    return &INFO;
}

__declspec(dllexport)
bool plugin_init(PluginHost* host) {
    std::cout << "[manager] Initialized" << std::endl;
    g_host = host;

    host->register_event("loadPlugin", onLoadEvent);
    host->register_event("unloadPlugin", onUnloadEvent);

    return true;
}

__declspec(dllexport)
void plugin_shutdown() {
    std::cout << "[manager] Shutdown" << std::endl;
    g_host = nullptr;
}

}