#include "../plugin_api.h"
#include <iostream>
#include <sstream>
#include <string>

start();

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
        plugin::host->load_plugin(pluginName.c_str());
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
        plugin::host->unload_plugin(pluginName.c_str());
        return;
    }

    if (token == "list") {
        plugin::send("requestPluginList", "");
        return;
    }

    plugin::warn(std::string("Unknown command: ").append(token).c_str());
}

event_handler(onConsoleInput) {
    if (!payload) return;
    handleCommand(payload);
}

manifest("console", "1.0.0")

api bool plugin_init(PluginHost* host){
    sethost();
    plugin::on("consoleInput", onConsoleInput);
    return true;
}

api void plugin_shutdown() {
    plugin::off(onConsoleInput);
}