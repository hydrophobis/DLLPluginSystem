#ifndef ABI_COMPAT_LAYER_H
#define ABI_COMPAT_LAYER_H

#ifdef _WIN32
    #define WINLIN(windows, linux) windows
    #define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
    #include <windows.h>
    #include <libloaderapi.h>
    #include <synchapi.h>
    #include <conio.h>

    // Types
    typedef HMODULE PluginHandle;

    // Library Loading
    #define PLATFORM_LOAD_LIB(path) LoadLibraryA(path)
    #define PLATFORM_GET_PROC(handle, name) GetProcAddress(handle, name)
    #define PLATFORM_FREE_LIB(handle) FreeLibrary(handle)
    
    // System
    #define PLATFORM_SLEEP_MS(ms) Sleep(ms)
    
    // Input (already exists in conio.h, just mapping/wrapping if needed)
    inline int platform_kbhit() { return _kbhit(); }
    inline int platform_getch() { return _getch(); }

#else
    #define WINLIN(windows, linux) linux
    #include <dlfcn.h>
    #include <unistd.h>
    #include <termios.h>
    #include <fcntl.h>
    #include <sys/select.h>
    #include <stdio.h>

    // Types
    typedef void* PluginHandle;

    // Library Loading
    // RTLD_LAZY is standard for plugins
    #define PLATFORM_LOAD_LIB(path) dlopen(path, RTLD_LAZY)
    #define PLATFORM_GET_PROC(handle, name) dlsym(handle, name)
    #define PLATFORM_FREE_LIB(handle) dlclose(handle)

    // System
    #define PLATFORM_SLEEP_MS(ms) usleep((ms) * 1000)

    // Linux implementation of conio.h _kbhit
    inline int platform_kbhit() {
        struct timeval tv = { 0L, 0L };
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
    }

    // Linux implementation of conio.h _getch
    inline int platform_getch() {
        int r;
        unsigned char c;
        if ((r = read(STDIN_FILENO, &c, sizeof(c))) < 0) {
            return r;
        } else {
            return c;
        }
    }

    // Helper to setup terminal for non-canonical input (raw mode)
    // This allows reading chars without hitting enter
    struct TermSetup {
        struct termios oldt;
        TermSetup() {
            tcgetattr(STDIN_FILENO, &oldt);
            struct termios newt = oldt;
            newt.c_lflag &= ~(ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        }
        ~TermSetup() {
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        }
    };
    
    // Global static to ensure terminal resets on exit
    static TermSetup _term_setup; 

#endif

#endif // ABI_COMPAT_LAYER_H