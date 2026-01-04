#include <iostream>
#include <fstream>
#include <string>
#include "../../plugin_api.h"

static PluginInfo INFO = {
    "config",
    "1.0.0",
    ABI_V1
};

static std::string g_message = "default";

extern "C" {

__declspec(dllexport)
const PluginInfo* plugin_get_info() {
    return &INFO;
}

__declspec(dllexport)
bool plugin_init(PluginHost* host) {
    std::cout << "[config] Initialized" << std::endl;

    std::ifstream file("plugins/config/config.txt");
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            auto eq = line.find('=');
            if (eq != std::string::npos) {
                std::string key = line.substr(0, eq);
                std::string value = line.substr(eq + 1);
                if (key == "message") {
                    g_message = value;
                }
            }
        }
    }

    std::string payload = "Config message: " + g_message;
    host->send_event("configLoaded", payload.c_str());

    return true;
}

__declspec(dllexport)
void plugin_shutdown() {
    std::cout << "[config] Shutdown" << std::endl;
}

}