import host

host.log("INFO", "Python plugin loaded.")

def on_input(event, payload):
    host.log("INFO", f"Input received: {payload}")

host.on("consoleInput", on_input)