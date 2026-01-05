import api
import host
import shlex

@api.on("consoleInput")
def on_console_input(event, payload):
    if not payload:
        return

    try:
        tokens = shlex.split(payload)
    except ValueError:
        api.log("Invalid command syntax", api.WARN)
        return

    if not tokens:
        return

    token = tokens[0]

    if token == "help":
        api.log("[console] Commands:\n"
                "  load <plugin.dll>\n"
                "  unload <plugin.dll>\n"
                "  list\n"
                "  help")
        return

    if token == "load":
        if len(tokens) < 2:
            api.log("Usage: load <plugin.dll>", api.WARN)
            return
        plugin_name = tokens[1]
        api.log(f"Loading plugin: {plugin_name}")
        host.load_plugin(plugin_name)
        return

    if token == "unload":
        if len(tokens) < 2:
            api.log("Usage: unload <plugin.dll>", api.WARN)
            return
        plugin_name = tokens[1]
        api.log(f"Unloading plugin: {plugin_name}")
        host.unload_plugin(plugin_name)
        return

    if token == "list":
        host.send("requestPluginList", "")
        return

    api.log(f"Unknown command: {token}", api.WARN)
