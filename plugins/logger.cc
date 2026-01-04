#include "../plugin_api.h"
#include <iostream>
#include <string>

static void logEvent(const char* eventName, const char* payload) {
    std::cout << "[logger] event=" << eventName
              << " payload=" << (payload ? payload : "") << std::endl;
}

EVENT_HANDLER(onKey) {
    logEvent(eventName, payload);
}

EVENT_HANDLER(onHeartbeat) {
    logEvent(eventName, payload);
}

EVENT_HANDLER(onChatMessage) {
    logEvent(eventName, payload);
}

EVENT_HANDLER(onPluginLoaded) {
    logEvent(eventName, payload);
}

BEGIN_PLUGIN("logger", "1.0.0")
    DEPENDENCY("heartbeat.dll", DEP_TYPE_OPTIONAL);
    plugin::on("onKey", onKey);
    plugin::on("heartbeat", onHeartbeat);
    plugin::on("chatMessage", onChatMessage);
    plugin::on("pluginLoaded", onPluginLoaded);
END_PLUGIN()

expose pluginbhvr void plugin_shutdown() {
    plugin::off(onKey);
    plugin::off(onHeartbeat);
    plugin::off(onChatMessage);
    plugin::off(onPluginLoaded);
}