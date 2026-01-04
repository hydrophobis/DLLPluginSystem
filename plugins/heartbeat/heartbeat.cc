#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include "../../plugin_api.h"

static PluginInfo INFO = {
    "heartbeat",
    "1.0.0",
    ABI_V1
};

static std::atomic<bool> running{false};
static PluginHost* g_host = nullptr;
static std::thread heartbeatThread;

static void heartbeat_loop() {
    while (running.load()) {
        if (g_host) {
            g_host->send_event("heartbeat", "1s");
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

extern "C" {

__declspec(dllexport)
const PluginInfo* plugin_get_info() {
    return &INFO;
}

__declspec(dllexport)
bool plugin_init(PluginHost* host) {
    std::cout << "[heartbeat] Initialized" << std::endl;
    g_host = host;
    running.store(true);
    heartbeatThread = std::thread(heartbeat_loop);
    return true;
}

__declspec(dllexport)
void plugin_shutdown() {
    std::cout << "[heartbeat] Shutdown" << std::endl;
    running.store(false);
    if (heartbeatThread.joinable())
        heartbeatThread.join();
    g_host = nullptr;
}

}