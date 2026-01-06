# DLLPluginSystem

---

## 1. Project Layout

For the runtime to resolve paths correctly, use this structure:

* **runtime.exe/runtime**: The main runtime.
* **plugins.ini**: Configuration file for loading order.
* **/plugins/**: `.dll` or `.so` files.
* **/plugins/python/**: `.py` scripts. (if using the python plugin)

---

## 2. Plugin Configuration (`plugins.ini`)

The host looks for a `[PLUGINS]` section. Specify the filename only; the runtime assumes they are inside the `plugins/` directory. I include in mine a metadata section but this is not used currently

```ini
[PLUGINS]
Console=console.dll
Python=python.dll
```
---

## 3. C++ Plugin Development

Include `plugin_api.h` in your project. This header defines the `PluginHost` interface and helper macros.

### Compilation

I use clang version 20.1.0 with the target x86_64-pc-windows-msvc to compile C++ plugins. the command is just `clang++.exe <plugin file> -o <output name>.dll -shared -std=c++20` (linux: `clang++ -fPIC -shared -o lib<output name>.so <plugin file> -std=c++20`)

If you are compiling the Python plugin you need to add an include path and link it something like this if you use [this](https://www.python.org/downloads/release/python-3142/) python download page and scroll down to the bottom for your appropriate version it should install to `%USERPROFILE%\AppData\Local\Python\pythoncore-3.14-64` although the final dir may need to be changed depending on various factors. You will probably also need to put that directory in your PATH<br>**Command:**<br>
`clang++.exe <plugin file> -o <output name>.dll -shared -I"$env:USERPROFILE\AppData\Local\Python\pythoncore-3.14-64\include" -L"$env:USERPROFILE\AppData\Local\Python\pythoncore-3.14-64\libs" -lpython314 -std=c++20`

### Metadata & Initialization

Every plugin must define its identity and initialize the host in order to create and subscribe to events.

```cpp
#include "plugin_api.h"

manifest("MyPlugin", "1.0.0");

api bool plugin_init(PluginHost* host){
    sethost(host);

    // Register dependencies if needed
    dependency("logger.dll", DEP_TYPE_REQUIRED)
    dependency("python.dll", DEP_TYPE_OPTIONAL)

    return true; // RETURN TYPES CANNOT BE INFERRED
}

api void plugin_shutdown(){
    return;
}

```
You are also not actually forced to use the macros. If you need custom behaviour for plugin info, staring, or stopping, it can be defined like so
```cpp
#include "plugin_api.h"
// "expose" is defined in plugin_api.h and it is just extern "C"
// "pluginbhvr" is the same but for __declspec(dllexport)
extern "C" __declspec(dllexport) const PluginInfo* plugin_get_info() {
    static PluginInfo info = {name, version, ABI_V1, PRIORITY_DEFAULT};
    return &info;
}
extern "C" __declspec(dllexport) bool plugin_init(PluginHost* host) {
    plugin::g_host = host;
}
extern "C" __declspec(dllexport) void plugin_shutdown() {
    // this example requires no shutdown actions but the method def is still required
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

The `python.dll` embeds a Python interpreter. Scripts in `/plugins/python/` can use the supplied api.py file.

**Example Script (`script.py`):**

```python
import api

@api.on("tick")
def handle_tick(name, payload):
    # 'payload' contains "16ms"
    pass

api.log("INFO", "Python script active.")

```
---
## 6. Planned language supports
(in order of when i plan to do them)
1. JavaScript
2. Lua
3. Java(?)
---

## 7. Technical Constraints

1. **String Ownership**: The `const char*` pointers passed in events are owned by the caller. **Do not** store these pointers. If you need the data later, copy it to a `std::string`.
2. **Threading**: The current runtime is single-threaded. Event handlers should not perform "blocking" work (like `Sleep()`), or they will freeze the entire host loop.
