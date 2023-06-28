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
#include "platform/window/PlatformWindow.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"

#include <EscargotPublic.h>
using namespace Escargot;

#ifdef STARFISH_ENABLE_TEST
#include <signal.h>
extern int g_exitCode;
void customExit(int returnCode)
{
    fflush(stdout);
    fflush(stderr);

// TODO enable this every port
// --hide-window + EFL window is not working correctly
// because EFL throws error
#ifdef PORT_WEBVIEW_BRIDGE_GLFW
    g_exitCode = returnCode;
    raise(SIGINT);
    exit(returnCode);
#else
    exit(returnCode);
#endif
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
    GCVector<ScriptValue> value2;
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
            ScriptValue itemNV;

            itemNV = itemJS;
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
            PlatformWindow* window = (PlatformWindow*)data;
            window->pause();
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
            PlatformWindow* window = (PlatformWindow*)data;
            window->resume();
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
            DEFINE_TEST_FUNCTION(screenShot, 2);
            DEFINE_TEST_FUNCTION(screenShotRelativePath, 2);
            DEFINE_TEST_FUNCTION(forceDisableOnloadCapture, 0);
            DEFINE_TEST_FUNCTION(getXYWH, 1);
            DEFINE_TEST_FUNCTION(simulateClick, 2);
            DEFINE_TEST_FUNCTION(simulateMouseDown, 2);
            DEFINE_TEST_FUNCTION(simulateMouseUp, 2);
            DEFINE_TEST_FUNCTION(simulateVisibilitychange, 1);
            DEFINE_TEST_FUNCTION(testAssert, 1);
            DEFINE_TEST_FUNCTION(testEnd, 0);
            DEFINE_TEST_FUNCTION(testImgDiff, 2);
            DEFINE_TEST_FUNCTION(wptTestEnd, 0);

#endif

            return ValueRef::createUndefined();
        },
        this);

    m_object = window()->scriptObject();
}
} // namespace Starfish
