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
#include "UnitTestRunner.h"

#if defined(SHELL_ENABLE_BACKTRACE)
#include <execinfo.h>
#endif

#include <cstring>
#include <memory>
#include <pthread.h>
#include <malloc.h>
#include <unistd.h>
#include <signal.h>

#include <vector>

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
    if (argc == 1) {
        printUsage();
        return false;
    }

    if (strstr(argv[1], "unit-test")) {
        return runUnitTest(argc, argv);
    } else if (strstr(argv[1], "create-destroy-test")) {
        // Usage: ./Starfish create-destroy-test {repeat-count} {interval} {URL}
        return runCreateDestroyTest(argc, argv);
    } else {
        return runMiniBrowser(argc, argv);
    }
}

int Shell::runUnitTest(int argc, char* argv[])
{
    UnitTestRunner runner;
    runner.initialize(argc, argv);
    return runner.runAllTests();
}

int Shell::runCreateDestroyTest(int argc, char* argv[])
{
    if (argc != 5) {
        printf(
            "Usage: ./Starfish create-destroy-test {repeat-count} {interval} "
            "{URL}");
        return false;
    }

    int repeatCount = std::atoi(argv[2]);
    int interval = std::atoi(argv[3]); // seconds
    std::string timeout = "--timeout=" + std::string(argv[3]);
    std::string url = argv[4];
    std::vector<const char*> newArgv;

    newArgv.push_back("Starfish");
    newArgv.push_back(url.c_str());
    newArgv.push_back(timeout.c_str());

    while (repeatCount--) {
        runMiniBrowser(newArgv.size(), const_cast<char**>(newArgv.data()));
    }
    return true;
}

int Shell::runMiniBrowser(int argc, char* argv[])
{
#if defined(SHELL_ENABLE_BACKTRACE)
    setBacktraceHandler();
#endif

    MiniBrowser::EnvironmentValues env;
    MiniBrowser::InitOption init;
    MiniBrowser::Settings settings;
    MiniBrowser::OtherOptions others;

    init.geometry = { 0, 0, kDefaultWidth, kDefaultHeight };
    MiniBrowser::parseArgs(argc, argv, env, init, settings, others);

    MiniBrowser::setEnvironmentValues(env);

    auto browser = std::unique_ptr<MiniBrowser>(new MiniBrowser());
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
    int ret = 0;
    if (others.timeout > 0.0) {
        ret = browser->runMainLoopWithTimeout(others.timeout);
    } else {
        ret = browser->runMainLoop();
    }
    if (ret != 0) {
        return ret;
    }

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
