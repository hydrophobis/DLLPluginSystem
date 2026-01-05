#include "../plugin_api.h"
#include "../ini.h"
#include <Python.h>
#include <string>
#include <map>
#include <filesystem>
#include <iostream>
#include <algorithm>

static PluginHost* host = nullptr;
static std::multimap<std::string, PyObject*> python_event_listeners;

manifest("python", "1.0.0")
start();

struct ScriptState {
    std::filesystem::file_time_type last_write;
    std::string module_name;
};
static std::map<std::string, ScriptState> watched_scripts;

void check_hot_reload() {
    namespace fs = std::filesystem;
    // Iterate over watched files
    for (auto& [path, state] : watched_scripts) {
        if (!fs::exists(path)) continue;

        auto current_time = fs::last_write_time(path);
        if (current_time > state.last_write) {
            state.last_write = current_time;
            plugin::host->log("INFO", ("Reloading: " + state.module_name).c_str());

            std::string reloadCmd = 
                "import sys, importlib\n"
                "if '" + state.module_name + "' in sys.modules:\n"
                "    importlib.reload(sys.modules['" + state.module_name + "'])\n";
            
            // Note: In a real scenario, you must deregister old events first!
            // This simple version assumes your python scripts are smart enough 
            // not to duplicate listeners, or you clear listeners on reload.
            PyGILState_STATE gstate = PyGILState_Ensure();
            PyRun_SimpleString(reloadCmd.c_str());
            PyGILState_Release(gstate);
        }
    }
}

// Proxy for events
expose void python_event_proxy(const char* eventName, const char* payload) {
    // This ensures the C++ thread safely interacts with the Python interpreter
    PyGILState_STATE gstate = PyGILState_Ensure();

    auto range = python_event_listeners.equal_range(eventName);
    for (auto it = range.first; it != range.second; ++it) {
        PyObject* func = it->second;
        if (func && PyCallable_Check(func)) {
            PyObject* args = Py_BuildValue("(ss)", eventName, payload);
            PyObject* result = PyObject_CallObject(func, args);

            if (result == NULL) {
                fprintf(stderr, "[Python Error in %s]:\n", eventName);
                PyErr_Print(); // This prints the traceback to your console
            } else {
                Py_DECREF(result);
            }
            Py_DECREF(args);
        }
    }

    PyGILState_Release(gstate);
}

static PyObject* py_log(PyObject* self, PyObject* args) {
    const char *lvl, *msg;
    if (!PyArg_ParseTuple(args, "ss", &lvl, &msg)) return NULL;
    if (plugin::host) plugin::host->log(lvl, msg);
    Py_RETURN_NONE;
}

static PyObject* py_on(PyObject* self, PyObject* args) {
    const char* event;
    PyObject* callback;
    if (!PyArg_ParseTuple(args, "sO", &event, &callback)) return NULL;

    if (python_event_listeners.find(event) == python_event_listeners.end()) {
        if (plugin::host) plugin::host->register_event(event, python_event_proxy);
    }

    Py_INCREF(callback);
    python_event_listeners.insert({event, callback});
    Py_RETURN_NONE;
}

static PyObject* py_send_event(PyObject* self, PyObject* args) {
    const char* event;
    const char* payload;
    if (!PyArg_ParseTuple(args, "ss", &event, &payload)) return NULL;
    if (plugin::host && plugin::host->send_event) plugin::host->send_event(event, payload);
    Py_RETURN_NONE;
}

static PyObject* py_load_plugin(PyObject* self, PyObject* args) {
    const char* name;
    if (!PyArg_ParseTuple(args, "s", &name)) return NULL;
    if (plugin::host && plugin::host->load_plugin) {
        if (plugin::host->load_plugin(name)) Py_RETURN_TRUE;
        else Py_RETURN_FALSE;
    }
    Py_RETURN_FALSE;
}

static PyObject* py_unload_plugin(PyObject* self, PyObject* args) {
    const char* name;
    if (!PyArg_ParseTuple(args, "s", &name)) return NULL;
    if (plugin::host && plugin::host->unload_plugin) {
        if (plugin::host->unload_plugin(name)) Py_RETURN_TRUE;
        else Py_RETURN_FALSE;
    }
    Py_RETURN_FALSE;
}

static PyObject* py_set_data(PyObject* self, PyObject* args) {
    const char* key;
    const char* value;
    if (!PyArg_ParseTuple(args, "ss", &key, &value)) return NULL;
    if (plugin::host && plugin::host->set_data) {
        if (plugin::host->set_data(key, value)) Py_RETURN_TRUE;
        else Py_RETURN_FALSE;
    }
    Py_RETURN_FALSE;
}

static PyObject* py_get_data(PyObject* self, PyObject* args) {
    const char* key;
    if (!PyArg_ParseTuple(args, "s", &key)) return NULL;
    if (plugin::host && plugin::host->get_data) {
        const char* value = plugin::host->get_data(key);
        if (value) return PyUnicode_FromString(value);
    }
    Py_RETURN_NONE;
}

static PyObject* py_has_data(PyObject* self, PyObject* args) {
    const char* key;
    if (!PyArg_ParseTuple(args, "s", &key)) return NULL;
    if (plugin::host && plugin::host->has_data) {
        if (plugin::host->has_data(key)) Py_RETURN_TRUE;
        else Py_RETURN_FALSE;
    }
    Py_RETURN_FALSE;
}

static PyObject* py_delete_data(PyObject* self, PyObject* args) {
    const char* key;
    if (!PyArg_ParseTuple(args, "s", &key)) return NULL;
    if (plugin::host && plugin::host->delete_data) {
        if (plugin::host->delete_data(key)) Py_RETURN_TRUE;
        else Py_RETURN_FALSE;
    }
    Py_RETURN_FALSE;
}

static PyObject* py_set_timer(PyObject* self, PyObject* args) {
    uint32_t ms;
    PyObject* callback;
    int repeat = 0;
    if (!PyArg_ParseTuple(args, "IO|p", &ms, &callback, &repeat)) return NULL;
    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "callback must be callable");
        return NULL;
    }

    if (plugin::host && plugin::host->set_timer) {
        uint64_t id = plugin::host->set_timer(ms, python_event_proxy, repeat != 0);
        return PyLong_FromUnsignedLongLong(id);
    }
    Py_RETURN_NONE;
}

static PyObject* py_cancel_timer(PyObject* self, PyObject* args) {
    uint64_t timer_id;
    if (!PyArg_ParseTuple(args, "K", &timer_id)) return NULL;
    if (plugin::host && plugin::host->cancel_timer) {
        if (plugin::host->cancel_timer(timer_id)) Py_RETURN_TRUE;
        else Py_RETURN_FALSE;
    }
    Py_RETURN_FALSE;
}

// Extend the method table
static PyMethodDef HostMethods[] = {
    {"log", py_log, METH_VARARGS, ""},
    {"on", py_on, METH_VARARGS, ""},
    {"send_event", py_send_event, METH_VARARGS, ""},
    {"load_plugin", py_load_plugin, METH_VARARGS, ""},
    {"unload_plugin", py_unload_plugin, METH_VARARGS, ""},
    {"set_data", py_set_data, METH_VARARGS, ""},
    {"get_data", py_get_data, METH_VARARGS, ""},
    {"has_data", py_has_data, METH_VARARGS, ""},
    {"delete_data", py_delete_data, METH_VARARGS, ""},
    {"set_timer", py_set_timer, METH_VARARGS, ""},
    {"cancel_timer", py_cancel_timer, METH_VARARGS, ""},
    {NULL, NULL, 0, NULL}
};


static struct PyModuleDef host_module = { PyModuleDef_HEAD_INIT, "host", NULL, -1, HostMethods };
PyMODINIT_FUNC PyInit_host(void) { return PyModule_Create(&host_module); }

api bool plugin_init(PluginHost* host) {
    sethost();

    if (PyImport_AppendInittab("host", PyInit_host) == -1) {
        plugin::host->log("ERROR", "Could not extend python inittab");
        return false;
    }

    Py_Initialize();

    // 1. Setup path so Python can find scripts in 'plugins/python'
    // We use PyRun to set up sys.path easily
    PyRun_SimpleString(
        "import sys, os\n"
        "sys.path.append(os.path.abspath('plugins/python'))\n"
    );

    namespace fs = std::filesystem;
    std::string scriptDir = "plugins/python/";
    
    if (fs::exists(scriptDir)) {
        for (const auto& entry : fs::directory_iterator(scriptDir)) {
            if (entry.path().extension() == ".py" && entry.path().filename() != "api.py") {

                std::string moduleName = entry.path().stem().string();
                
                plugin::host->log("INFO", ("Importing Module: " + moduleName).c_str());

                PyObject* pName = PyUnicode_FromString(moduleName.c_str());
                PyObject* pModule = PyImport_Import(pName);
                
                if (pModule == nullptr) {
                    PyErr_Print();
                    plugin::host->log("ERROR", ("Failed to load: " + moduleName).c_str());
                } else {
                    Py_DECREF(pModule);
                }
                Py_DECREF(pName);
            }
        }
    }
    return true;
}

api void plugin_shutdown() {
    plugin::host->log("INFO", "Python Loader shutting down...");

    if (plugin::host) {
        plugin::host->unregister_event(python_event_proxy);
    }

    PyGILState_STATE gstate = PyGILState_Ensure();
    for (auto& pair : python_event_listeners) {
        Py_DECREF(pair.second);
    }
    python_event_listeners.clear();
    PyGILState_Release(gstate);

    Py_Finalize();
    plugin::host = nullptr;
}