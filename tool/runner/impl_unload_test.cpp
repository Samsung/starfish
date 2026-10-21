// Copyright (c) 2026-present Samsung Electronics Co., Ltd.
// SPDX-License-Identifier: Apache-2.0

#include <dlfcn.h>
#include <link.h>
#include <signal.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <glib.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <thread>
#include <chrono>

static void require(bool condition, const char* message)
{
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

template <typename T>
static T symbol(void* handle, const char* name)
{
    auto fn = reinterpret_cast<T>(dlsym(handle, name));
    if (!fn) {
        std::fprintf(stderr, "Missing %s: %s\n", name, dlerror());
        std::exit(1);
    }
    return fn;
}

static bool mapped(const char* path)
{
    struct State {
        const char* path;
        bool found;
    } state{ path, false };
    dl_iterate_phdr(
        [](dl_phdr_info* info, size_t, void* data) {
            auto* state = static_cast<State*>(data);
            if (std::strcmp(info->dlpi_name, state->path) == 0) {
                state->found = true;
            }
            return 0;
        },
        &state);
    FILE* maps = std::fopen("/proc/self/maps", "r");
    require(maps, "cannot inspect mapped segments");
    char line[4096];
    while (std::fgets(line, sizeof(line), maps)) {
        if (std::strstr(line, path)) {
            state.found = true;
        }
    }
    std::fclose(maps);
    return state.found;
}

static std::set<std::string> entries(const char* path)
{
    DIR* dir = opendir(path);
    require(dir, "cannot inspect /proc resources");
    std::set<std::string> result;
    while (auto* entry = readdir(dir)) {
        if (entry->d_name[0] != '.') {
            result.insert(entry->d_name);
        }
    }
    closedir(dir);
    return result;
}

static bool threadExists(size_t tid)
{
    std::string path = "/proc/self/task/" + std::to_string(tid);
    return access(path.c_str(), F_OK) == 0;
}

static void checkThreadLifetime(size_t tid, bool hostThread)
{
    if (!hostThread) {
        // pthread_join has completed, but /proc can briefly retain the task
        // while the kernel finishes exit bookkeeping. Require its removal
        // within a bounded interval instead of depending on that timing.
        auto deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(1);
        while (threadExists(tid) &&
               std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    require(threadExists(tid) == hostThread,
            "shutdown left an unexpected thread lifetime");
}

static void checkCollector(void* handle)
{
    auto vdb = symbol<unsigned (*)()>(handle, "LWEImplTest_VDB")();
    require((vdb != 0) == (std::getenv("GC_ENABLE_INCREMENTAL") != nullptr),
            "collector did not use the requested incremental mode");
    std::printf("GC VDB=%u\n", vdb);
}

static int runAPITest(const char* implPath, const char* apiPath,
                      const char* storage, int cycles, bool hostThread,
                      bool legacy)
{
    auto initialThreads = entries("/proc/self/task");
    auto initialFDs = entries("/proc/self/fd");
    std::string pageURL =
        std::string("file://") + storage + "/unload-page.html";
    size_t previousThread = 0;
    for (int cycle = 0; cycle < cycles; ++cycle) {
        require(!mapped(apiPath), "API was mapped before dlopen");
        require(!mapped(implPath), "impl was mapped before API dlopen");
        void* api = dlopen(apiPath, RTLD_NOW | RTLD_LOCAL);
        if (!api) {
            std::fprintf(stderr, "API dlopen: %s\n", dlerror());
            return 1;
        }
        auto initialize = symbol<void (*)(const char*, unsigned)>(
            api, "LWEAPITest_Initialize");
        auto finalize = symbol<void (*)()>(api, "LWEAPITest_Finalize");
        auto initialized = symbol<bool (*)()>(api, "LWEAPITest_IsInitialized");
        auto canUnload = symbol<bool (*)()>(api, "LWEAPITest_CanUnload");
        auto hasCookies = symbol<bool (*)()>(api, "LWEAPITest_HasCookies");
        auto loadAgain = symbol<bool (*)()>(api, "LWEAPITest_LoadAgain");
        auto swapGlobal =
            symbol<unsigned (*)(unsigned)>(api, "LWEAPITest_SwapGlobal");
        auto swapStatic =
            symbol<unsigned (*)(unsigned)>(api, "LWEAPITest_SwapStatic");
        require(!initialized(), "fresh API is initialized");
        require(canUnload(), "fresh API retains an impl handle");
        require(swapGlobal(41) == 0, "API global survived dlclose");
        require(swapStatic(43) == 0, "API static survived dlclose");
        require(!mapped(implPath), "API query loaded the implementation");

        if (legacy) {
            for (int restart = 0; restart < 3; ++restart) {
                initialize(storage, 1);
                require(initialized(), "retained impl Initialize failed");
                require(!canUnload(), "live API reported safe to unload");
                require(loadAgain(), "retained loader load failed");
                finalize();
                require(!initialized(), "retained impl Finalize failed");
                require(!canUnload(), "API discarded an unprepared impl");
                require(mapped(implPath), "API unmapped an unprepared impl");
            }
            std::puts("PASS: legacy impl retained without shutdown export");
            return 0;
        }

        for (int restart = 0; restart < 2; ++restart) {
            initialize(storage, hostThread ? 0 : 1);
            require(initialized(), "API Initialize failed");
            require(!canUnload(), "live API reported safe to unload");
            require(loadAgain(), "idempotent loader load failed");
            hasCookies();
            require(mapped(implPath), "API did not map the impl");
            // Release this inspection reference before Finalize, so the
            // loader's own dlclose must be the last reference.
            void* impl = dlopen(implPath, RTLD_NOLOAD | RTLD_LAZY);
            require(impl, "cannot inspect the API-loaded impl");
            auto threadID = symbol<size_t (*)()>(impl, "LWEImplTest_ThreadID");
            auto swapImplGlobal =
                symbol<unsigned (*)(unsigned)>(impl, "LWEImplTest_SwapGlobal");
            auto swapTLS =
                symbol<unsigned (*)(unsigned)>(impl, "LWEImplTest_SwapTLS");
            auto exercise =
                symbol<bool (*)(const char*)>(impl, "LWEImplTest_Exercise");
            size_t tid = threadID();
            checkCollector(impl);
            if (hostThread) {
                require(tid == static_cast<size_t>(syscall(SYS_gettid)),
                        "API did not use the caller's host thread");
            } else {
                require(tid != previousThread,
                        "API Finalize reused the old thread");
            }
            require(swapImplGlobal(47) == 0, "API retained impl globals");
            require(swapTLS(53) == 0, "API retained impl TLS");
            require(exercise(pageURL.c_str()),
                    "DOM/JS failed through an API-loaded impl");
            require(dlclose(impl) == 0, "cannot close inspection reference");
            finalize();
            require(!initialized(), "API Finalize left engine initialized");
            require(canUnload(), "API Finalize retained the impl handle");
            require(!mapped(implPath), "API Finalize did not unmap impl");
            checkThreadLifetime(tid, hostThread);
            require(mapped(apiPath), "Finalize unexpectedly unloaded API");
            previousThread = tid;
        }
        require(swapGlobal(41) == 41, "API global reset before API dlclose");
        require(swapStatic(43) == 43, "API static reset before API dlclose");
        require(dlclose(api) == 0, "API dlclose failed");
        require(!mapped(apiPath), "API dlclose kept API mapped");
        require(!mapped(implPath), "API dlclose left impl mapped");
        while (g_main_context_iteration(nullptr, FALSE)) {
        }
        require(entries("/proc/self/task") == initialThreads,
                "API reload leaked a thread");
        require(entries("/proc/self/fd") == initialFDs,
                "API reload leaked a file descriptor");
        std::printf("PASS API cycle %d: impl unmaps, API resets, %s TID %zu\n",
                    cycle + 1, hostThread ? "host" : "dedicated",
                    previousThread);
    }
    return 0;
}

int main(int argc, char** argv)
{
    require(argc == 5 || argc == 6,
            "expected impl path, storage directory, cycle count, thread mode");
    const char* path = argv[1];
    const char* storage = argv[2];
    int cycles = std::atoi(argv[3]);
    bool hostThread = std::strcmp(argv[4], "host") == 0;
    // The host owns this context and its wakeup FD; initialize it before
    // measuring resources owned by an impl load/unload cycle.
    g_main_context_iteration(nullptr, FALSE);
    if (argc == 6) {
        return runAPITest(path, argv[5], argv[2], cycles,
                          std::strcmp(argv[4], "host") == 0,
                          std::strcmp(argv[4], "legacy") == 0);
    }
    auto initialThreads = entries("/proc/self/task");
    auto initialFDs = entries("/proc/self/fd");
    struct sigaction initialSegv;
    struct sigaction initialBus;
    require(sigaction(SIGSEGV, nullptr, &initialSegv) == 0, "read SIGSEGV");
    require(sigaction(SIGBUS, nullptr, &initialBus) == 0, "read SIGBUS");

    std::string pageURL =
        std::string("file://") + storage + "/unload-page.html";
    size_t previousThread = 0;
    for (int cycle = 0; cycle < cycles; ++cycle) {
        require(!mapped(path), "impl was already mapped before dlopen");
        void* handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            std::fprintf(stderr, "dlopen: %s\n", dlerror());
            return 1;
        }
        require(mapped(path), "dlopen did not map the requested impl");
        auto initialize = symbol<void (*)(const char*, unsigned)>(
            handle, "LWEDelegate_LWE_Initialize");
        auto finalize = symbol<void (*)()>(handle, "LWEDelegate_LWE_Finalize");
        auto initialized =
            symbol<bool (*)()>(handle, "LWEDelegate_LWE_IsInitialized");
        auto shutdown =
            symbol<bool (*)()>(handle, "LWEDelegate_LWE_ShutdownForUnload");
        auto swapGlobal =
            symbol<unsigned (*)(unsigned)>(handle, "LWEImplTest_SwapGlobal");
        auto swapStatic =
            symbol<unsigned (*)(unsigned)>(handle, "LWEImplTest_SwapStatic");
        auto threadID = symbol<size_t (*)()>(handle, "LWEImplTest_ThreadID");
        auto swapTLS =
            symbol<unsigned (*)(unsigned)>(handle, "LWEImplTest_SwapTLS");
        auto exercise =
            symbol<bool (*)(const char*)>(handle, "LWEImplTest_Exercise");
        auto pauseHost = symbol<void (*)()>(handle, "LWEImplTest_PauseHost");
        auto shutdownFromLWE =
            symbol<bool (*)()>(handle, "LWEImplTest_ShutdownFromLWEThread");

        require(!initialized(), "fresh impl is initialized");
        require(swapGlobal(17) == 0, "global storage survived dlclose");
        require(swapStatic(23) == 0, "function static survived dlclose");
        require(shutdown(), "shutdown before initialization failed");
        {
            initialize(argv[2], hostThread ? 0 : 1);
            size_t tid = threadID();
            checkCollector(handle);
            if (hostThread) {
                require(tid == static_cast<size_t>(syscall(SYS_gettid)),
                        "impl did not use the caller's host thread");
            } else {
                require(tid != previousThread,
                        "reload reused the previous thread");
            }
            require(swapTLS(31) == 0, "TLS survived the previous thread");
            require(initialized(), "Initialize failed");
            require(!shutdown(), "shutdown accepted a live engine");
            require(exercise(pageURL.c_str()),
                    "DOM/JS did not work after initialization");
            // Keep the process main context idle to exercise cancellation
            // of a timed-out GLib pause source before the DSO disappears.
            if (!hostThread) {
                std::thread pause(pauseHost);
                pause.join();
            }
            finalize();
            require(!initialized(), "Finalize left the engine initialized");
            require(threadExists(tid), "Finalize stopped the reusable thread");
            if (hostThread) {
                bool wrongThreadResult = true;
                std::thread foreign([&] { wrongThreadResult = shutdown(); });
                foreign.join();
                require(!wrongThreadResult,
                        "shutdown accepted the wrong host thread");
            } else {
                require(!shutdownFromLWE(), "shutdown accepted a self-join");
            }
            initialize(argv[2], hostThread ? 0 : 1);
            require(threadID() == tid, "ordinary Finalize changed the thread");
            require(swapTLS(0) == 31, "ordinary Finalize reset the thread TLS");
            require(exercise(pageURL.c_str()),
                    "DOM/JS did not work after ordinary Finalize");
            finalize();
            require(shutdown(), "shutdown after Finalize failed");
            checkThreadLifetime(tid, hostThread);
            require(shutdown(), "repeated shutdown failed");
            previousThread = tid;
        }
        require(swapGlobal(0) == 17, "global changed before dlclose");
        require(swapStatic(0) == 23, "function static changed before dlclose");
        // Leave nonzero values in the mapping to prove reload resets them.
        swapGlobal(17);
        swapStatic(23);
        require(dlclose(handle) == 0, "dlclose failed");
        require(!mapped(path), "dlclose kept the impl mapped");
        while (g_main_context_iteration(nullptr, FALSE)) {
        }
        struct sigaction segv;
        struct sigaction bus;
        sigaction(SIGSEGV, nullptr, &segv);
        sigaction(SIGBUS, nullptr, &bus);
        require(segv.sa_sigaction == initialSegv.sa_sigaction,
                "unloaded impl left a SIGSEGV handler behind");
        require(bus.sa_sigaction == initialBus.sa_sigaction,
                "unloaded impl left a SIGBUS handler behind");
        require(entries("/proc/self/task") == initialThreads,
                "thread count grew across unload");
        auto finalFDs = entries("/proc/self/fd");
        if (finalFDs != initialFDs) {
            for (const auto& fd : finalFDs) {
                if (initialFDs.count(fd) == 0) {
                    char target[4096];
                    std::string file = "/proc/self/fd/" + fd;
                    ssize_t length =
                        readlink(file.c_str(), target, sizeof(target) - 1);
                    if (length >= 0) {
                        target[length] = 0;
                        std::fprintf(stderr, "Extra FD %s: %s\n", fd.c_str(),
                                     target);
                    }
                }
            }
            require(false, "file descriptor count grew across unload");
        }
        std::printf(
            "PASS cycle %d: globals/TLS reset, unmapped impl, %s TID %zu\n",
            cycle + 1, hostThread ? "host" : "dedicated", previousThread);
    }
}
