#include <iostream>
#include <string>
#include "../../plugin_api.h"

static PluginInfo INFO = {
    "echo",
    "1.0.0",
    ABI_V1
};

static PluginHost* g_host = nullptr;

static void onChatMessage(const char* eventName, const char* payload) {
    std::cout << "[echo] Received chat: " << (payload ? payload : "") << std::endl;
    if (g_host) {
        std::string reply = std::string("Echo: ") + (payload ? payload : "");
        g_host->send_event("chatReply", reply.c_str());
    }
}

extern "C" {

__declspec(dllexport)
const PluginInfo* plugin_get_info() {
    return &INFO;
}

__declspec(dllexport)
bool plugin_init(PluginHost* host) {
    std::cout << "[echo] Initialized" << std::endl;
    g_host = host;
    host->register_event("chatMessage", onChatMessage);
    return true;
}

__declspec(dllexport)
void plugin_shutdown() {
    std::cout << "[echo] Shutdown" << std::endl;
    g_host = nullptr;
}

}