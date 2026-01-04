import host

host.log("INFO", "Echo plugin loaded.")

def on_input(event, payload):
    host.log("INFO", f"Input received: {payload}")

host.on("consoleInput", on_input)