/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#include "gtest/gtest.h"

#include "LWEWebView.h"
#include "MiniBrowser.h"
#include "Shell.h"

#include <cstdio>
#include <string>
#include <unistd.h>
#include <vector>

namespace StarfishShell {

// The shell's own entry points, run in-process. Each MiniBrowser run
// initializes and finalizes the engine itself, like a separate launch.
static int runShell(std::vector<const char*> args)
{
    Shell shell;
    return shell.run(static_cast<int>(args.size()),
                     const_cast<char**>(args.data()));
}

TEST(ShellTest, VersionAndHelp)
{
    EXPECT_EQ(0, runShell({ "Starfish", "--version" }));
    EXPECT_EQ(0, runShell({ "Starfish", "-h" }));
}

TEST(ShellTest, CreateDestroyTest)
{
    // Wrong arity prints the usage and reports failure.
    EXPECT_EQ(0, runShell({ "Starfish", "create-destroy-test", "1" }));
    // Two full create/run/destroy cycles of one second each.
    EXPECT_EQ(1, runShell({ "Starfish", "create-destroy-test", "2", "1",
                            "about:blank" }));
}

TEST(ShellTest, ReplayRecording)
{
    EXPECT_EQ(0, runShell({ "Starfish", "replay" }));

    const char* path = "/tmp/starfish_shell_replay_test.jsonl";
    FILE* f = fopen(path, "w");
    ASSERT_NE(f, nullptr);
    fprintf(
        f,
        "{\"type\":\"header\",\"ts_us\":0,\"args\":"
        "{\"version\":1,\"w\":320,\"h\":240,\"dpr\":1.000,"
        "\"font\":\"serif\",\"locale\":\"ko-KR\",\"tz\":\"Asia/Seoul\"}}\n");
    fprintf(f,
            "{\"type\":\"LoadURL\",\"ts_us\":100,\"args\":"
            "{\"url\":\"data:text/html,<title>replayed</title>\"}}\n");
    fprintf(f,
            "{\"type\":\"SetDevicePixelRatio\",\"ts_us\":2000,\"args\":"
            "{\"dpr\":2.000}}\n");
    fprintf(f,
            "{\"type\":\"ScrollTo\",\"ts_us\":3000,\"args\":"
            "{\"x\":0,\"y\":10}}\n");
    fprintf(f, "{\"type\":\"Reload\",\"ts_us\":4000,\"args\":{}}\n");
    fclose(f);
    // The replay drives a MiniBrowser until its timeout, then exits cleanly.
    EXPECT_EQ(0, runShell({ "Starfish", "replay", path, "--speed=10",
                            "--timeout=2" }));
    unlink(path);
}

TEST(ShellTest, MiniBrowserControls)
{
    MiniBrowser browser;
    MiniBrowser::InitOption init;
    init.geometry = { 0, 0, 320, 240 };
    ASSERT_TRUE(browser.init(init));
    // Spins the browser's loop until `script` evaluates to `expected`.
    auto waitFor = [&browser](const std::string& script,
                              const std::string& expected) {
        for (int i = 0; i < 100; i++) {
            if (browser.evaluateJavaScript(script) == expected) {
                return true;
            }
            browser.runMainLoopWithTimeout(0.05);
        }
        return false;
    };

    MiniBrowser::Settings settings;
    settings.customUserAgentString = "ShellTest/1.0";
    browser.setSettings(settings);
    browser.loadURL("data:text/html,<title>mini</title>");
    browser.focus();
    EXPECT_TRUE(waitFor("document.title", "mini"));
    EXPECT_EQ("ShellTest/1.0",
              browser.evaluateJavaScript("navigator.userAgent"));

    browser.setDevicePixelRatio(2);
    EXPECT_TRUE(waitFor("String(devicePixelRatio)", "2"));

    browser.setRotate(90);
    browser.runMainLoopWithTimeout(0.1);
    browser.setRotate(0);

    // A reload starts a new document, with a new time origin.
    std::string origin =
        browser.evaluateJavaScript("String(performance.timeOrigin)");
    browser.reload();
    EXPECT_TRUE(
        waitFor("String(performance.timeOrigin) !== '" + origin + "'", "true"));
}

} // namespace StarfishShell
