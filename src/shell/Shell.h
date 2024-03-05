/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishShell__
#define __StarfishShell__

#include <string>
#include <cstdint>

#include "PlatformIntegrationData.h"
#include "MiniBrowser.h"
#include "Console.h"

namespace StarfishShell {

// Originally defined in core/page/WebView.h
enum StarfishStartUpFlag {
    enableComputedStyleDump = 1 << 1,
    enableFrameTreeDump = 1 << 2,
    enableStackingContextDump = 1 << 3,
    enableHitTestDump = 1 << 4,
    enableDebugGraphicsLayer = 1 << 5,
    enableDebugRepaintRegion = 1 << 6,
    enableRegressionTest = 1 << 7,
};

struct EnvOptions {
    bool pixelTest = false;
    bool referenceTestState = false;
    bool hideWindow = false;
    bool networkLogVerbose = false;
    bool starfishIgnoreSSLVerify = false;
    int flag = 0;
    std::string screenShot;
    std::string screenShotWidth;
    std::string screenShotHeight;
    std::string glCompositorScale;
};

struct ShellOptions {
    bool crashTest = false;
    bool disableConsole = false;
    int timeout = 0;
};

class Shell {
public:
    Shell();
    ~Shell();

    int run(int argc, char* argv[]);

private:
#if defined(SHELL_ENABLE_BACKTRACE)
    void setBacktraceHandler();
#endif
    void printUsage();
    void parseArg(int argc, char* argv[]);
    void setEnv();

    void runConsoleThread();
    void stopConsoleThread();

    void runCrashTestThread();
    void runTimeoutThread();

    int runMainLoop();
    void stopMainLoop();
    void onTimeout();

    int getExitCode();

    EnvOptions m_envOptions;
    MiniBrowserInitOption m_initOption;
    MiniBrowserSettings m_settings;
    ShellOptions m_shellOptions;
    std::string m_url;

    MiniBrowser* m_browser = nullptr;
    Console* m_console = nullptr;
};

} // namespace StarfishShell

#endif
