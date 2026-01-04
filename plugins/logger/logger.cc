#include <iostream>
#include "../../plugin_api.h"

static PluginInfo INFO = {
    "LoggerPlugin",
    "1.0.0",
    ABI_V1
};

static void onAnyEvent(const char* eventName, const char* payload) {
    std::cout << "[LoggerPlugin] event=" << eventName
              << " payload=" << (payload ? payload : "") << std::endl;
}

extern "C" {

__declspec(dllexport)
const PluginInfo* plugin_get_info() {
    return &INFO;
}

__declspec(dllexport)
bool plugin_init(PluginHost* host) {
    std::cout << "[LoggerPlugin] Initialized" << std::endl;
    host->register_event("onKey", onAnyEvent);
    host->register_event("heartbeat", onAnyEvent);
    host->register_event("chatMessage", onAnyEvent);
    host->register_event("pluginLoaded", onAnyEvent);
    return true;
}

__declspec(dllexport)
void plugin_shutdown() {
    std::cout << "[LoggerPlugin] Shutdown" << std::endl;
}

}