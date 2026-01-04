#pragma once
#include <stdint.h>

#define ABI_V1 1

#define PRIORITY_DEFAULT 1
#define PRIORITY_FIRST 0
#define PRIORITY_LATER 2

extern "C" {

struct PluginInfo {
    const char* name;
    const char* version;
    uint32_t abi_version;
    char priority = PRIORITY_DEFAULT;
};

// Event callback type
typedef void (*event_callback_t)(const char* eventName, const char* payload);

// Host interface passed to plugins
struct PluginHost {
    void (*send_event)(const char* eventName, const char* payload);
    void (*register_event)(const char* eventName, event_callback_t callback);
};

typedef bool (*plugin_init_t)(PluginHost* host);
typedef void (*plugin_shutdown_t)();
typedef const PluginInfo* (*plugin_get_info_t)();

}