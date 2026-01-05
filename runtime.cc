#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <unordered_map>
#include <functional>
#include <vector>
#include <algorithm>

// Define plugin directory based on OS
#ifdef _WIN32
    #define PLUGIN_DIR "plugins\\"
#else
    #define PLUGIN_DIR "./plugins/"
#endif

#define TRACE(msg) std::cout << "[TRACE] " << msg << std::endl;
#define ERROR(msg) std::cerr << "[ERROR] " << msg << std::endl;

#include "plugin_api.h"
#include "ini.h"
#include "ABI_compat_layer.h" // New compatibility layer

// --- [Previous EventBus, Storage, and Timer Classes remain identical] ---
// (They are standard C++ and do not need changes)

class EventBus {
public:
    std::unordered_map<std::string, std::vector<event_callback_t>> listeners;

    void register_event(const char* eventName, event_callback_t cb) {
        listeners[eventName].push_back(cb);
    }

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
                cb(eventName, payload);
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

// --- [Plugin Class Refactored for ABI Layer] ---

class Plugin {
public:
    std::string name;
    PluginHandle handle; // Changed from HMODULE

    plugin_get_info_t getInfo;
    plugin_init_t init;
    plugin_shutdown_t shutdown;

    Plugin(const std::string& pluginName)
        : name(pluginName), handle(nullptr),
          getInfo(nullptr), init(nullptr), shutdown(nullptr) {}

    bool load() {
        std::string fullPath = PLUGIN_DIR + name;
        
        // Linux specific: dlopen usually requires ./ for local files if not in LD_PATH
        // The PLUGIN_DIR definition handles this, but user must ensure .so extension in INI or here
        
        handle = PLATFORM_LOAD_LIB(fullPath.c_str());
        
        if (!handle) {
            // Optional: Print dlerror() on Linux for debugging
            #ifndef _WIN32
            const char* err = dlerror();
            if(err) std::cerr << "dlopen error: " << err << std::endl;
            #endif
            
            std::cerr << "Failed to load plugin: " << name << std::endl;
            return false;
        }

        // Use PLATFORM_GET_PROC macro
        getInfo = (plugin_get_info_t)PLATFORM_GET_PROC(handle, "plugin_get_info");
        init = (plugin_init_t)PLATFORM_GET_PROC(handle, "plugin_init");
        shutdown = (plugin_shutdown_t)PLATFORM_GET_PROC(handle, "plugin_shutdown");

        if (!getInfo || !init || !shutdown) {
            std::cerr << "Plugin missing required exports: " << name << std::endl;
            PLATFORM_FREE_LIB(handle);
            return false;
        }

        const PluginInfo* info = getInfo();
        std::cout << "Loaded plugin: " << info->name
                  << " v" << info->version << std::endl;

        if (!init(&host)) {
            std::cerr << "Plugin failed to initialize: " << name << std::endl;
            PLATFORM_FREE_LIB(handle);
            return false;
        }

        return true;
    }

    void unload() {
        if (handle) {
            shutdown();
            PLATFORM_FREE_LIB(handle);
            std::cout << "Unloaded plugin: " << name << std::endl;
        }
    }

    // Host callback implementations (Identical)
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
    std::cout << "[Runtime] Starting plugin host (" << WINLIN("Windows", "Linux") << ")..." << std::endl;

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

    std::cout << "[Runtime] Entering main loop (Press ESC to quit)..." << std::endl;
    std::cout << "> ";
    std::cout.flush(); // Ensure prompt is visible

    bool running = true;
    std::string inputBuffer;

    while (running) {
        TIMER_MANAGER.update();
        
        EVENT_BUS.send_event("tick", "16ms");

        // Cross-platform input handling using ABI layer
        if (platform_kbhit()) {
            int ch = platform_getch();

            if (ch == 27) { // ESC key (Standard ASCII)
                 std::cout << "\n[Runtime] ESC pressed, shutting down..." << std::endl;
                 running = false;
            }
            else if (ch == '\r' || ch == '\n') { // Enter
                if (!inputBuffer.empty()) {
                    std::cout << std::endl;
                    EVENT_BUS.send_event("consoleInput", inputBuffer.c_str());
                    inputBuffer.clear();
                }
                std::cout << "> ";
                std::cout.flush();
            }
            else if (ch == '\b' || ch == 127) { // Backspace (127 is common on Linux)
                if (!inputBuffer.empty()) {
                    inputBuffer.pop_back();
                    std::cout << "\b \b";
                    std::cout.flush();
                }
            }
            else if (ch >= 32 && ch <= 126) {
                inputBuffer.push_back((char)ch);
                std::cout << (char)ch;
                std::cout.flush();
            }
        }

        PLATFORM_SLEEP_MS(16);
    }

    for (auto& plugin : loadedPlugins) {
        plugin.unload();
    }

    std::cout << "[Runtime] Exiting." << std::endl;
    return 0;
}