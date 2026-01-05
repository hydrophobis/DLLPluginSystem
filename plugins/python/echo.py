import api

@api.on("consoleInput")
def on_input(event, payload):
    api.log(f"Input received: {payload} added", api.INFO)