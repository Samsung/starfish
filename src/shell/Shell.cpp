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
#include <future>
#include <pthread.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <string>
#include <chrono>
#include <signal.h>

namespace StarfishShell {

Shell::Shell()
{
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

#if !defined(SHELL_ANDROID) && !defined(SHELL_WINDOWS)
    // Changing these options can reducing {malloc, free} internal memory pool
    // usage
    // for big chunk ex) packets for MSE
    mallopt(M_MMAP_THRESHOLD, 2048);
    mallopt(M_MMAP_MAX, 1024 * 1024);
#endif
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

    parseArg(argc, argv);
    setEnv();

    m_browser = new MiniBrowser();
    if (!m_browser->init(m_initOption)) {
        return false;
    }

    m_browser->setSettings(m_settings);
    m_browser->loadURL(m_url);
    m_browser->focus();

    if (!m_shellOptions.disableConsole) {
        m_console = Console::create(m_browser);
        m_console->run();
    }

    if (m_shellOptions.crashTest) {
        runCrashTestThread();
    }

    if (m_shellOptions.timeout > 0) {
        runTimeoutThread();
    }

    int ret = runMainLoop();
    if (ret != 0) {
        return ret;
    }

    if (m_console) {
        delete m_console;
    }
    if (m_browser) {
        delete m_browser;
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

void Shell::parseArg(int argc, char* argv[])
{
    m_url = argv[1];
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--dump-computed-style") == 0) {
            m_envOptions.flag |= StarfishStartUpFlag::enableComputedStyleDump;
        } else if (strcmp(argv[i], "--dump-frame-tree") == 0) {
            m_envOptions.flag |= StarfishStartUpFlag::enableFrameTreeDump;
        } else if (strcmp(argv[i], "--dump-stacking-context") == 0) {
            m_envOptions.flag |= StarfishStartUpFlag::enableStackingContextDump;
        } else if (strcmp(argv[i], "--dump-hittest") == 0) {
            m_envOptions.flag |= StarfishStartUpFlag::enableHitTestDump;
        } else if (strcmp(argv[i], "--debug-graphics-layer") == 0) {
            m_envOptions.flag |= StarfishStartUpFlag::enableDebugGraphicsLayer;
        } else if (strcmp(argv[i], "--debug-repaint-region") == 0) {
            m_envOptions.flag |= StarfishStartUpFlag::enableDebugRepaintRegion;
        } else if (strcmp(argv[i], "--pixel-test") == 0) {
#ifdef SHELL_ENABLE_TEST
            m_envOptions.pixelTest = true;
#endif
        } else if (strcmp(argv[i], "--ref-test") == 0) {
#ifdef SHELL_ENABLE_TEST
            m_envOptions.referenceTestState = true;
#endif
        } else if (strstr(argv[i], "--width=") == argv[i]) {
            m_initOption.geometry.width =
                std::atoi(argv[i] + strlen("--width="));
        } else if (strstr(argv[i], "--height=") == argv[i]) {
            m_initOption.geometry.height =
                std::atoi(argv[i] + strlen("--height="));
        } else if (strcmp(argv[i], "--regression-test") == 0) {
            m_envOptions.flag |= StarfishStartUpFlag::enableRegressionTest;
        } else if (strstr(argv[i], "--screen-shot=") == argv[i]) {
            m_envOptions.screenShot = argv[i] + strlen("--screen-shot=");
        } else if (strstr(argv[i], "--screen-shot-width=") == argv[i]) {
            m_envOptions.screenShotWidth =
                (argv[i] + strlen("--screen-shot-width="));
        } else if (strstr(argv[i], "--screen-shot-height=") == argv[i]) {
            m_envOptions.screenShotHeight =
                argv[i] + strlen("--screen-shot-height=");
        } else if (strcmp(argv[i], "--hide-window") == 0) {
            // regression test, pixel test only
            m_envOptions.hideWindow = true;
            m_envOptions.flag |= StarfishStartUpFlag::enableRegressionTest;
        } else if (strcmp(argv[i], "--network-log-verbose") == 0) {
            m_envOptions.networkLogVerbose = true;
        } else if (strstr(argv[i], "--posX=") == argv[i]) {
            m_initOption.geometry.x = std::atoi(argv[i] + strlen("--posX="));
        } else if (strstr(argv[i], "--posY=") == argv[i]) {
            m_initOption.geometry.y = std::atoi(argv[i] + strlen("--posY="));
        } else if (strstr(argv[i], "--device-pixel-ratio=") == argv[i]) {
            m_initOption.scaleFactor =
                std::atof(argv[i] + strlen("--device-pixel-ratio="));
        } else if (strstr(argv[i], "--useragent=") == argv[i]) {
            m_settings.customUserAgentString = argv[i] + strlen("--useragent=");
        } else if (strcmp(argv[i], "--disable-web-security") == 0) {
            m_settings.enableSecurity = false;
        } else if (strcmp(argv[i], "--tts-forced") == 0) {
            m_settings.ttsMode = LWE::TTSMode::Forced;
        } else if (strcmp(argv[i], "--crash-test") == 0) {
            m_shellOptions.crashTest = true;
        } else if (strstr(argv[i], "--needs-download-webfont-early") ==
                   argv[i]) {
            m_settings.needsDownloadWebFontsEarly = true;
        } else if (strcmp(argv[i], "--disable-console") == 0) {
            m_shellOptions.disableConsole = true;
        } else if (strstr(argv[i],
                          "--needs-downscale-image-resource-larger-than=") ==
                   argv[i]) {
            m_settings.needsDownScaleImageResourceLargerThan = std::atoi(
                argv[i] +
                strlen("--needs-downscale-image-resource-larger-than="));
        } else if (strstr(argv[i], "--scrollbar-unvisible")) {
            m_settings.scrollbarVisible = false;
        } else if (strstr(argv[i], "--use-external-popup")) {
            m_settings.useExternalPopup = true;
        } else if (strstr(argv[i], "--use-spatial-navigation")) {
            m_settings.useSpatialNavigation = true;
        } else if (strcmp(argv[i], "--use-http2") == 0) {
            m_settings.useHTTP2 = true;
        } else if (strstr(argv[i], "--tts-language=") == argv[i]) {
            m_settings.language = argv[i] + strlen("--tts-language=");
        } else if (strstr(argv[i], "--timeout=") == argv[i]) {
            m_shellOptions.timeout = std::atoi(argv[i] + strlen("--timeout="));
        } else if (strstr(argv[i], "--ignore-ssl-verify")) {
            m_envOptions.starfishIgnoreSSLVerify = true;
        } else if (strstr(argv[i], "--gl-compositor-scale=") == argv[i]) {
            // this is secret feature for testing(working on gl + efl webview)
            m_envOptions.glCompositorScale =
                argv[i] + strlen("--gl-compositor-scale=");
        }
    }
}

void Shell::setEnv()
{
    if (m_envOptions.screenShot.length()) {
        setenv("SCREEN_SHOT", m_envOptions.screenShot.data(), 1);
        setenv("SCREEN_SHOT_FILE", m_envOptions.screenShot.c_str(), 1);
        setenv("EXIT_AFTER_SCREEN_SHOT", "1", 1);
    }

    if (m_envOptions.screenShotWidth.length()) {
        setenv("SCREEN_SHOT_WIDTH", m_envOptions.screenShotWidth.c_str(), 1);
    }

    if (m_envOptions.screenShotHeight.length()) {
        setenv("SCREEN_SHOT_HEIGHT", m_envOptions.screenShotHeight.c_str(), 1);
    }

    if (m_envOptions.hideWindow) {
        setenv("HIDE_WINDOW", "1", 1);
    }

    if (m_envOptions.networkLogVerbose) {
        setenv("NETWORK_LOG_VERBOSE", "1", 1);
    }

    if (m_envOptions.starfishIgnoreSSLVerify) {
        setenv("IGNORE_SSL_VERIFY", "1", 1);
    }

    if (m_envOptions.glCompositorScale.length()) {
        setenv("LWE_GL_COMPOSITOR_SCALE",
               m_envOptions.glCompositorScale.c_str(), 1);
    }

    if (m_envOptions.pixelTest) {
        setenv("PIXEL_TEST", "1", 1);
    }

    if (m_envOptions.referenceTestState) {
        setenv("REF_TEST_STATE", "1", 1);
        setenv("HIDE_WINDOW", "1", 1);
    }

    std::string startUpFlag = std::to_string(m_envOptions.flag);
    setenv("START_UP_FLAG", startUpFlag.c_str(), 1);
    setenv("SHELL_DONE_FLAG", "0", 1);
    setenv("EXIT_CODE", "0", 1);
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

void Shell::runTimeoutThread()
{
    struct Param {
        std::future<int> future;
        Shell* shell;
    };
    Param* param = new Param();

    param->future = std::async(std::launch::async, [this]() {
        std::this_thread::sleep_for(
            std::chrono::seconds(this->m_shellOptions.timeout));
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
