#pragma once
#include <stdint.h>

#define ABI_V1 1

#define PRIORITY_DEFAULT 1
#define PRIORITY_FIRST 0
#define PRIORITY_LATER 2

#define DEP_TYPE_REQUIRED 0
#define DEP_TYPE_OPTIONAL 1

// Fixed expose macro - just adds extern "C" prefix
#ifdef _WIN32
    #define pluginbhvr __cdecl
#else
    #define pluginbhvr
#endif

#ifdef __cplusplus
    #define EXTERN_C extern "C"
#else
    #define EXTERN_C
#endif

#if defined(_WIN32)
    #define PLATFORM_EXPORT __declspec(dllexport)
#else
    #define PLATFORM_EXPORT __attribute__((visibility("default")))
#endif

#define expose EXTERN_C PLATFORM_EXPORT
#define api expose pluginbhvr


extern "C" {

struct Dependency {
    const char* name;
    uint8_t type; // DEP_TYPE_REQUIRED or DEP_TYPE_OPTIONAL
};

struct PluginInfo {
    const char* name;
    const char* version;
    uint32_t abi_version;
    char priority = PRIORITY_DEFAULT;
    Dependency dependencies[128] = {};
};

// Event callback type
typedef void (*event_callback_t)(const char* eventName, const char* payload);

// Logging callback
typedef void (*log_callback_t)(const char* level, const char* message);

// Host interface passed to plugins
struct PluginHost {
    // Event system
    void (*send_event)(const char* eventName, const char* payload);
    void (*register_event)(const char* eventName, event_callback_t callback);
    void (*unregister_event)(event_callback_t callback);
    
    // Plugin management
    bool (*load_plugin)(const char* name);
    bool (*unload_plugin)(const char* name);
    
    // Logging
    void (*log)(const char* level, const char* message);
    
    // Key-value storage for plugins
    bool (*set_data)(const char* key, const char* value);
    const char* (*get_data)(const char* key);
    bool (*has_data)(const char* key);
    bool (*delete_data)(const char* key);
    
    // Timer system
    uint64_t (*set_timer)(uint32_t ms, event_callback_t callback, bool repeat);
    bool (*cancel_timer)(uint64_t timer_id);
};

typedef bool (*plugin_init_t)(PluginHost* host);
typedef void (*plugin_shutdown_t)();
typedef const PluginInfo* (*plugin_get_info_t)();

}

// Helper functions for plugins (use extern host declared in plugin)
namespace plugin {
    extern PluginHost* host;
    
    inline void send(const char* event, const char* payload = "") {
        if (host) host->send_event(event, payload);
    }
    
    inline void log(const char* level, const char* message) {
        if (host) host->log(level, message);
    }
    
    inline void info(const char* message) { log("INFO", message); }
    inline void warn(const char* message) { log("WARN", message); }
    inline void error(const char* message) { log("ERROR", message); }
    
    inline bool store(const char* key, const char* value) {
        return host ? host->set_data(key, value) : false;
    }
    
    inline const char* load(const char* key) {
        return host ? host->get_data(key) : nullptr;
    }
    
    inline uint64_t timer(uint32_t ms, event_callback_t callback, bool repeat = false) {
        return host ? host->set_timer(ms, callback, repeat) : 0;
    }
    
    inline void on(const char* eventName, event_callback_t callback) {
        if (host) host->register_event(eventName, callback);
    }

    inline void off(event_callback_t callback) {
        if (host) host->unregister_event(callback);
    }
}

#define manifest(name, version) \
    expose pluginbhvr const PluginInfo* plugin_get_info() { \
        static PluginInfo info = {name, version, ABI_V1, PRIORITY_DEFAULT}; \
        return &info; \
    }

#define start() \
    namespace plugin { PluginHost* host = nullptr; } \

#define sethost() \
    plugin::host = host;        

#define dependency(dep_name, dep_type) \
    {\
        PluginInfo *info = const_cast<PluginInfo*>(plugin_get_info()); \
        static Dependency dep = {dep_name, dep_type}; \
        for (size_t i = 0; i < 128; i++) { \
            if (info->dependencies[i].name == nullptr || info->dependencies[i].name[0] == '\0') { \
                info->dependencies[i] = dep; \
                break; \
            } \
        } \
    }

#define event_handler(name) \
    static void name(const char* eventName, const char* payload)