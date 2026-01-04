#include "../plugin_api.h"
#include <iostream>
#include <sstream>
#include <string>

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
            plugin::warn("Usage: load <plugin.dll>");
            return;
        }

        plugin::info(std::string("Loading plugin: ").append(pluginName).c_str());
        plugin::g_host->load_plugin(pluginName.c_str());
        return;
    }

    if (token == "unload") {
        std::string pluginName;
        iss >> pluginName;

        if (pluginName.empty()) {
            plugin::warn("Usage: unload <plugin.dll>");
            return;
        }

        plugin::info(std::string("Unloading plugin: ").append(pluginName).c_str());
        plugin::g_host->unload_plugin(pluginName.c_str());
        return;
    }

    if (token == "list") {
        plugin::send("requestPluginList", "");
        return;
    }

    plugin::warn(std::string("Unknown command: ").append(token).c_str());
}

EVENT_HANDLER(onConsoleInput) {
    if (!payload) return;
    handleCommand(payload);
}

BEGIN_PLUGIN("console", "1.0.0")
    plugin::on("consoleInput", onConsoleInput);
END_PLUGIN()

expose pluginbhvr void plugin_shutdown() {
    plugin::off(onConsoleInput);
}