#include <iostream>
#include <string>
#include <libloaderapi.h>
#include <fstream>
#include <conio.h>
#include <chrono>
#include <unordered_map>
#include <functional>
#include <vector>
#include <algorithm>

#define PLUGIN_DIR "plugins/"
#define TRACE(msg) std::cout << "[TRACE] " << msg << std::endl;
#define ERROR(msg) std::cerr << "[ERROR] " << msg << std::endl;

#include "plugin_api.h"
#include "ini.h"
#include "ocular.h"

class EventBus {
public:
    std::unordered_map<std::string, std::vector<event_callback_t>> listeners;

    void register_event(const char* eventName, event_callback_t cb) {
        listeners[eventName].push_back(cb);
    }

    // Add this method
    void unregister_all_by_callback(event_callback_t cb) {
        for (auto& pair : listeners) {
            auto& vec = pair.second;
            vec.erase(std::remove(vec.begin(), vec.end(), cb), vec.end());
        }
    }

    void send_event(const char* eventName, const char* payload) {
        auto it = listeners.find(eventName);
        if (it != listeners.end()) {
            for (auto& cb : it->second) {
                cb(eventName, payload); // <--- Crash happens here without cleanup
            }
        }
    }
};

EventBus EVENT_BUS;

class Storage {
public:
    std::unordered_map<std::string, std::string> data;
    
    bool set(const char* key, const char* value) {
        data[key] = value;
        return true;
    }
    
    const char* get(const char* key) {
        auto it = data.find(key);
        if (it != data.end()) {
            return it->second.c_str();
        }
        return nullptr;
    }
    
    bool has(const char* key) {
        return data.find(key) != data.end();
    }
    
    bool remove(const char* key) {
        return data.erase(key) > 0;
    }
};

Storage STORAGE;

struct Timer {
    uint64_t id;
    uint32_t interval_ms;
    event_callback_t callback;
    bool repeat;
    std::chrono::steady_clock::time_point next_fire;
    bool active;
};

class TimerManager {
public:
    std::vector<Timer> timers;
    uint64_t next_id = 1;
    
    uint64_t add_timer(uint32_t ms, event_callback_t callback, bool repeat) {
        Timer t;
        t.id = next_id++;
        t.interval_ms = ms;
        t.callback = callback;
        t.repeat = repeat;
        t.next_fire = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
        t.active = true;
        timers.push_back(t);
        return t.id;
    }
    
    bool cancel_timer(uint64_t id) {
        for (auto& t : timers) {
            if (t.id == id) {
                t.active = false;
                return true;
            }
        }
        return false;
    }
    
    void update() {
        auto now = std::chrono::steady_clock::now();
        
        for (auto& t : timers) {
            if (!t.active) continue;
            
            if (now >= t.next_fire) {
                t.callback("timer", "");
                
                if (t.repeat) {
                    t.next_fire = now + std::chrono::milliseconds(t.interval_ms);
                } else {
                    t.active = false;
                }
            }
        }
        
        // Remove inactive timers
        timers.erase(
            std::remove_if(timers.begin(), timers.end(), 
                [](const Timer& t) { return !t.active; }),
            timers.end()
        );
    }
};

TimerManager TIMER_MANAGER;

void host_log(const char* level, const char* message) {
    std::cout << "[" << level << "] " << message << std::endl;
}

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

    // Host callback implementations
    static void __cdecl host_send_event(const char* eventName, const char* payload) {
        EVENT_BUS.send_event(eventName, payload);
    }

    static void __cdecl host_register_event(const char* eventName, event_callback_t cb) {
        EVENT_BUS.register_event(eventName, cb);
    }

    static void __cdecl host_unregister_event(event_callback_t cb) {
        EVENT_BUS.unregister_all_by_callback(cb);
    }

    static bool __cdecl host_set_data(const char* key, const char* value) {
        return STORAGE.set(key, value);
    }

    static const char* __cdecl host_get_data(const char* key) {
        return STORAGE.get(key);
    }

    static bool __cdecl host_has_data(const char* key) {
        return STORAGE.has(key);
    }

    static bool __cdecl host_delete_data(const char* key) {
        return STORAGE.remove(key);
    }

    static uint64_t __cdecl host_set_timer(uint32_t ms, event_callback_t callback, bool repeat) {
        return TIMER_MANAGER.add_timer(ms, callback, repeat);
    }

    static bool __cdecl host_cancel_timer(uint64_t timer_id) {
        return TIMER_MANAGER.cancel_timer(timer_id);
    }

    inline static std::vector<Plugin>* g_plugins = nullptr;

    static bool __cdecl host_load_plugin(const char* name) {
        std::cout << "[Host] Plugin requested load: " << name << std::endl;

        Plugin p(name);
        if (p.load()) {
            g_plugins->push_back(std::move(p));
            return true;
        }
        return false;
    }

    static bool __cdecl host_unload_plugin(const char* name) {
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
        host_unregister_event,
        host_load_plugin,
        host_unload_plugin,
        host_log,
        host_set_data,
        host_get_data,
        host_has_data,
        host_delete_data,
        host_set_timer,
        host_cancel_timer
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

        if (!plugin.load()) {
            std::cerr << "[Runtime] Failed to load plugin: " << pluginPath << std::endl;
            continue;
        }

        const PluginInfo* info = plugin.getInfo();

        for (const auto& dep : info->dependencies) {
            if (dep.type == DEP_TYPE_OPTIONAL) break;
            if (!dep.name || dep.name[0] == '\0') break;

            std::cout << "[Runtime] Checking dependency: " << dep.name << std::endl;

            Plugin depPlugin(dep.name);
            if (!depPlugin.load()) {
                std::cerr << "[Runtime] Failed to load dependency: " << dep.name << std::endl;
                continue;
            }

            loadedPlugins.push_back(std::move(depPlugin));
        }

        loadedPlugins.push_back(std::move(plugin));
    }

    std::cout << "[Runtime] Entering main loop..." << std::endl;
    std::cout << "> ";

    bool running = true;
    std::string inputBuffer;

    while (running) {
        // Update timers
        TIMER_MANAGER.update();
        
        // Send periodic tick event
        EVENT_BUS.send_event("tick", "16ms");

        // Handle keyboard input
        if (_kbhit()) {
            int ch = _getch();

            if (ch == '\r') {
                if (!inputBuffer.empty()) {
                    std::cout << std::endl;
                    EVENT_BUS.send_event("consoleInput", inputBuffer.c_str());
                    inputBuffer.clear();
                }
                std::cout << "> ";
            }
            else if (ch == '\b') {
                if (!inputBuffer.empty()) {
                    inputBuffer.pop_back();
                    std::cout << "\b \b";
                }
            }
            else if (ch >= 32 && ch <= 126) {
                inputBuffer.push_back((char)ch);
                std::cout << (char)ch;
            }
        }

        // ESC to quit
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            std::cout << "\n[Runtime] ESC pressed, shutting down..." << std::endl;
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