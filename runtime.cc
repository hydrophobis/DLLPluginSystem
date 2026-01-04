#include <iostream>
#include <string>
#include <libloaderapi.h>
#include <fstream>
#include <conio.h>

#define PLUGIN_DIR "plugins/"
#define LOAD_LIBRARY(name) LoadLibraryA((PLUGIN_DIR + std::string(name)).c_str())
#define TRACE(msg) std::cout << "[TRACE] " << msg << std::endl;
#define ERROR(msg) std::cerr << "[ERROR] " << msg << std::endl;

#include "plugin_api.h"
#include "ini.h"
#include "ocular.h"

#include <unordered_map>
#include <functional>

class EventBus {
public:
    std::unordered_map<std::string, std::vector<event_callback_t>> listeners;

    void register_event(const char* eventName, event_callback_t cb) {
        listeners[eventName].push_back(cb);
    }

    void send_event(const char* eventName, const char* payload) {
        auto it = listeners.find(eventName);
        if (it != listeners.end()) {
            for (auto& cb : it->second) {
                cb(eventName, payload);
            }
        }
    }
};

EventBus EVENT_BUS;

class Plugin {
public:
    std::string name;
    HMODULE handle;

    plugin_get_info_t getInfo;
    plugin_init_t init;
    plugin_shutdown_t shutdown;

    Plugin(const std::string& pluginName)
        : name(pluginName), handle(nullptr),
          getInfo(nullptr), init(nullptr), shutdown(nullptr) {}

    bool load() {
        AUTOLOG;
        handle = LoadLibraryA((PLUGIN_DIR + name).c_str());
        if (!handle) {
            std::cerr << "Failed to load plugin: " << name << std::endl;
            return false;
        }

        getInfo = (plugin_get_info_t)GetProcAddress(handle, "plugin_get_info");
        init = (plugin_init_t)GetProcAddress(handle, "plugin_init");
        shutdown = (plugin_shutdown_t)GetProcAddress(handle, "plugin_shutdown");

        if (!getInfo || !init || !shutdown) {
            std::cerr << "Plugin missing required exports: " << name << std::endl;
            FreeLibrary(handle);
            return false;
        }

        const PluginInfo* info = getInfo();
        std::cout << "Loaded plugin: " << info->name
                  << " v" << info->version << std::endl;

        if (!init(&host)) {
            std::cerr << "Plugin failed to initialize: " << name << std::endl;
            FreeLibrary(handle);
            return false;
        }

        return true;
    }

    void unload() {
        if (handle) {
            shutdown();
            FreeLibrary(handle);
            std::cout << "Unloaded plugin: " << name << std::endl;
        }
    }

    static void host_send_event(const char* eventName, const char* payload) {
        EVENT_BUS.send_event(eventName, payload);
    }

    static void host_register_event(const char* eventName, event_callback_t cb) {
        EVENT_BUS.register_event(eventName, cb);
    }

    inline static std::vector<Plugin>* g_plugins = nullptr;

    static bool host_load_plugin(const char* name) {
        std::cout << "[Host] Plugin requested load: " << name << std::endl;

        Plugin p(name);
        if (p.load()) {
            g_plugins->push_back(std::move(p));
            return true;
        }
        return false;
    }

    static bool host_unload_plugin(const char* name) {
        std::cout << "[Host] Plugin requested unload: " << name << std::endl;

        for (size_t i = 0; i < g_plugins->size(); i++) {
            if ((*g_plugins)[i].name == name) {
                (*g_plugins)[i].unload();
                g_plugins->erase(g_plugins->begin() + i);
                return true;
            }
        }
        return false;
    }

    PluginHost host = {
        host_send_event,
        host_register_event,
        host_load_plugin,
        host_unload_plugin
    };
};

int main() {
    std::cout << "[Runtime] Starting plugin host..." << std::endl;

    std::vector<Plugin> loadedPlugins;
    Plugin::g_plugins = &loadedPlugins;
    std::vector<std::string> pluginEntries = parse_ini("plugins.ini", "PLUGINS");

    if (pluginEntries.empty()) {
        std::cerr << "[Runtime] No plugins found in plugins.ini" << std::endl;
    }

    // Load plugins
    for (const auto& entry : pluginEntries) {
        size_t eq_pos = entry.find('=');
        if (eq_pos == std::string::npos) {
            std::cerr << "[Runtime] Invalid INI entry: " << entry << std::endl;
            continue;
        }

        std::string pluginPath = entry.substr(eq_pos + 1);
        std::cout << "[Runtime] Loading plugin: " << pluginPath << std::endl;

        Plugin plugin(pluginPath);
        if (plugin.load()) {
            loadedPlugins.push_back(std::move(plugin));
        }
    }

    std::cout << "[Runtime] Entering main loop..." << std::endl;

    bool running = true;

    while (running) {
        // Example periodic event
        EVENT_BUS.send_event("tick", "16ms");

        static std::string inputBuffer;

        if (_kbhit()) {
            int ch = _getch();

            // Enter key = submit command
            if (ch == '\r') {
                if (!inputBuffer.empty()) {
                    EVENT_BUS.send_event("consoleInput", inputBuffer.c_str());
                    inputBuffer.clear();
                }
                std::cout << "\n> "; // prompt
            }
            // Backspace
            else if (ch == '\b') {
                if (!inputBuffer.empty()) {
                    inputBuffer.pop_back();
                    std::cout << "\b \b";
                }
            }
            // Printable characters
            else if (ch >= 32 && ch <= 126) {
                inputBuffer.push_back((char)ch);
                std::cout << (char)ch;
            }
        }


        // ESC to quit
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            std::cout << "[Runtime] ESC pressed, shutting down..." << std::endl;
            running = false;
        }

        Sleep(16);
    }

    // Unload plugins
    for (auto& plugin : loadedPlugins) {
        plugin.unload();
    }

    std::cout << "[Runtime] Exiting." << std::endl;
    return 0;
}