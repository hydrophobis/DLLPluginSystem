import host
import sys
import traceback
import json

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

def send_event(event, payload):
    """Send an event to the host."""
    host.send_event(str(event), str(payload))

def load_plugin(name):
    """Load a plugin by name. Returns True on success."""
    return host.load_plugin(str(name))

def unload_plugin(name):
    """Unload a plugin by name. Returns True on success."""
    return host.unload_plugin(str(name))

def set_data(key, value):
    """Set key-value data in the host."""
    return host.set_data(str(key), str(value))

def get_data(key):
    """Get value for a key from the host, or None."""
    return host.get_data(str(key))

def has_data(key):
    """Check if a key exists in host storage."""
    return host.has_data(str(key))

def delete_data(key):
    """Delete a key from host storage. Returns True if deleted."""
    return host.delete_data(str(key))

def set_timer(ms, callback, repeat=False):
    """Set a timer in milliseconds. Returns the timer ID."""
    return host.set_timer(int(ms), callback, repeat)

def cancel_timer(timer_id):
    """Cancel a timer by its ID. Returns True if canceled."""
    return host.cancel_timer(timer_id)
