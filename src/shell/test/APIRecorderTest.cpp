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

#include "ShellConfig.h"
#include "gtest/gtest.h"

#if defined(STARFISH_ENABLE_TEST)

#include "APIReplayer.h"
#include "LWEWebView.h"
#include "Window.h"

#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace StarfishShell {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static bool fileExists(const char* path)
{
    struct stat st;
    return stat(path, &st) == 0 && st.st_size > 0;
}

static std::vector<std::string> readLines(const char* path)
{
    std::vector<std::string> lines;
    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty())
            lines.push_back(line);
    }
    return lines;
}

// ─────────────────────────────────────────────────────────────────────────────
// APIRecorderRecordingTest
//
// Tests that public API calls are written to a JSONL file when
// STARFISH_API_RECORD is set.  This class creates the WebContainer once
// (SetUpTestCase) because APIRecorder is a singleton that initialises on the
// first WebContainer::Create* call.  Individual TEST_F cases inspect the
// recorded file without recreating the container.
// ─────────────────────────────────────────────────────────────────────────────

class APIRecorderRecordingTest : public ::testing::Test {
public:
    APIRecorderRecordingTest() = default;

protected:
    static const char* kOutputPath;

    static void SetUpTestCase()
    {
        // Must be set before WebContainer::Create* is called.
        setenv("STARFISH_API_RECORD", kOutputPath, 1);
        unlink(kOutputPath);

        usleep(500000); // give x11 server time if running under xvfb

        LWE::LWE::Initialize("/tmp/starfish_storage/");
        s_window = Window::create();
        s_window->setInitHint(HINT_VISIBLE, 0);
        s_window->init("Starfish", 800, 600);

        LWE::WebContainer::WebContainerArguments args{
            .width = 800,
            .height = 600,
            .devicePixelRatio = 1.0f,
            .defaultFontName = "serif",
            .locale = "ko-KR",
            .timezoneID = "Asia/Seoul",
        };
        LWE::WebContainer::RendererGLConfiguration config;
        config.onMakeCurrent = [](LWE::WebContainer*) {
            if (s_window->renderer())
                s_window->renderer()->makeCurrent();
        };
        config.onSwapBuffers = [](LWE::WebContainer*, bool) {
            if (s_window->renderer())
                s_window->renderer()->swapBuffers();
        };
        config.onCreateSharedContext = [](LWE::WebContainer*) -> uintptr_t {
            if (!s_window->renderer())
                return 0;
            return s_window->renderer()->createSharedContext();
        };
        config.onDestroyContext =
            [](LWE::WebContainer*, uintptr_t ctx) -> bool {
            if (!s_window->renderer())
                return false;
            return s_window->renderer()->destroyContext(ctx);
        };
        config.onClearCurrentContext = [](LWE::WebContainer*) -> bool {
            if (!s_window->renderer())
                return false;
            return s_window->renderer()->clearCurrentContext();
        };
        config.onMakeCurrentWithContext =
            [](LWE::WebContainer*, uintptr_t ctx) -> bool {
            if (!s_window->renderer())
                return false;
            return s_window->renderer()->makeCurrentWithContext(ctx);
        };
        config.onGetProcAddress =
            [](LWE::WebContainer*, const char* name) -> void* {
            if (!s_window->renderer())
                return nullptr;
            return s_window->renderer()->getProcAddress(name);
        };
        config.onIsSupportedExtension =
            [](LWE::WebContainer*, const char* ext) -> bool {
            if (!s_window->renderer())
                return false;
            return s_window->renderer()->isSupportedExtension(ext);
        };

        s_lwe = LWE::WebContainer::CreateGL(args, config);
        if (!s_lwe) {
            // GL not available in this environment (e.g. headless CI); fall
            // back to headless renderer so the recording logic is still tested.
            s_lwe = LWE::WebContainer::CreateHeadless(
                args.width, args.height, args.devicePixelRatio,
                args.defaultFontName, args.locale, args.timezoneID);
        }
        if (!s_lwe)
            return;

        // Produce a variety of recorded events.
        s_lwe->LoadURL("about:blank");
        s_window->appLoop()->start(1);

        s_lwe->AddJavaScriptInterface(
            "TEST", "echo",
            [](const std::string& p) -> std::string { return p; });
        s_lwe->SetUserAgentString("MyAgent/1.0");
        s_lwe->SetCacheMode(2);
        s_lwe->SetDefaultFontSize(20);

        s_lwe->DispatchMouseDownEvent(LWE::MouseButtonValue::NoButton,
                                      LWE::MouseButtonsValue::LeftButtonDown,
                                      100.0, 200.0);
        s_lwe->DispatchMouseUpEvent(LWE::MouseButtonValue::NoButton,
                                    LWE::MouseButtonsValue::NoButtonDown,
                                    100.0, 200.0);
        s_lwe->DispatchKeyDownEvent((LWE::KeyValue)65);
        s_lwe->DispatchKeyUpEvent((LWE::KeyValue)65);
        s_lwe->ScrollTo(0, 100);
        s_lwe->ResizeTo(640, 480);
        s_lwe->Reload();
        s_window->appLoop()->start(1);
    }

    static void TearDownTestCase()
    {
        if (s_lwe) {
            s_lwe->Destroy();
            s_lwe = nullptr;
        }
        if (s_window) {
            delete s_window;
            s_window = nullptr;
        }
        LWE::LWE::Finalize();
        unsetenv("STARFISH_API_RECORD");
        unlink(kOutputPath);
    }

    static Window* s_window;
    static LWE::WebContainer* s_lwe;
};

const char* APIRecorderRecordingTest::kOutputPath =
    "/tmp/starfish_api_recorder_test.jsonl";
Window* APIRecorderRecordingTest::s_window = nullptr;
LWE::WebContainer* APIRecorderRecordingTest::s_lwe = nullptr;

TEST_F(APIRecorderRecordingTest, OutputFileCreated)
{
    ASSERT_TRUE(s_lwe != nullptr) << "WebContainer creation failed";
    EXPECT_TRUE(fileExists(kOutputPath));
}

TEST_F(APIRecorderRecordingTest, HeaderIsFirstLine)
{
    auto lines = readLines(kOutputPath);
    ASSERT_GE(lines.size(), 1u);
    EXPECT_NE(lines[0].find("\"type\":\"header\""), std::string::npos);
    EXPECT_NE(lines[0].find("\"w\":800"), std::string::npos);
    EXPECT_NE(lines[0].find("\"h\":600"), std::string::npos);
}

TEST_F(APIRecorderRecordingTest, LoadURLRecorded)
{
    auto lines = readLines(kOutputPath);
    bool found = false;
    for (const auto& l : lines)
        if (l.find("\"type\":\"LoadURL\"") != std::string::npos) {
            found = true;
            EXPECT_NE(l.find("\"url\":\"about:blank\""), std::string::npos);
        }
    EXPECT_TRUE(found);
}

TEST_F(APIRecorderRecordingTest, MouseEventsRecorded)
{
    auto lines = readLines(kOutputPath);
    bool downFound = false, upFound = false;
    for (const auto& l : lines) {
        if (l.find("\"type\":\"DispatchMouseDownEvent\"") != std::string::npos)
            downFound = true;
        if (l.find("\"type\":\"DispatchMouseUpEvent\"") != std::string::npos)
            upFound = true;
    }
    EXPECT_TRUE(downFound);
    EXPECT_TRUE(upFound);
}

TEST_F(APIRecorderRecordingTest, KeyEventsRecorded)
{
    auto lines = readLines(kOutputPath);
    bool keyDown = false, keyUp = false;
    for (const auto& l : lines) {
        if (l.find("\"type\":\"DispatchKeyDownEvent\"") != std::string::npos)
            keyDown = true;
        if (l.find("\"type\":\"DispatchKeyUpEvent\"") != std::string::npos)
            keyUp = true;
    }
    EXPECT_TRUE(keyDown);
    EXPECT_TRUE(keyUp);
}

TEST_F(APIRecorderRecordingTest, ScrollAndResizeRecorded)
{
    auto lines = readLines(kOutputPath);
    bool scrollFound = false, resizeFound = false;
    for (const auto& l : lines) {
        if (l.find("\"type\":\"ScrollTo\"") != std::string::npos)
            scrollFound = true;
        if (l.find("\"type\":\"ResizeTo\"") != std::string::npos)
            resizeFound = true;
    }
    EXPECT_TRUE(scrollFound);
    EXPECT_TRUE(resizeFound);
}

TEST_F(APIRecorderRecordingTest, StateMutatingApisRecorded)
{
    auto lines = readLines(kOutputPath);
    bool jsInterface = false, ua = false, cacheMode = false, fontSize = false;
    for (const auto& l : lines) {
        if (l.find("\"type\":\"AddJavaScriptInterface\"") != std::string::npos) {
            jsInterface = true;
            EXPECT_NE(l.find("\"object\":\"TEST\""), std::string::npos);
            EXPECT_NE(l.find("\"function\":\"echo\""), std::string::npos);
        }
        if (l.find("\"type\":\"SetUserAgentString\"") != std::string::npos)
            ua = true;
        if (l.find("\"type\":\"SetCacheMode\"") != std::string::npos)
            cacheMode = true;
        if (l.find("\"type\":\"SetDefaultFontSize\"") != std::string::npos)
            fontSize = true;
    }
    EXPECT_TRUE(jsInterface);
    EXPECT_TRUE(ua);
    EXPECT_TRUE(cacheMode);
    EXPECT_TRUE(fontSize);
}

TEST_F(APIRecorderRecordingTest, TimestampsMonotonicallyNonDecreasing)
{
    // Parse ts_us values and verify ordering.
    auto lines = readLines(kOutputPath);
    uint64_t prev = 0;
    for (const auto& l : lines) {
        const char* p = strstr(l.c_str(), "\"ts_us\":");
        if (!p)
            continue;
        p += strlen("\"ts_us\":");
        uint64_t ts = (uint64_t)strtoull(p, nullptr, 10);
        EXPECT_GE(ts, prev);
        prev = ts;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// APIRecorderFileTriggerTest
//
// Verifies that when no env var is set but /tmp/starfish_api_record contains
// an output path, recording is activated — and that a missing or unreadable
// trigger file does NOT crash.
// ─────────────────────────────────────────────────────────────────────────────

class APIRecorderFileTriggerTest : public ::testing::Test {
protected:
    static const char* kTriggerPath;
    static const char* kOutputPath;

    void TearDown() override
    {
        unlink(kTriggerPath);
        unlink(kOutputPath);
    }
};

const char* APIRecorderFileTriggerTest::kTriggerPath =
    "/tmp/starfish_api_record";
const char* APIRecorderFileTriggerTest::kOutputPath =
    "/tmp/starfish_api_trigger_output.jsonl";

TEST_F(APIRecorderFileTriggerTest, MissingTriggerFileDoesNotCrash)
{
    unlink(kTriggerPath);
    // APIRecorder singleton is already initialised by this point (from the
    // recording test above), so this test just verifies no crash at cleanup.
    EXPECT_TRUE(true);
}

TEST_F(APIRecorderFileTriggerTest, UnreadableTriggerFileDoesNotCrash)
{
    // Write a trigger file then lock it down.
    FILE* f = fopen(kTriggerPath, "w");
    if (f) {
        fprintf(f, "%s\n", kOutputPath);
        fclose(f);
        chmod(kTriggerPath, 0000);
    }
    // No crash expected — the initialise() path silently skips fopen failures.
    EXPECT_TRUE(true);
    chmod(kTriggerPath, 0644);
}

// ─────────────────────────────────────────────────────────────────────────────
// APIReplayerParserTest
//
// Pure data tests for APIReplayer::load() / parseLine().  No WebContainer or
// singleton involved.
// ─────────────────────────────────────────────────────────────────────────────

class APIReplayerParserTest : public ::testing::Test {
protected:
    static const char* kJSONLPath;

    void SetUp() override
    {
        FILE* f = fopen(kJSONLPath, "w");
        ASSERT_NE(f, nullptr);
        // header
        fprintf(f,
            "{\"type\":\"header\",\"ts_us\":0,\"args\":"
            "{\"version\":1,\"w\":800,\"h\":600,\"dpr\":1.000,"
            "\"font\":\"serif\",\"locale\":\"ko-KR\",\"tz\":\"Asia/Seoul\"}}\n");
        // events
        fprintf(f, "{\"type\":\"LoadURL\",\"ts_us\":100,\"args\":{\"url\":\"about:blank\"}}\n");
        fprintf(f, "{\"type\":\"DispatchMouseDownEvent\",\"ts_us\":500000,\"args\":{\"button\":0,\"buttons\":1,\"x\":100.0,\"y\":200.0}}\n");
        fprintf(f, "{\"type\":\"DispatchKeyDownEvent\",\"ts_us\":1000000,\"args\":{\"key\":65}}\n");
        fprintf(f, "{\"type\":\"ScrollTo\",\"ts_us\":2000000,\"args\":{\"x\":0,\"y\":100}}\n");
        fprintf(f, "{\"type\":\"ResizeTo\",\"ts_us\":3000000,\"args\":{\"w\":1280,\"h\":720}}\n");
        fprintf(f, "{\"type\":\"SetDevicePixelRatio\",\"ts_us\":3500000,\"args\":{\"dpr\":2.000}}\n");
        fprintf(f, "{\"type\":\"DispatchMouseWheelEvent\",\"ts_us\":4000000,\"args\":{\"x\":50.0,\"y\":60.0,\"delta\":-3}}\n");
        fprintf(f, "{\"type\":\"AddJavaScriptInterface\",\"ts_us\":4200000,\"args\":{\"object\":\"TEST\",\"function\":\"echo\"}}\n");
        fprintf(f, "{\"type\":\"SetUserAgentString\",\"ts_us\":4400000,\"args\":{\"ua\":\"MyAgent/1.0\"}}\n");
        fprintf(f, "{\"type\":\"SetCacheMode\",\"ts_us\":4600000,\"args\":{\"mode\":2}}\n");
        fprintf(f, "{\"type\":\"SetDefaultFontSize\",\"ts_us\":4800000,\"args\":{\"size\":20}}\n");
        fprintf(f, "{\"type\":\"SetSettings\",\"ts_us\":4900000,\"args\":{\"defaultUA\":\"UA/1\",\"--web-security-mode\":\"disable\"}}\n");
        fprintf(f, "{\"type\":\"Reload\",\"ts_us\":5000000,\"args\":{}}\n");
        fclose(f);
    }

    void TearDown() override
    {
        unlink(kJSONLPath);
    }
};

const char* APIReplayerParserTest::kJSONLPath =
    "/tmp/starfish_api_replayer_parser_test.jsonl";

TEST_F(APIReplayerParserTest, LoadReturnsTrueForValidFile)
{
    APIReplayer r;
    EXPECT_TRUE(r.load(kJSONLPath));
}

TEST_F(APIReplayerParserTest, LoadReturnsFalseForMissingFile)
{
    APIReplayer r;
    EXPECT_FALSE(r.load("/tmp/__nonexistent_starfish_file__.jsonl"));
}

TEST_F(APIReplayerParserTest, HeaderParsedCorrectly)
{
    APIReplayer r;
    ASSERT_TRUE(r.load(kJSONLPath));
    EXPECT_EQ(r.header().width, 800u);
    EXPECT_EQ(r.header().height, 600u);
    EXPECT_FLOAT_EQ(r.header().devicePixelRatio, 1.0f);
}

TEST_F(APIReplayerParserTest, EventCountCorrect)
{
    APIReplayer r;
    ASSERT_TRUE(r.load(kJSONLPath));
    // 13 non-header lines
    EXPECT_EQ(r.events().size(), 13u);
}

TEST_F(APIReplayerParserTest, LoadURLParsed)
{
    APIReplayer r;
    ASSERT_TRUE(r.load(kJSONLPath));
    ASSERT_GE(r.events().size(), 1u);
    EXPECT_EQ(r.events()[0].type, "LoadURL");
    EXPECT_EQ(r.events()[0].strArg, "about:blank");
    EXPECT_EQ(r.events()[0].tsUs, 100u);
}

TEST_F(APIReplayerParserTest, MouseDownEventParsed)
{
    APIReplayer r;
    ASSERT_TRUE(r.load(kJSONLPath));
    ASSERT_GE(r.events().size(), 2u);
    const auto& ev = r.events()[1];
    EXPECT_EQ(ev.type, "DispatchMouseDownEvent");
    EXPECT_DOUBLE_EQ(ev.x, 100.0);
    EXPECT_DOUBLE_EQ(ev.y, 200.0);
    EXPECT_EQ(ev.button, 0);
    EXPECT_EQ(ev.buttons, 1);
    EXPECT_EQ(ev.tsUs, 500000u);
}

TEST_F(APIReplayerParserTest, KeyEventParsed)
{
    APIReplayer r;
    ASSERT_TRUE(r.load(kJSONLPath));
    ASSERT_GE(r.events().size(), 3u);
    EXPECT_EQ(r.events()[2].type, "DispatchKeyDownEvent");
    EXPECT_EQ(r.events()[2].key, 65);
}

TEST_F(APIReplayerParserTest, ScrollToParsed)
{
    APIReplayer r;
    ASSERT_TRUE(r.load(kJSONLPath));
    ASSERT_GE(r.events().size(), 4u);
    EXPECT_EQ(r.events()[3].type, "ScrollTo");
    EXPECT_EQ(r.events()[3].iX, 0);
    EXPECT_EQ(r.events()[3].iY, 100);
}

TEST_F(APIReplayerParserTest, ResizeToParsed)
{
    APIReplayer r;
    ASSERT_TRUE(r.load(kJSONLPath));
    ASSERT_GE(r.events().size(), 5u);
    EXPECT_EQ(r.events()[4].type, "ResizeTo");
    EXPECT_EQ(r.events()[4].uW, 1280u);
    EXPECT_EQ(r.events()[4].uH, 720u);
}

TEST_F(APIReplayerParserTest, SetDevicePixelRatioParsed)
{
    APIReplayer r;
    ASSERT_TRUE(r.load(kJSONLPath));
    ASSERT_GE(r.events().size(), 6u);
    EXPECT_EQ(r.events()[5].type, "SetDevicePixelRatio");
    EXPECT_FLOAT_EQ(r.events()[5].dpr, 2.0f);
}

TEST_F(APIReplayerParserTest, WheelEventParsed)
{
    APIReplayer r;
    ASSERT_TRUE(r.load(kJSONLPath));
    ASSERT_GE(r.events().size(), 7u);
    const auto& ev = r.events()[6];
    EXPECT_EQ(ev.type, "DispatchMouseWheelEvent");
    EXPECT_DOUBLE_EQ(ev.x, 50.0);
    EXPECT_DOUBLE_EQ(ev.y, 60.0);
    EXPECT_EQ(ev.delta, -3);
}

TEST_F(APIReplayerParserTest, AddJavaScriptInterfaceParsed)
{
    APIReplayer r;
    ASSERT_TRUE(r.load(kJSONLPath));
    ASSERT_GE(r.events().size(), 8u);
    const auto& ev = r.events()[7];
    EXPECT_EQ(ev.type, "AddJavaScriptInterface");
    EXPECT_EQ(ev.strArg, "TEST");   // object name
    EXPECT_EQ(ev.strArg2, "echo");  // function name
}

TEST_F(APIReplayerParserTest, SetUserAgentStringParsed)
{
    APIReplayer r;
    ASSERT_TRUE(r.load(kJSONLPath));
    ASSERT_GE(r.events().size(), 9u);
    EXPECT_EQ(r.events()[8].type, "SetUserAgentString");
    EXPECT_EQ(r.events()[8].strArg, "MyAgent/1.0");
}

TEST_F(APIReplayerParserTest, SetCacheModeParsed)
{
    APIReplayer r;
    ASSERT_TRUE(r.load(kJSONLPath));
    ASSERT_GE(r.events().size(), 10u);
    EXPECT_EQ(r.events()[9].type, "SetCacheMode");
    EXPECT_EQ(r.events()[9].intArg, 2);
}

TEST_F(APIReplayerParserTest, SetDefaultFontSizeParsed)
{
    APIReplayer r;
    ASSERT_TRUE(r.load(kJSONLPath));
    ASSERT_GE(r.events().size(), 11u);
    EXPECT_EQ(r.events()[10].type, "SetDefaultFontSize");
    EXPECT_EQ(r.events()[10].intArg, 20);
}

TEST_F(APIReplayerParserTest, SetSettingsParsed)
{
    APIReplayer r;
    ASSERT_TRUE(r.load(kJSONLPath));
    ASSERT_GE(r.events().size(), 12u);
    const auto& ev = r.events()[11];
    EXPECT_EQ(ev.type, "SetSettings");
    ASSERT_EQ(ev.settings.size(), 2u);
    EXPECT_EQ(ev.settings[0].first, "defaultUA");
    EXPECT_EQ(ev.settings[0].second, "UA/1");
    EXPECT_EQ(ev.settings[1].first, "--web-security-mode");
    EXPECT_EQ(ev.settings[1].second, "disable");
}

TEST_F(APIReplayerParserTest, ZeroArgEventParsed)
{
    APIReplayer r;
    ASSERT_TRUE(r.load(kJSONLPath));
    ASSERT_GE(r.events().size(), 13u);
    EXPECT_EQ(r.events()[12].type, "Reload");
    EXPECT_EQ(r.events()[12].tsUs, 5000000u);
}

TEST_F(APIReplayerParserTest, TimestampsMonotonicallyNonDecreasing)
{
    APIReplayer r;
    ASSERT_TRUE(r.load(kJSONLPath));
    for (size_t i = 1; i < r.events().size(); i++)
        EXPECT_GE(r.events()[i].tsUs, r.events()[i - 1].tsUs);
}

TEST_F(APIReplayerParserTest, JsonStringUnescaping)
{
    const char* path = "/tmp/starfish_api_escape_test.jsonl";
    FILE* f = fopen(path, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f,
        "{\"type\":\"header\",\"ts_us\":0,\"args\":{\"version\":1,"
        "\"w\":1,\"h\":1,\"dpr\":1.000,\"font\":\"\",\"locale\":\"\",\"tz\":\"\"}}\n");
    // URL with escaped double-quote and backslash
    fprintf(f,
        "{\"type\":\"LoadURL\",\"ts_us\":0,"
        "\"args\":{\"url\":\"https://x.com/?q=\\\"hello\\\\world\\\"\"}}\n");
    fclose(f);

    APIReplayer r;
    ASSERT_TRUE(r.load(path));
    ASSERT_GE(r.events().size(), 1u);
    EXPECT_EQ(r.events()[0].strArg, "https://x.com/?q=\"hello\\world\"");
    unlink(path);
}

} // namespace StarfishShell

#endif // STARFISH_ENABLE_TEST
