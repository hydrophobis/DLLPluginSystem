#include <iostream>
#include <sstream>
#include <string>
#include "../../plugin_api.h"

static PluginInfo INFO = {
    "console",
    "1.0.0",
    ABI_V1,
    PRIORITY_LATER
};

static PluginHost* g_host = nullptr;

static void handleCommand(const std::string& cmd) {
    std::istringstream iss(cmd);
    std::string token;
    iss >> token;

    if (token == "help") {
        std::cout << "[console] Commands:\n"
                  << "  load <plugin.dll>\n"
                  << "  unload <plugin.dll>\n"
                  << "  list\n"
                  << "  help\n";
        return;
    }

    if (token == "load") {
        std::string pluginName;
        iss >> pluginName;

        if (pluginName.empty()) {
            std::cout << "[console] Usage: load <plugin.dll>\n";
            return;
        }

        std::cout << "[console] Loading plugin: " << pluginName << std::endl;
        g_host->load_plugin(pluginName.c_str());
        return;
    }

    if (token == "unload") {
        std::string pluginName;
        iss >> pluginName;

        if (pluginName.empty()) {
            std::cout << "[console] Usage: unload <plugin.dll>\n";
            return;
        }

        std::cout << "[console] Unloading plugin: " << pluginName << std::endl;
        g_host->unload_plugin(pluginName.c_str());
        return;
    }

    if (token == "list") {
        g_host->send_event("requestPluginList", "");
        return;
    }

    std::cout << "[console] Unknown command: " << token << std::endl;
}

static void onConsoleInput(const char* eventName, const char* payload) {
    if (!payload) return;
    handleCommand(payload);
}

extern "C" {

__declspec(dllexport)
const PluginInfo* plugin_get_info() {
    return &INFO;
}

__declspec(dllexport)
bool plugin_init(PluginHost* host) {
    std::cout << "[console] CommandConsolePlugin initialized\n";
    g_host = host;

    host->register_event("consoleInput", onConsoleInput);
    return true;
}

__declspec(dllexport)
void plugin_shutdown() {
    std::cout << "[console] CommandConsolePlugin shutdown\n";
    g_host = nullptr;
}

}