import host
import sys
import traceback
import json

# Log levels
INFO = "INFO"
WARN = "WARN"
ERROR = "ERROR"

def log(msg, level=INFO):
    host.log(level, str(msg))

def on(event_name):
    def decorator(func):
        def wrapper(event, payload):
            try:
                return func(event, payload)
            except Exception as e:
                log(f"Exception in event '{event_name}': {e}", ERROR)
                traceback.print_exc()
        
        host.on(event_name, wrapper)
        return wrapper
    return decorator