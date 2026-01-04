#include "../plugin_api.h"

static uint64_t heartbeat_timer = 0;

EVENT_HANDLER(onHeartbeatTimer) {
    if (plugin::g_host) {
        plugin::g_host->send_event("heartbeat", "1s");
    }
}

BEGIN_PLUGIN("heartbeat", "1.0.0")
    // Set up repeating 1-second timer
    heartbeat_timer = plugin::timer(1000, onHeartbeatTimer, true);
END_PLUGIN_SHUTDOWN()