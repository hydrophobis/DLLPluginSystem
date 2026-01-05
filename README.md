# DLLPluginSystem

---

## 1. Project Layout

For the runtime to resolve paths correctly, use this structure:

* **Host.exe**: The main runtime.
* **plugins.ini**: Configuration file for loading order.
* **/plugins/**: C++ `.dll` files.
* **/plugins/python/**: `.py` scripts + `python.ini`.

---

## 2. Plugin Configuration (`plugins.ini`)

The host looks for a `[PLUGINS]` section. Specify the filename only; the runtime assumes they are inside the `plugins/` directory.

```ini
[PLUGINS]
Console=console.dll
Python=python.dll

```

---

I've reorganized the documentation so that it starts with the standard initialization and concludes with the more advanced shutdown logic. I also took the liberty of fixing a small syntax error in your shutdown example (adding a missing semicolon) to keep the code snippet clean.

---

## 3. C++ Plugin Development

Include `plugin_api.h` in your project. This header defines the `PluginHost` interface and helper macros.

### Metadata & Initialization

Every plugin must define its identity and initialize the host pointer. For basic plugins that don't require manual memory management, use the standard registration macros:

```cpp
#include "plugin_api.h"

BEGIN_PLUGIN("MyPlugin", "1.0.0")
    // Register dependencies if needed
    DEPENDENCY("logger.dll", DEP_TYPE_REQUIRED)
    DEPENDENCY("logger.dll", DEP_TYPE_OPTIONAL)

// Use this macro if your plugin doesn't require custom cleanup logic
END_PLUGIN_SHUTDOWN() 

```

### Advanced: Handling Manual Shutdown

If your plugin allocates memory (e.g., `new`, `malloc`), opens file handles, or registers events that must be explicitly detached, you should use `END_PLUGIN()` and provide your own `plugin_shutdown` logic.

This ensures that all resources are released before the DLL is unloaded from memory.

```cpp
#include "plugin_api.h"

BEGIN_PLUGIN("StorageModule", "1.0.0")
    // Initialization logic here
    plugin::on("consoleInput", OnConsoleInput);
END_PLUGIN()

// This function is called by the host during the unloading process
expose pluginbhvr void plugin_shutdown() {
    // Unregister listeners
    plugin::off(OnConsoleInput);
    
    // Perform any manual memory cleanup or close file handles here
    plugin::info("StorageModule shutting down safely.");
}

```

---

## 4. API Reference (`plugin::` namespace)

The header provides inline helpers to simplify calls to the `PluginHost` table.

| Helper | Description |
| --- | --- |
| `plugin::info(msg)` | Logs a message at the "INFO" level. |
| `plugin::on(event, cb)` | Subscribes to a specific event string. |
| `plugin::off(cb)` | Unsubscribes a function pointer from all events. |
| `plugin::send(event, payload)` | Broadcasts an event to all other plugins. |
| `plugin::store(key, val)` | Saves a string to the host's global data map. |
| `plugin::load(key)` | Retrieves a string from global storage. |
| `plugin::timer(ms, cb, rep)` | Starts a timer (one-shot or repeating). |

---

## 5. Python Bridge

The `python.cc` embeds a Python interpreter. Scripts in `/plugins/python/` have access to a built-in `host` module.

**python.ini requirements:**

```ini
[PYTHON]
HOME=C:/Path/To/PythonInstallation # Must contain python3x.dll

```

**Example Script (`script.py`):**

```python
import host

def handle_tick(name, payload):
    # 'payload' contains "16ms"
    pass

host.on("tick", handle_tick)
host.log("INFO", "Python script active.")

```

---

## 6. Technical Constraints

1. **String Ownership**: The `const char*` pointers passed in events are owned by the caller. **Do not** store these pointers. If you need the data later, copy it to a `std::string`.
2. **Threading**: The current runtime is single-threaded. Event handlers should not perform "blocking" work (like `Sleep()`), or they will freeze the entire host loop. (i plan to fix this soon)
