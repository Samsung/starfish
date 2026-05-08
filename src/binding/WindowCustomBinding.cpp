/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "PlatformIntegrationData.h"
#include "binding/ScriptBindingInstance.h"
#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/MessageEvent.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBox.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/renderer/Renderer.h"
#include "core/modules/sharedworker/SharedWorkerProcessManager.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"

#include <EscargotPublic.h>
using namespace Escargot;

#ifdef STARFISH_ENABLE_TEST
#include <png.h>
#include <fstream>
#include <vector>
#endif

#ifdef STARFISH_ENABLE_TEST
extern int32_t g_renderingCount;
static bool g_gotFailure = false;
void starfishRecordTestFailure()
{
    g_gotFailure = true;
}

#include <signal.h>
void customExit(int returnCode)
{
    fflush(stdout);
    fflush(stderr);

    if (getenv("DISABLE_TEST_EXIT")) {
        return;
    }

    int prevExitCode = 0;
    if (getenv("EXIT_CODE")) {
        prevExitCode = std::atoi(getenv("EXIT_CODE"));
    }

    if (prevExitCode == 0) {
        std::string exitCode = std::to_string(returnCode);
        setenv("EXIT_CODE", exitCode.c_str(), 1);
    }
    if (returnCode) {
        starfishRecordTestFailure();
    }
    raise(SIGINT);
}
#endif

namespace Starfish {

struct TimeOutData : public gc {
    TimeOutData(GlobalScope* globalScope)
        : listener(nullptr)
        , globalScope(globalScope)
    {
    }
    void* listener;
    GCVector<ScriptValue> argVector;
    GlobalScope* globalScope;
};

struct ScreenShotTimeOutData : public gc {
    Window* window;
    ValueRef* arg;
};

static void timeoutHandler(void* data)
{
    TimeOutData* td = (TimeOutData*)data;
    FunctionObjectRef* fn = (FunctionObjectRef*)td->listener;
    ScriptBindingInstance* instance =
        td->globalScope->executionContext()->scriptBindingInstance();

    callScriptFunction(instance, ValueRef::create(fn), td->argVector.data(),
                       td->argVector.size(), scriptUndefined());
}

ValueRef* setTimeoutWindowFunction(ExecutionStateRef* state,
                                   ValueRef* thisValue, size_t argc,
                                   ValueRef** argv, bool isNewExpression)
{
    GENERATE_WINDOW();
    size_t argCount = argc;
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "setTimeout", "Window", reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    int32_t result;
    TimeOutData* td = new TimeOutData(window);
    ValueRef* arg1 = (argc > 1) ? argv[1] : ValueRef::createUndefined();

    // Handle ellipsis arguments from index2
    for (size_t i = 2; i < argCount; i++) {
        td->argVector.push_back(argv[i]);
    }
    // Handle argument arg1
    int32_t value1 = 0;
    if (!arg1->isUndefinedOrNull()) {
        value1 = arg1->toInt32(state);
    }
    // Handle argument arg0
    if (argv[0]->isCallable()) {
        td->listener = argv[0]->asObject();
    } else {
        if (!window->checkSecurityPolicy()) {
            return ValueRef::create(0);
        }
        String* bodyStr = toBrowserString(state, argv[0]);
        String* name[] = { String::emptyString };
        bool error = false;
        td->listener = createScriptFunction(window->scriptBindingInstance(),
                                            name, 1, bodyStr, error);
    }

    // Call native function (nargs: 3)
    result = window->setTimeout(timeoutHandler, value1, td);

    // Return ValueRef* from native value
    return ValueRef::create(result);
}

ValueRef* setIntervalWindowFunction(ExecutionStateRef* state,
                                    ValueRef* thisValue, size_t argc,
                                    ValueRef** argv, bool isNewExpression)
{
    GENERATE_WINDOW();
    size_t argCount = argc;
    if (argCount < 2) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "2", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "setInterval", "Window",
                        reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    int32_t result;
    TimeOutData* td = new TimeOutData(window);
    ValueRef* arg1 = (argc > 1) ? argv[1] : ValueRef::createUndefined();

    // Handle ellipsis arguments from index2
    for (size_t i = 2; i < argCount; i++) {
        td->argVector.push_back(argv[i]);
    }
    // Handle argument arg1
    int32_t value1 = 0;
    if (!arg1->isUndefinedOrNull()) {
        value1 = arg1->toInt32(state);
    }
    // Handle argument arg0
    if (argv[0]->isCallable()) {
        td->listener = argv[0]->asObject();
    } else {
        if (!window->checkSecurityPolicy()) {
            return ValueRef::create(0);
        }
        String* bodyStr = toBrowserString(state, argv[0]);
        String* name[] = { String::emptyString };
        bool error = false;
        td->listener = createScriptFunction(window->scriptBindingInstance(),
                                            name, 1, bodyStr, error);
    }

    // Call native function (nargs: 3)
    result = window->setInterval(timeoutHandler, value1, td);

    // Return ValueRef* from native value
    return ValueRef::create(result);
}

static void requestAnimationFrameHandler(void* data)
{
    TimeOutData* td = (TimeOutData*)data;
    ObjectRef* fn = (ObjectRef*)td->listener;

    double DOMHighResTimeStamp =
        (td->globalScope->webBase()->lastRenderingTick() -
         td->globalScope->executionContext()->createdTick()) /
        1000.0;
    GCVector<ScriptValue> newArgVector;
    newArgVector.reserve(1 + td->argVector.size());
    newArgVector.push_back(createScriptValue(DOMHighResTimeStamp));
    newArgVector.insert(newArgVector.end(), td->argVector.begin(),
                        td->argVector.end());
    ScriptBindingInstance* instance =
        td->globalScope->executionContext()->scriptBindingInstance();
    callScriptFunction(instance, ValueRef::create(fn), newArgVector.data(),
                       newArgVector.size(), scriptUndefined());
}

ValueRef* postMessageWindowFunction(ExecutionStateRef* state,
                                    ValueRef* thisValue, size_t argc,
                                    ValueRef** argv, bool isNewExpression)
{
    GENERATE_WINDOW();
    size_t argCount = argc;
    if (argCount < 2) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "2", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "postMessage", "Window",
                        reason);
        THROW_EXCEPTION(msg);
    }
    size_t validArgCount = 3;
    // Declare native value (empty when type is void)
    ValueRef* arg0 = argv[0];
    ValueRef* arg1 = argv[1];
    ValueRef* arg2 = (argc > 2) ? argv[2] : ValueRef::createUndefined();
    // Handle argument arg2
    GCVector<ScriptObject> value2;
    if (arg2->isUndefined()) {
        validArgCount--;
    } else {
        if (!(arg2->isObject() && arg2->asObject()->isArrayObject())) {
            THROW_EXCEPTION(ILLEGAL_INVOKE);
        }
        int arg2Size =
            (int)arg2->asObject()
                ->get(state,
                      ValueRef::create(StringRef::createFromASCII("length")))
                ->toNumber(state);
        for (int i = 0; i < arg2Size; i++) {
            ValueRef* itemJS =
                arg2->asObject()->get(state, ValueRef::create(i));
            ScriptObject itemNV;

            itemNV = itemJS->toObject(state);
            value2.push_back(itemNV);
        }
    }
    // Handle argument arg1
    String* value1 = String::emptyString;
    value1 = toBrowserString(state, arg1);
    // Handle argument arg0
    ScriptValue value0;
    value0 = arg0;
    // Call native function (nargs: 2-3)

    Window* lexicalGlobal =
        (Window*)state->resolveCallerLexicalGlobalObject()->extraData();

    try {
        if (validArgCount == 2) {
            window->postMessage(lexicalGlobal, value0, value1);
        } else {
            window->postMessage(lexicalGlobal, value0, value1, value2);
        }
    } catch (DOMException* e) {
        state->throwException(e->scriptValue());
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    // Return ValueRef* from native value
    return ValueRef::createUndefined();
}

ValueRef* requestAnimationFrameWindowFunction(ExecutionStateRef* state,
                                              ValueRef* thisValue, size_t argc,
                                              ValueRef** argv,
                                              bool isNewExpression)
{
    GENERATE_WINDOW();
    size_t argCount = argc;
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "requestAnimationFrame",
                        "Window", reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    uint32_t result;
    ValueRef* arg0 = argv[0];

    TimeOutData* td = new TimeOutData(window);
    // Handle ellipsis arguments from index1
    for (size_t i = 1; i < argCount; i++) {
        td->argVector.push_back(argv[i]);
    }
    // Handle argument arg0
    if (argv[0]->isCallable()) {
        td->listener = argv[0]->asObject();
    } else {
        String* bodyStr = toBrowserString(state, argv[0]);
        String* name[] = { String::emptyString };
        bool error = false;
        td->listener = createScriptFunction(window->scriptBindingInstance(),
                                            name, 1, bodyStr, error);
    }

    return ValueRef::create(
        window->requestAnimationFrame(requestAnimationFrameHandler, td));
}

#ifdef STARFISH_ENABLE_TEST
static ValueRef* debugPauseFunction(ExecutionStateRef* state,
                                    ValueRef* thisValue, size_t argc,
                                    ValueRef** argv, bool isNewExpression)
{
    GENERATE_WINDOW();

    window->webView()->messageLoop()->addIdler(
        window,
        [](size_t, void* data, void*) {
            Renderer* renderer = (Renderer*)data;
            renderer->pause();
        },
        window->webView(), nullptr);
    return scriptUndefined();
}

static ValueRef* debugResumeFunction(ExecutionStateRef* state,
                                     ValueRef* thisValue, size_t argc,
                                     ValueRef** argv, bool isNewExpression)
{
    GENERATE_WINDOW();

    window->webView()->messageLoop()->addIdler(
        window,
        [](size_t, void* data, void*) {
            Renderer* renderer = (Renderer*)data;
            renderer->resume();
        },
        window->webView(), nullptr);
    return scriptUndefined();
}

static ValueRef* networkEnableFunction(ExecutionStateRef* state,
                                       ValueRef* thisValue, size_t argc,
                                       ValueRef** argv, bool isNewExpression)
{
    GENERATE_WINDOW();

    window->setNetworkState(true);
    return scriptUndefined();
}

static ValueRef* networkDisableFunction(ExecutionStateRef* state,
                                        ValueRef* thisValue, size_t argc,
                                        ValueRef** argv, bool isNewExpression)
{
    GENERATE_WINDOW();

    window->setNetworkState(false);
    return scriptUndefined();
}

static ValueRef* webSecurityEnableFunction(ExecutionStateRef* state,
                                           ValueRef* thisValue, size_t argc,
                                           NULLABLE ValueRef** argv,
                                           bool isNewExpression)
{
    STARFISH_ASSERT(state != nullptr);
    STARFISH_ASSERT(thisValue != nullptr);

    GENERATE_WINDOW();

    window->webView()->setWebSecurityMode(LWE::WebSecurityMode::Enable);
    return scriptUndefined();
}

static ValueRef* webSecurityDisableFunction(ExecutionStateRef* state,
                                            ValueRef* thisValue, size_t argc,
                                            ValueRef** argv,
                                            bool isNewExpression)
{
    GENERATE_WINDOW();

    window->webView()->setWebSecurityMode(LWE::WebSecurityMode::Disable);
    return scriptUndefined();
}

static ValueRef* isPixelTestFunction(ExecutionStateRef* state,
                                     ValueRef* thisValue, size_t argc,
                                     ValueRef** argv, bool isNewExpression)
{
    GENERATE_WINDOW();

    if (getenv("PIXEL_TEST") && strlen(getenv("PIXEL_TEST"))) {
        return ValueRef::create(true);
    } else {
        return ValueRef::create(false);
    }
}

static ValueRef* renderingCountFunction(ExecutionStateRef* state,
                                        ValueRef* thisValue, size_t argc,
                                        ValueRef** argv, bool isNewExpression)
{
    return ValueRef::create(g_renderingCount);
}

static void screenShotTimeoutHandler(void* data)
{
    ScreenShotTimeOutData* std = static_cast<ScreenShotTimeOutData*>(data);
    ObjectRef* p = reinterpret_cast<ObjectRef*>(std->arg);
    ScriptBindingInstance* instance = std->window->scriptBindingInstance();
    callScriptFunction(
        instance, ValueRef::create(p), nullptr, 0,
        ValueRef::create(instance->scriptContext()->globalObject()));
}

static ValueRef* screenShotFunction(ExecutionStateRef* state,
                                    ValueRef* thisValue, size_t argc,
                                    ValueRef** argv, bool isNewExpression)
{
    GENERATE_WINDOW();

    UTF8StringDataNonGCStd path =
        window->document()->documentURI()->baseURI()->toUTF8NonGCString();
    path = path.substr(strlen("file://"));
    path += argv[0]->toString(state)->toStdUTF8String().data();

    ScreenShotTimeOutData* d = new ScreenShotTimeOutData();
    d->window = window;
    d->arg = argv[1];

    window->screenShot(
        path,
        [](void* data) {
            ScreenShotTimeOutData* d =
                static_cast<ScreenShotTimeOutData*>(data);
            d->window->setTimeout(screenShotTimeoutHandler, 1, d);
        },
        d);
    return ValueRef::createUndefined();
}

static ValueRef* screenShotRelativePathFunction(ExecutionStateRef* state,
                                                ValueRef* thisValue,
                                                size_t argc,
                                                NULLABLE ValueRef** argv,
                                                bool isNewExpression)
{
    STARFISH_ASSERT(state != nullptr);
    STARFISH_ASSERT(thisValue != nullptr);

    GENERATE_WINDOW();

    char buff[1024];
    getcwd(buff, 1024);

    const char* filePath =
        getenv("SCREEN_SHOT_FILE") ? getenv("SCREEN_SHOT_FILE") : "";

    String* path = String::fromUTF8(buff, strnlen(buff, sizeof(buff)))
                       ->concat(String::fromUTF8("/"))
                       ->concat(String::fromUTF8(filePath, strlen(filePath)));

    ScreenShotTimeOutData* d = new ScreenShotTimeOutData();
    d->window = window;
    d->arg = argv[0];

    window->screenShot(
        path->toUTF8NonGCString(),
        [](void* data) {
            ScreenShotTimeOutData* d =
                static_cast<ScreenShotTimeOutData*>(data);
            d->window->setTimeout(screenShotTimeoutHandler, 1, d);
        },
        d);

    return ValueRef::createUndefined();
}

static ValueRef* forceDisableOnloadCaptureFunction(ExecutionStateRef* state,
                                                   ValueRef* thisValue,
                                                   size_t argc, ValueRef** argv,
                                                   bool isNewExpression)
{
    GENERATE_WINDOW();

    window->forceDisableOnloadCapture();
    return ValueRef::createUndefined();
}

static ValueRef* getXYWHFunction(ExecutionStateRef* state, ValueRef* thisValue,
                                 size_t argc, ValueRef** argv,
                                 bool isNewExpression)
{
    GENERATE_WINDOW();

    window->browsingContext()->webView()->layoutIfNeeded(false);

    ValueRef* arg0 = argv[0];
    Node* value0 = nullptr;
    if (!arg0->isUndefinedOrNull()) {
        CHECK_TYPEOF(arg0, Node);

        value0 = (Node*)(arg0->asObject()->extraData());
    }
    Frame* fr = (Frame*)value0->frame();
    if (!fr) {
        return scriptNull();
    } else if (fr->isFrameBox()) {
        LayoutRect rect = fr->asFrameBox()->absoluteRect(
            value0->document()->frame()->asFrameBox());
        ObjectRef* result = ObjectRef::create(state);
        result->set(state, ValueRef::create(StringRef::createFromASCII("x")),
                    ValueRef::create(rect.x().toFloat()));
        result->set(state, ValueRef::create(StringRef::createFromASCII("y")),
                    ValueRef::create(rect.y().toFloat()));
        result->set(state,
                    ValueRef::create(StringRef::createFromASCII("width")),
                    ValueRef::create(rect.width().toFloat()));
        result->set(state,
                    ValueRef::create(StringRef::createFromASCII("height")),
                    ValueRef::create(rect.height().toFloat()));
        return ValueRef::create(result);
    } else {
        // TODO
    }

    return scriptUndefined();
}

static ValueRef* simulateClickFunction(ExecutionStateRef* state,
                                       ValueRef* thisValue, size_t argc,
                                       NULLABLE ValueRef** argv,
                                       bool isNewExpression)
{
    STARFISH_ASSERT(state != nullptr);
    STARFISH_ASSERT(thisValue != nullptr);

    GENERATE_WINDOW();

    ValueRef* arg0 = argv[0];
    ValueRef* arg1 = argv[1];
    double value0 = arg0->toNumber(state);
    double value1 = arg1->toNumber(state);

    window->simulateClick(value0, value1);
    return scriptUndefined();
}

static ValueRef* simulateMouseDownFunction(ExecutionStateRef* state,
                                           ValueRef* thisValue, size_t argc,
                                           NULLABLE ValueRef** argv,
                                           bool isNewExpression)
{
    STARFISH_ASSERT(state != nullptr);
    STARFISH_ASSERT(thisValue != nullptr);

    GENERATE_WINDOW();

    ValueRef* arg0 = argv[0];
    ValueRef* arg1 = argv[1];
    double value0 = arg0->toNumber(state);
    double value1 = arg1->toNumber(state);

    window->simulateMouseDown(value0, value1);
    return scriptUndefined();
}

static ValueRef* simulateMouseUpFunction(ExecutionStateRef* state,
                                         ValueRef* thisValue, size_t argc,
                                         NULLABLE ValueRef** argv,
                                         bool isNewExpression)
{
    STARFISH_ASSERT(state != nullptr);
    STARFISH_ASSERT(thisValue != nullptr);

    GENERATE_WINDOW();

    ValueRef* arg0 = argv[0];
    ValueRef* arg1 = argv[1];
    double value0 = arg0->toNumber(state);
    double value1 = arg1->toNumber(state);

    window->simulateMouseUp(value0, value1);
    return scriptUndefined();
}

static ValueRef* simulateMouseMoveFunction(ExecutionStateRef* state,
                                           ValueRef* thisValue, size_t argc,
                                           NULLABLE ValueRef** argv,
                                           bool isNewExpression)
{
    GENERATE_WINDOW();

    ValueRef* arg0 = argv[0];
    ValueRef* arg1 = argv[1];
    double value0 = arg0->toNumber(state);
    double value1 = arg1->toNumber(state);

    window->simulateMouseMove(value0, value1);
    return scriptUndefined();
}

static ValueRef* simulateVisibilitychangeFunction(ExecutionStateRef* state,
                                                  ValueRef* thisValue,
                                                  size_t argc, ValueRef** argv,
                                                  bool isNewExpression)
{
    GENERATE_WINDOW();

    ValueRef* arg0 = argv[0];
    bool value0 = arg0->toBoolean(state);

    window->simulateVisibilitychange(value0);

    return scriptUndefined();
}

static bool gotTestAssert = false;

static ValueRef* testAssertFunction(ExecutionStateRef* state,
                                    ValueRef* thisValue, size_t argc,
                                    ValueRef** argv, bool isNewExpression)
{
    GENERATE_WINDOW();

    if (argv[0]->isString()) {
        bool su = false;
        String* scriptString = toBrowserString(state, argv[0]);
        ScriptValue result =
            evaluateString(window->scriptBindingInstance(), scriptString,
                           String::emptyString, &su);
        if (su && result->toBoolean(state)) {
            return scriptUndefined();
        }
        std::string errString = "[FAIL]assertion fail";
        STARFISH_LOG_ERROR("%s", errString.data());
        customExit(-1);
    } else {
        if (argv[0]->toBoolean(state)) {
        } else {
            std::string errString = "[FAIL]assertion fail";
            STARFISH_LOG_ERROR("%s", errString.data());
            customExit(-1);
        }
    }
    gotTestAssert = true;
    return scriptUndefined();
}

static ValueRef* testEndFunction(ExecutionStateRef* state, ValueRef* thisValue,
                                 size_t argc, ValueRef** argv,
                                 bool isNewExpression)
{
    if (g_gotFailure ||
        (argc > 0 && argv[0]->isBoolean() && argv[0]->isFalse())) {
        puts("[FAIL]");
        STARFISH_LOG_ERROR("[FAIL]");
        customExit(1);
        return scriptUndefined();
    }

    puts("[PASS]");
    STARFISH_LOG_ERROR("%s", "[PASS]");
    customExit(0);

    return scriptUndefined();
}

static ValueRef* wptTestEndFunction(ExecutionStateRef* state,
                                    ValueRef* thisValue, size_t argc,
                                    ValueRef** argv, bool isNewExpression)
{
    puts("wptTestEnd() called");

#if defined(STARFISH_ENABLE_SHARED_WORKER)
    // must notify the server that the connection has ended.
    SharedWorkerProcessManager::instance()->closeConnection();
#endif

    if (gotTestAssert) {
        puts("[PASS]");
        STARFISH_LOG_ERROR("%s", "[PASS]");
        customExit(0);
    }
    const char* hide = getenv("HIDE_WINDOW");
    if ((hide && strlen(hide))) {
        customExit(0);
    }
    return ValueRef::createUndefined();
}

static ValueRef* testImgDiffFunction(ExecutionStateRef* state,
                                     ValueRef* thisValue, size_t argc,
                                     ValueRef** argv, bool isNewExpression)
{
    GENERATE_WINDOW();

    std::string cmd = "./tool/imgdiff/imgdiff ";

    Window* wnd = window;
    UTF8StringDataNonGCStd path =
        wnd->document()->documentURI()->baseURI()->toUTF8NonGCString();
    path = path.substr(strlen("file://"));

    cmd += path;
    cmd += argv[0]->toString(state)->toStdUTF8String().data();
    cmd += " ";
    cmd += path;
    cmd += argv[1]->toString(state)->toStdUTF8String().data();

    STARFISH_LOG_INFO("%s", cmd.c_str());
    FILE* fp = popen(cmd.c_str(), "r");
    int ch;

    if (!fp) {
        RELEASE_ASSERT_NOT_REACHED();
    }

    std::string output;
    while ((ch = fgetc(fp)) != EOF) {
        output += ch;
    }

    STARFISH_LOG_INFO("%s", output.c_str());

    if (output.find("[imgdiff-fail]") != std::string::npos) {
        cmd = "test/tools/image_diff/image_diff --diff ";
        cmd += path;
        cmd += argv[0]->toString(state)->toStdUTF8String().data();
        cmd += " ";
        cmd += path;
        cmd += argv[1]->toString(state)->toStdUTF8String().data();
        cmd += " ";
        cmd += path;
        cmd += std::string(argv[0]->toString(state)->toStdUTF8String().data()) +
               "_diff.png";
        puts(cmd.c_str());

        FILE* fp = popen(cmd.c_str(), "r");
        int ch;

        if (!fp) {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }

        std::string output;
        while ((ch = fgetc(fp)) != EOF) {
            output += ch;
        }

        exit(-1);
    }

    pclose(fp);
    return scriptUndefined();
}

// Helper function to read PNG file and get pixel data
static bool readPNGPixelData(const std::string& filePath,
                             std::vector<uint8_t>& pixelData, uint32_t& width,
                             uint32_t& height)
{
    FILE* fp = fopen(filePath.c_str(), "rb");
    if (!fp) {
        STARFISH_LOG_ERROR("Failed to open PNG file: %s", filePath.c_str());
        return false;
    }

    // Check PNG signature
    unsigned char header[8];
    if (fread(header, 1, 8, fp) != 8) {
        fclose(fp);
        return false;
    }
    if (png_sig_cmp(header, 0, 8)) {
        STARFISH_LOG_ERROR("File is not a valid PNG: %s", filePath.c_str());
        fclose(fp);
        return false;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr,
                                             nullptr, nullptr);
    if (!png) {
        fclose(fp);
        return false;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, nullptr, nullptr);
        fclose(fp);
        return false;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        return false;
    }

    png_init_io(png, fp);
    png_set_sig_bytes(png, 8);
    png_read_info(png, info);

    width = png_get_image_width(png, info);
    height = png_get_image_height(png, info);
    png_byte colorType = png_get_color_type(png, info);
    png_byte bitDepth = png_get_bit_depth(png, info);

    // Convert to RGBA format
    if (bitDepth == 16) {
        png_set_strip_16(png);
    }
    if (colorType == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png);
    }
    if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8) {
        png_set_expand_gray_1_2_4_to_8(png);
    }
    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png);
    }
    if (colorType == PNG_COLOR_TYPE_RGB || colorType == PNG_COLOR_TYPE_GRAY ||
        colorType == PNG_COLOR_TYPE_PALETTE) {
        png_set_filler(png, 0xff, PNG_FILLER_AFTER);
    }
    if (colorType == PNG_COLOR_TYPE_GRAY ||
        colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png);
    }

    png_read_update_info(png, info);

    uint32_t rowbytes = png_get_rowbytes(png, info);
    pixelData.resize(rowbytes * height);

    std::vector<png_bytep> rowPointers(height);
    for (uint32_t i = 0; i < height; i++) {
        rowPointers[i] = &pixelData[i * rowbytes];
    }

    png_read_image(png, rowPointers.data());
    png_read_end(png, nullptr);

    png_destroy_read_struct(&png, &info, nullptr);
    fclose(fp);

    return true;
}

// getPixelColor(pngPath, x, y) -> returns {r, g, b, a}
static ValueRef* getPixelColorFunction(ExecutionStateRef* state,
                                       ValueRef* thisValue, size_t argc,
                                       ValueRef** argv, bool isNewExpression)
{
    GENERATE_WINDOW();

    if (argc < 3) {
        STARFISH_LOG_ERROR("getPixelColor requires 3 arguments: path, x, y");
        return scriptNull();
    }

    // Get file path
    UTF8StringDataNonGCStd basePath =
        window->document()->documentURI()->baseURI()->toUTF8NonGCString();
    basePath = basePath.substr(strlen("file://"));

    std::string filePath =
        basePath + argv[0]->toString(state)->toStdUTF8String().data();

    // Get x, y coordinates
    int32_t x = argv[1]->toInt32(state);
    int32_t y = argv[2]->toInt32(state);

    // Read PNG file
    std::vector<uint8_t> pixelData;
    uint32_t width, height;

    if (!readPNGPixelData(filePath, pixelData, width, height)) {
        STARFISH_LOG_ERROR("Failed to read PNG file: %s", filePath.c_str());
        return scriptNull();
    }

    // Check bounds
    if (x < 0 || x >= (int32_t)width || y < 0 || y >= (int32_t)height) {
        STARFISH_LOG_ERROR(
            "Coordinates out of bounds: (%d, %d), image size: (%u, %u)", x, y,
            width, height);
        return scriptNull();
    }

    // Get pixel color (BGRA format in memory)
    uint32_t rowbytes = width * 4;
    uint8_t* pixel = &pixelData[y * rowbytes + x * 4];

    // Create result object with r, g, b, a
    ObjectRef* result = ObjectRef::create(state);
#ifdef PORT_PIXEL_ORDER_RGBA
    result->set(state, ValueRef::create(StringRef::createFromASCII("r")),
                ValueRef::create((int)pixel[0]));
    result->set(state, ValueRef::create(StringRef::createFromASCII("g")),
                ValueRef::create((int)pixel[1]));
    result->set(state, ValueRef::create(StringRef::createFromASCII("b")),
                ValueRef::create((int)pixel[2]));
    result->set(state, ValueRef::create(StringRef::createFromASCII("a")),
                ValueRef::create((int)pixel[3]));
#else
    // BGRA format
    result->set(state, ValueRef::create(StringRef::createFromASCII("r")),
                ValueRef::create((int)pixel[2]));
    result->set(state, ValueRef::create(StringRef::createFromASCII("g")),
                ValueRef::create((int)pixel[1]));
    result->set(state, ValueRef::create(StringRef::createFromASCII("b")),
                ValueRef::create((int)pixel[0]));
    result->set(state, ValueRef::create(StringRef::createFromASCII("a")),
                ValueRef::create((int)pixel[3]));
#endif

    return ValueRef::create(result);
}

// checkPixelColor(pngPath, x, y, r, g, b, a, tolerance) -> returns true/false
// tolerance is optional, default is 0
static ValueRef* checkPixelColorFunction(ExecutionStateRef* state,
                                         ValueRef* thisValue, size_t argc,
                                         ValueRef** argv, bool isNewExpression)
{
    GENERATE_WINDOW();

    if (argc < 6) {
        STARFISH_LOG_ERROR(
            "checkPixelColor requires at least 6 arguments: path, x, y, r, g, "
            "b");
        return ValueRef::create(false);
    }

    // Get file path
    UTF8StringDataNonGCStd basePath =
        window->document()->documentURI()->baseURI()->toUTF8NonGCString();
    basePath = basePath.substr(strlen("file://"));

    std::string filePath =
        basePath + argv[0]->toString(state)->toStdUTF8String().data();

    // Get x, y coordinates
    int32_t x = argv[1]->toInt32(state);
    int32_t y = argv[2]->toInt32(state);

    // Get expected color values
    int32_t expectedR = argv[3]->toInt32(state);
    int32_t expectedG = argv[4]->toInt32(state);
    int32_t expectedB = argv[5]->toInt32(state);
    int32_t expectedA = (argc > 6) ? argv[6]->toInt32(state) : 255;
    int32_t tolerance = (argc > 7) ? argv[7]->toInt32(state) : 0;

    // Read PNG file
    std::vector<uint8_t> pixelData;
    uint32_t width, height;

    if (!readPNGPixelData(filePath, pixelData, width, height)) {
        STARFISH_LOG_ERROR("Failed to read PNG file: %s", filePath.c_str());
        return ValueRef::create(false);
    }

    // Check bounds
    if (x < 0 || x >= (int32_t)width || y < 0 || y >= (int32_t)height) {
        STARFISH_LOG_ERROR(
            "Coordinates out of bounds: (%d, %d), image size: (%u, %u)", x, y,
            width, height);
        return ValueRef::create(false);
    }

    // Get pixel color
    uint32_t rowbytes = width * 4;
    uint8_t* pixel = &pixelData[y * rowbytes + x * 4];

    int32_t actualR, actualG, actualB, actualA;
#ifdef PORT_PIXEL_ORDER_RGBA
    actualR = pixel[0];
    actualG = pixel[1];
    actualB = pixel[2];
    actualA = pixel[3];
#else
    // BGRA format
    actualR = pixel[2];
    actualG = pixel[1];
    actualB = pixel[0];
    actualA = pixel[3];
#endif

    // Compare with tolerance
    auto inRange = [tolerance](int32_t actual, int32_t expected) -> bool {
        return actual >= (expected - tolerance) &&
               actual <= (expected + tolerance);
    };

    bool match = inRange(actualR, expectedR) && inRange(actualG, expectedG) &&
                 inRange(actualB, expectedB) && inRange(actualA, expectedA);

    if (!match) {
        STARFISH_LOG_INFO(
            "Pixel color mismatch at (%d, %d): expected rgba(%d,%d,%d,%d), got "
            "rgba(%d,%d,%d,%d)",
            x, y, expectedR, expectedG, expectedB, expectedA, actualR, actualG,
            actualB, actualA);
    }

    return ValueRef::create(match);
}

// checkPixelColors(pngPath, coordinatesArray, tolerance) -> returns true/false
// coordinatesArray is an array of {x, y, r, g, b, a} objects
static ValueRef* checkPixelColorsFunction(ExecutionStateRef* state,
                                          ValueRef* thisValue, size_t argc,
                                          ValueRef** argv, bool isNewExpression)
{
    GENERATE_WINDOW();

    if (argc < 2) {
        STARFISH_LOG_ERROR(
            "checkPixelColors requires at least 2 arguments: path, "
            "coordinatesArray");
        return ValueRef::create(false);
    }

    // Get file path
    UTF8StringDataNonGCStd basePath =
        window->document()->documentURI()->baseURI()->toUTF8NonGCString();
    basePath = basePath.substr(strlen("file://"));

    std::string filePath =
        basePath + argv[0]->toString(state)->toStdUTF8String().data();

    // Get tolerance (optional)
    int32_t tolerance = (argc > 2) ? argv[2]->toInt32(state) : 0;

    // Read PNG file once
    std::vector<uint8_t> pixelData;
    uint32_t width, height;

    if (!readPNGPixelData(filePath, pixelData, width, height)) {
        STARFISH_LOG_ERROR("Failed to read PNG file: %s", filePath.c_str());
        return ValueRef::create(false);
    }

    // Get coordinates array
    if (!argv[1]->isObject() || !argv[1]->asObject()->isArrayObject()) {
        STARFISH_LOG_ERROR("Second argument must be an array");
        return ValueRef::create(false);
    }

    ObjectRef* coordsArray = argv[1]->asObject();
    int32_t arrayLength =
        coordsArray
            ->get(state, ValueRef::create(StringRef::createFromASCII("length")))
            ->toInt32(state);

    uint32_t rowbytes = width * 4;

    for (int32_t i = 0; i < arrayLength; i++) {
        ValueRef* item = coordsArray->get(state, ValueRef::create(i));
        if (!item->isObject()) {
            STARFISH_LOG_ERROR("Invalid coordinate object at index %d", i);
            return ValueRef::create(false);
        }

        ObjectRef* coordObj = item->asObject();
        int32_t x =
            coordObj
                ->get(state, ValueRef::create(StringRef::createFromASCII("x")))
                ->toInt32(state);
        int32_t y =
            coordObj
                ->get(state, ValueRef::create(StringRef::createFromASCII("y")))
                ->toInt32(state);
        int32_t expectedR =
            coordObj
                ->get(state, ValueRef::create(StringRef::createFromASCII("r")))
                ->toInt32(state);
        int32_t expectedG =
            coordObj
                ->get(state, ValueRef::create(StringRef::createFromASCII("g")))
                ->toInt32(state);
        int32_t expectedB =
            coordObj
                ->get(state, ValueRef::create(StringRef::createFromASCII("b")))
                ->toInt32(state);
        int32_t expectedA =
            coordObj
                ->get(state, ValueRef::create(StringRef::createFromASCII("a")))
                ->toInt32(state);

        // Check bounds
        if (x < 0 || x >= (int32_t)width || y < 0 || y >= (int32_t)height) {
            STARFISH_LOG_ERROR(
                "Coordinates out of bounds at index %d: (%d, %d), image size: "
                "(%u, %u)",
                i, x, y, width, height);
            return ValueRef::create(false);
        }

        // Get pixel color
        uint8_t* pixel = &pixelData[y * rowbytes + x * 4];

        int32_t actualR, actualG, actualB, actualA;
#ifdef PORT_PIXEL_ORDER_RGBA
        actualR = pixel[0];
        actualG = pixel[1];
        actualB = pixel[2];
        actualA = pixel[3];
#else
        actualR = pixel[2];
        actualG = pixel[1];
        actualB = pixel[0];
        actualA = pixel[3];
#endif

        // Compare with tolerance
        auto inRange = [tolerance](int32_t actual, int32_t expected) -> bool {
            return actual >= (expected - tolerance) &&
                   actual <= (expected + tolerance);
        };

        bool match = inRange(actualR, expectedR) &&
                     inRange(actualG, expectedG) &&
                     inRange(actualB, expectedB) && inRange(actualA, expectedA);

        if (!match) {
            STARFISH_LOG_INFO(
                "Pixel color mismatch at index %d (%d, %d): expected "
                "rgba(%d,%d,%d,%d), got rgba(%d,%d,%d,%d)",
                i, x, y, expectedR, expectedG, expectedB, expectedA, actualR,
                actualG, actualB, actualA);
            return ValueRef::create(false);
        }
    }

    return ValueRef::create(true);
}

// getImageSize(pngPath) -> returns {width, height}
static ValueRef* getImageSizeFunction(ExecutionStateRef* state,
                                      ValueRef* thisValue, size_t argc,
                                      ValueRef** argv, bool isNewExpression)
{
    GENERATE_WINDOW();

    if (argc < 1) {
        STARFISH_LOG_ERROR("getImageSize requires 1 argument: path");
        return scriptNull();
    }

    // Get file path
    UTF8StringDataNonGCStd basePath =
        window->document()->documentURI()->baseURI()->toUTF8NonGCString();
    basePath = basePath.substr(strlen("file://"));

    std::string filePath =
        basePath + argv[0]->toString(state)->toStdUTF8String().data();

    // Read PNG file
    std::vector<uint8_t> pixelData;
    uint32_t width, height;

    if (!readPNGPixelData(filePath, pixelData, width, height)) {
        STARFISH_LOG_ERROR("Failed to read PNG file: %s", filePath.c_str());
        return scriptNull();
    }

    // Create result object
    ObjectRef* result = ObjectRef::create(state);
    result->set(state, ValueRef::create(StringRef::createFromASCII("width")),
                ValueRef::create((int)width));
    result->set(state, ValueRef::create(StringRef::createFromASCII("height")),
                ValueRef::create((int)height));

    return ValueRef::create(result);
}
#endif

void Window::postInit(ScriptBindingInstance* instance)
{
    ContextRef* context = instance->scriptContext();

    Evaluator::execute(
        context,
        [](ExecutionStateRef* state, Window* self) -> ValueRef* {
            ContextRef* context = state->context();
#ifdef STARFISH_ENABLE_TEST
#define DEFINE_TEST_FUNCTION(name, length)                         \
    self->scriptObject()->defineDataProperty(                      \
        state, StringRef::createFromASCII(#name ""),               \
        FunctionObjectRef::create(                                 \
            state, FunctionObjectRef::NativeFunctionInfo(          \
                       AtomicStringRef::create(context, #name ""), \
                       name##Function, length, true, false)),      \
        true, true, true);

            DEFINE_TEST_FUNCTION(debugPause, 0);
            DEFINE_TEST_FUNCTION(debugResume, 0);
            DEFINE_TEST_FUNCTION(networkEnable, 0);
            DEFINE_TEST_FUNCTION(networkDisable, 0);
            DEFINE_TEST_FUNCTION(webSecurityEnable, 0);
            DEFINE_TEST_FUNCTION(webSecurityDisable, 0);
            DEFINE_TEST_FUNCTION(isPixelTest, 0);
            DEFINE_TEST_FUNCTION(renderingCount, 0);
            DEFINE_TEST_FUNCTION(screenShot, 2);
            DEFINE_TEST_FUNCTION(screenShotRelativePath, 2);
            DEFINE_TEST_FUNCTION(forceDisableOnloadCapture, 0);
            DEFINE_TEST_FUNCTION(getXYWH, 1);
            DEFINE_TEST_FUNCTION(simulateClick, 2);
            DEFINE_TEST_FUNCTION(simulateMouseDown, 2);
            DEFINE_TEST_FUNCTION(simulateMouseUp, 2);
            DEFINE_TEST_FUNCTION(simulateMouseMove, 2);
            DEFINE_TEST_FUNCTION(simulateVisibilitychange, 1);
            DEFINE_TEST_FUNCTION(testAssert, 1);
            DEFINE_TEST_FUNCTION(testEnd, 0);
            DEFINE_TEST_FUNCTION(testImgDiff, 2);
            DEFINE_TEST_FUNCTION(wptTestEnd, 0);
            // PNG pixel color check functions
            DEFINE_TEST_FUNCTION(getPixelColor, 3);
            DEFINE_TEST_FUNCTION(checkPixelColor, 8);
            DEFINE_TEST_FUNCTION(checkPixelColors, 3);
            DEFINE_TEST_FUNCTION(getImageSize, 1);

#endif

            return ValueRef::createUndefined();
        },
        this);

    m_object = window()->scriptObject();
}
} // namespace Starfish
