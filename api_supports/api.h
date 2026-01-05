#ifndef PLUGIN_API_H
#define PLUGIN_API_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define ABI_V1 1

#define PRIORITY_DEFAULT 1
#define PRIORITY_FIRST 0
#define PRIORITY_LATER 2

#define DEP_TYPE_REQUIRED 0
#define DEP_TYPE_OPTIONAL 1

/* Calling convention */
#ifdef _WIN32
    #define pluginbhvr __cdecl
#else
    #define pluginbhvr
#endif

/* C linkage + export */
#ifdef _WIN32
    #define PLATFORM_EXPORT __declspec(dllexport)
#else
    #define PLATFORM_EXPORT __attribute__((visibility("default")))
#endif

#define expose PLATFORM_EXPORT
#define api expose pluginbhvr

/* ================= ABI ================= */

struct Dependency {
    const char* name;
    uint8_t type;
};

struct PluginInfo {
    const char* name;
    const char* version;
    uint32_t abi_version;
    char priority;
    struct Dependency dependencies[128];
};

/* Callback types */
typedef void (*event_callback_t)(const char* eventName, const char* payload);
typedef void (*log_callback_t)(const char* level, const char* message);

/* Host interface */
struct PluginHost {
    void (*send_event)(const char* eventName, const char* payload);
    void (*register_event)(const char* eventName, event_callback_t callback);
    void (*unregister_event)(event_callback_t callback);

    bool (*load_plugin)(const char* name);
    bool (*unload_plugin)(const char* name);

    void (*log)(const char* level, const char* message);

    bool (*set_data)(const char* key, const char* value);
    const char* (*get_data)(const char* key);
    bool (*has_data)(const char* key);
    bool (*delete_data)(const char* key);

    uint64_t (*set_timer)(uint32_t ms, event_callback_t callback, bool repeat);
    bool (*cancel_timer)(uint64_t timer_id);
};

/* Entry point types */
typedef bool (*plugin_init_t)(struct PluginHost* host);
typedef void (*plugin_shutdown_t)(void);
typedef const struct PluginInfo* (*plugin_get_info_t)(void);

/* ================= Plugin Helpers ================= */

extern struct PluginHost* plugin_host;

static inline void plugin_send(const char* event, const char* payload)
{
    if (plugin_host) plugin_host->send_event(event, payload);
}

static inline void plugin_log(const char* level, const char* message)
{
    if (plugin_host) plugin_host->log(level, message);
}

static inline void plugin_info(const char* message) { plugin_log("INFO", message); }
static inline void plugin_warn(const char* message) { plugin_log("WARN", message); }
static inline void plugin_error(const char* message) { plugin_log("ERROR", message); }

static inline bool plugin_store(const char* key, const char* value)
{
    return plugin_host ? plugin_host->set_data(key, value) : false;
}

static inline const char* plugin_load(const char* key)
{
    return plugin_host ? plugin_host->get_data(key) : NULL;
}

static inline uint64_t plugin_timer(uint32_t ms, event_callback_t callback, bool repeat)
{
    return plugin_host ? plugin_host->set_timer(ms, callback, repeat) : 0;
}

static inline void plugin_on(const char* eventName, event_callback_t callback)
{
    if (plugin_host) plugin_host->register_event(eventName, callback);
}

static inline void plugin_off(event_callback_t callback)
{
    if (plugin_host) plugin_host->unregister_event(callback);
}

/* ================= Macros ================= */

#define manifest(name, version)                                  \
    api const struct PluginInfo* plugin_get_info(void) {        \
        static struct PluginInfo info = {                       \
            name, version, ABI_V1, PRIORITY_DEFAULT, {0}        \
        };                                                       \
        return &info;                                           \
    }

#define start() \
    struct PluginHost* plugin_host = NULL;

#define sethost() \
    plugin_host = host;

#define dependency(dep_name, dep_type)                          \
    do {                                                        \
        struct PluginInfo* info =                               \
            (struct PluginInfo*)plugin_get_info();             \
        struct Dependency dep = { dep_name, dep_type };        \
        for (size_t i = 0; i < 128; ++i) {                     \
            if (!info->dependencies[i].name ||                 \
                info->dependencies[i].name[0] == '\0') {      \
                info->dependencies[i] = dep;                   \
                break;                                         \
            }                                                   \
        }                                                       \
    } while (0)

#define event_handler(name) \
    static void name(const char* eventName, const char* payload)

#endif /* PLUGIN_API_H */
