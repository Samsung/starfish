/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "ShellConfig.h"

#include "Shell.h"

#include "LWEWebView.h"
#include "MiniBrowser.h"

#if defined(SHELL_ENABLE_BACKTRACE)
#include <execinfo.h>
#endif

#include <cstring>
#include <thread>
#include <future>
#include <pthread.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <string>
#include <chrono>
#include <signal.h>

namespace {
constexpr uint32_t kDefaultWidth = 1920;
constexpr uint32_t kDefaultHeight = 1080;

} // namespace

namespace StarfishShell {

Shell::Shell()
{
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    // Changing these options can reducing {malloc, free} internal memory pool
    // usage
    // for big chunk ex) packets for MSE
    mallopt(M_MMAP_THRESHOLD, 2048);
    mallopt(M_MMAP_MAX, 1024 * 1024);
}

Shell::~Shell()
{
}

int Shell::run(int argc, char* argv[])
{
#if defined(SHELL_ENABLE_BACKTRACE)
    setBacktraceHandler();
#endif

    if (argc == 1) {
        printUsage();
        return false;
    }

    return runMiniBrowser(argc, argv);
}

int Shell::runMiniBrowser(int argc, char* argv[])
{
    MiniBrowser::EnvironmentValues env;
    MiniBrowser::InitOption init;
    MiniBrowser::Settings settings;
    MiniBrowser::OtherOptions others;

    init.geometry = { 0, 0, kDefaultWidth, kDefaultHeight };
    MiniBrowser::parseArgs(argc, argv, env, init, settings, others);

    MiniBrowser::setEnvironmentValues(env);

    MiniBrowser* browser = new MiniBrowser();
    if (!browser->init(init)) {
        return false;
    }

    browser->setSettings(settings);
    browser->loadURL(argv[1]);
    browser->focus();

    if (!others.disableConsole) {
        browser->runConsole();
    }

    if (others.crashTest) {
        runCrashTestThread();
    }

    if (others.timeout > 0) {
        runTimeoutThread(others.timeout);
    }

    int ret = runMainLoop();
    if (ret != 0) {
        return ret;
    }

    if (browser) {
        delete browser;
    }

    stopMainLoop();

    return getExitCode();
}

#if defined(SHELL_ENABLE_BACKTRACE)
static void sigHandler(int sig, struct sigcontext ctx)
{
    void* trace[128];
    char** messages = (char**)NULL;
    int i, trace_size = 0;

    // `[STARFISH_TEST] Got signal` string is used by test case runner
    // don't change!
    if (sig == SIGSEGV) {
        printf(
            "[STARFISH_TEST] Got signal %d, pid %d, faulty address is %p, from "
            "%p\n",
            sig, (int)getpid(), (void*)ctx.cr2, (void*)ctx.rip);
    } else {
        printf("[STARFISH_TEST] Got signal %d, pid %d\n", sig, (int)getpid());
    }

    trace_size = backtrace(trace, 128);
    /* overwrite sigaction with caller's address */
    trace[1] = (void*)ctx.rip;
    messages = backtrace_symbols(trace, trace_size);
    /* skip first stack frame (points here) */
    printf("[bt] Execution path:\n");
    for (i = 1; i < trace_size; ++i) {
        printf("[bt] #%d %s ", i, messages[i]);

        char syscom[256];
        std::string temp = messages[i];
        auto moduleEnd = temp.find("(");
        auto addrStart = temp.find("+");
        auto addrEnd = temp.find(")");
        if (moduleEnd != std::string::npos && addrStart != std::string::npos &&
            addrEnd != std::string::npos) {
            std::string modulePath = temp.substr(0, moduleEnd);
            std::string addr = temp.substr(addrStart, addrEnd - addrStart);
            sprintf(syscom, "addr2line %s -e %s", addr.c_str(),
                    modulePath.c_str());
            system(syscom);
        } else {
            printf("\n");
        }
    }

    fflush(stdout);

    // this is the trick: it will trigger the core dump
    signal(sig, SIG_DFL);
    kill(getpid(), sig);
}

void Shell::setBacktraceHandler()
{
    /* Install our signal handler */
    struct sigaction sa;

    sa.sa_handler = (void (*)(int))sigHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
}
#endif

void Shell::printUsage()
{
    puts("please specify url");
}

void Shell::runCrashTestThread()
{
    pthread_t t;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(
        &t, &attr,
        [](void* data) -> void* {
            sleep(5);
            puts("raise SIGINT for crash test");
            puts(
                "if there is no crash until process exit, there "
                "is no problem");
            raise(SIGINT);
            return NULL;
        },
        nullptr);
}

void Shell::runTimeoutThread(int timeout)
{
    struct Param {
        std::future<int> future;
        Shell* shell;
    };
    Param* param = new Param();

    param->future = std::async(std::launch::async, [timeout]() {
        std::this_thread::sleep_for(std::chrono::seconds(timeout));
        return 1;
    });
    param->shell = this;

    pthread_t t;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(
        &t, &attr,
        [](void* data) -> void* {
            Param* param = reinterpret_cast<Param*>(data);
            std::future_status status;
            do {
                status = param->future.wait_for(std::chrono::seconds(1));
                switch (status) {
                case std::future_status::deferred:
                    puts("deferred");
                    break;
                case std::future_status::timeout:
                    puts("timeout");
                    break;
                case std::future_status::ready:
                    puts("ready!");
                    break;
                default:
                    puts("default!");
                    break;
                }
            } while (status != std::future_status::ready);

            param->shell->onTimeout();
            delete param;
            return nullptr;
        },
        param);
}

int Shell::getExitCode()
{
    int exitCode = 0;
    if (getenv("EXIT_CODE")) {
        exitCode = std::atoi(getenv("EXIT_CODE"));
    }
    return exitCode;
}
} // namespace StarfishShell

using namespace StarfishShell;

int main(int argc, char* argv[])
{
    Shell shell;
    return shell.run(argc, argv);
}
