#include "../plugin_api.h"
#include "../ini.h"
#include <Python.h>
#include <string>
#include <map>
#include <filesystem>
#include <iostream>

static PluginHost* g_host = nullptr;
static std::multimap<std::string, PyObject*> python_event_listeners;

// Proxy for events
void __cdecl python_event_proxy(const char* eventName, const char* payload) {
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
    if (g_host) g_host->log(lvl, msg);
    Py_RETURN_NONE;
}

static PyObject* py_on(PyObject* self, PyObject* args) {
    const char* event;
    PyObject* callback;
    if (!PyArg_ParseTuple(args, "sO", &event, &callback)) return NULL;

    if (python_event_listeners.find(event) == python_event_listeners.end()) {
        if (g_host) g_host->register_event(event, python_event_proxy);
    }

    Py_INCREF(callback);
    python_event_listeners.insert({event, callback});
    Py_RETURN_NONE;
}

static PyMethodDef HostMethods[] = {
    {"log", py_log, METH_VARARGS, ""},
    {"on", py_on, METH_VARARGS, ""},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef host_module = { PyModuleDef_HEAD_INIT, "host", NULL, -1, HostMethods };
PyMODINIT_FUNC PyInit_host(void) { return PyModule_Create(&host_module); }

expose pluginbhvr const PluginInfo* plugin_get_info() {
    static PluginInfo info = {"python", "1.0", ABI_V1, PRIORITY_DEFAULT};
    return &info;
}

expose pluginbhvr bool plugin_init(PluginHost* host) {
    g_host = host;
    
    std::string home_path = "python"; // Default value
    auto ini_entries = parse_ini("plugins/python/python.ini", "PYTHON");

    for (const auto& entry : ini_entries) {
        size_t eq_pos = entry.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = entry.substr(0, eq_pos);
            std::string value = entry.substr(eq_pos + 1);

            if (key == "HOME") { // Specifically look for the HOME key
                home_path = value;
                break; 
            }
        }
    }

    // Use a static buffer to ensure the pointer remains valid for Python
    static std::wstring pyHome;
    
    // Better way to convert string to wstring (handling potential encoding)
    pyHome.assign(home_path.begin(), home_path.end()); 
    
    Py_SetPythonHome(pyHome.c_str());
    
    std::wcout << L"[Python Loader] Python home set to: " << pyHome << std::endl;

    if (PyImport_AppendInittab("host", PyInit_host) == -1) {
        g_host->log("ERROR", "Could not extend python inittab");
        return false;
    }

    Py_Initialize();

    // Give Python the absolute path to your scripts
    PyRun_SimpleString("import sys, os; sys.path.append(os.path.abspath('plugins/python'))");

    namespace fs = std::filesystem;
    std::string scriptDir = "plugins/python/";
    
    if (fs::exists(scriptDir)) {
        for (const auto& entry : fs::directory_iterator(scriptDir)) {
            if (entry.path().extension() == ".py") {
                std::string fullPath = entry.path().string();
                // Normalize path separators for Python (Windows backslashes to forward slashes)
                std::replace(fullPath.begin(), fullPath.end(), '\\', '/');

                g_host->log("INFO", ("Executing script: " + fullPath).c_str());

                // Build a Python snippet to read and execute the file
                // This provides much better error reporting than PyImport
                std::string loaderCmd = 
                    "try:\n"
                    "    with open('" + fullPath + "', 'r') as f:\n"
                    "        exec(f.read(), globals())\n"
                    "except Exception as e:\n"
                    "    print(f'PYTHON ERROR in " + fullPath + ": {e}')\n"
                    "    import traceback\n"
                    "    traceback.print_exc()";

                if (PyRun_SimpleString(loaderCmd.c_str()) != 0) {
                    g_host->log("ERROR", "Python interpreter failed to handle the execution command.");
                } else {
                    g_host->log("INFO", "Script execution block finished.");
                }
            }
        }
    }
    return true;
}

expose pluginbhvr void plugin_shutdown() {
    g_host->log("INFO", "Python Loader shutting down...");

    if (g_host) {
        g_host->unregister_event(python_event_proxy);
    }

    PyGILState_STATE gstate = PyGILState_Ensure();
    for (auto& pair : python_event_listeners) {
        Py_DECREF(pair.second);
    }
    python_event_listeners.clear();
    PyGILState_Release(gstate);

    Py_Finalize();
    g_host = nullptr;
}