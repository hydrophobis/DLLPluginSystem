#ifndef OCULAR_H
#define OCULAR_H

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <algorithm>
#include <ctime>
#include <random>
#include <limits>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <cstdlib>
#include <utility>
#include <windows.h>

class Threading {
public:
    void thread_sleep(std::chrono::milliseconds ms) {
        std::this_thread::sleep_for(ms);
    }

    void thread_sleep(std::chrono::seconds s) {
        std::this_thread::sleep_for(s);
    }

    void thread_sleep(std::chrono::microseconds mcs) {
        std::this_thread::sleep_for(mcs);
    }

    std::thread::id thread_id() {
        return std::this_thread::get_id();
    }

    void thread_join(std::vector<std::thread>& threads) {
        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }
    }
};

struct Task {
    std::chrono::milliseconds delay;
    std::string task_path;

    void run() const {
        std::this_thread::sleep_for(delay);
        system(task_path.c_str());
    }
};

class TaskScheduler {
public:
    TaskScheduler(int numThreads) : running(true) {
        for (int i = 0; i < numThreads; ++i) {
            threads.emplace_back(&TaskScheduler::workerThread, this);
        }
    }

    ~TaskScheduler() {
        running = false;
        cv.notify_all();
        th.thread_join(threads);
    }

    void addTask(const Task& task) {
        std::lock_guard<std::mutex> lock(mutex);
        tasks.push(task);
        cv.notify_one();
    }

    void startProcess(const std::string& path) {
        std::thread([path]() {
            STARTUPINFOA si{};
            PROCESS_INFORMATION pi{};
            si.cb = sizeof(si);

            CreateProcessA(
                nullptr,
                const_cast<char*>(path.c_str()),
                nullptr,
                nullptr,
                FALSE,
                0,
                nullptr,
                nullptr,
                &si,
                &pi
            );

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }).detach();
    }

    void stopProcess(DWORD pid) {
        HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (process) {
            TerminateProcess(process, 1);
            CloseHandle(process);
        }
    }

private:
    Threading th;
    std::vector<std::thread> threads;
    std::queue<Task> tasks;
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<bool> running;

    void workerThread() {
        while (running) {
            Task task;
            {
                std::unique_lock<std::mutex> lock(mutex);
                cv.wait(lock, [this]() { return !tasks.empty() || !running; });
                if (!running) return;

                task = tasks.front();
                tasks.pop();
            }
            task.run();
        }
    }
};

class Log {
public:
    Log(const char* func_name) : func_name(func_name) {
        std::cout << "Entering function: " << func_name << std::endl;
    }

    ~Log() {
        std::cout << "Exiting function: " << func_name << std::endl;
    }

private:
    const char* func_name;
};

class Watch {
public:
    template<typename Func, typename... Args>
    auto track(Func func, Args&&... args) {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = func(std::forward<Args>(args)...);
        auto end = std::chrono::high_resolution_clock::now();

        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        return std::make_pair(elapsed, result);
    }
};

#define AUTOLOG Log log(__func__)

#endif
