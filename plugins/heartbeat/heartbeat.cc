#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include "../../plugin_api.h"

static PluginInfo INFO = {
    "heartbeat",
    "1.0.0",
    ABI_V1
};

static std::atomic<bool> running{false};
static PluginHost* g_host = nullptr;
static std::thread heartbeatThread;
static std::mutex hostMutex;

static void heartbeat_loop() {
    while (running.load()) {

        PluginHost* hostCopy = nullptr;

        {
            std::lock_guard<std::mutex> lock(hostMutex);
            hostCopy = g_host;
        }

        if (hostCopy) {
            hostCopy->send_event("heartbeat", "1s");
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

    {
        std::lock_guard<std::mutex> lock(hostMutex);
        g_host = host;
    }

    running.store(true);
    heartbeatThread = std::thread(heartbeat_loop);
    return true;
}

__declspec(dllexport)
void plugin_shutdown() {
    std::cout << "[heartbeat] Shutdown" << std::endl;

    // Remove host pointer first so thread can't call into unloaded code
    {
        std::lock_guard<std::mutex> lock(hostMutex);
        g_host = nullptr;
    }

    running.store(false);

    if (heartbeatThread.joinable())
        heartbeatThread.join();
}

}