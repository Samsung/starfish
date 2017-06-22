/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLCollection.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBox.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"

#if defined(PORT_GRAPHIC_BACKEND_DALI)
#include <dali-toolkit/dali-toolkit.h>
#endif

#include <EscargotPublic.h>
using namespace Escargot;

namespace StarFish {

void customExit(int returnCode, Window* window)
{
#if defined(PORT_GRAPHIC_BACKEND_EFL)
    exit(returnCode);
#elif defined(PORT_GRAPHIC_BACKEND_DALI)
    Dali::Application* app =
        (Dali::Application*)window->starFish()->nativeHandle();
    if (app) {
        app->Quit();
    } else {
        exit(returnCode);
    }
#endif
}

ValueRef* windowWindowGetterFunction(ExecutionStateRef* state,
                                     ValueRef* thisValue, size_t argc,
                                     ValueRef** argv, bool isNewExpression)
{
    return ValueRef::create(state->context()->globalObject());
}

static void timeoutHandler(Window* wnd, void* data)
{
    FunctionObjectRef* fn = (FunctionObjectRef*)data;
    callScriptFunction(wnd->scriptBindingInstance(), ValueRef::create(fn),
                       nullptr, 0, scriptUndefined());
}

// TODO : Pass "any... arguments" if exist
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
    int32_t value1 = 0;
    if (argc > 1) {
        value1 = argv[1]->toInt32(state);
    }

    if (argv[0]->isFunction()) {
        return ValueRef::create(
            window->setTimeout(timeoutHandler, value1, argv[0]->asObject()));
    } else {
        String* bodyStr = toBrowserString(state, argv[0]);
        String* name[] = { String::emptyString };
        bool error = false;
        ScriptValue m_listener = createScriptFunction(
            window->scriptBindingInstance(), name, 1, bodyStr, error);
        return ValueRef::create(
            window->setTimeout(timeoutHandler, value1, m_listener));
    }
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
    ValueRef* arg0 = argv[0];
    ValueRef* arg1 = argv[1];
    ValueRef* arg2 = argv[2];

    // Handle argument arg2
    ScriptValue value2;
    value2 = arg2;
    // Handle argument arg1
    int32_t value1 = 0;
    if (!arg1->isUndefinedOrNull()) {
        value1 = arg1->toInt32(state);
    }

    if (arg0->isFunction()) {
        return ValueRef::create(
            window->setInterval(timeoutHandler, value1, arg0->asObject()));
    } else {
        String* bodyStr = toBrowserString(state, arg0);
        String* name[] = { String::emptyString };
        bool error = false;
        ScriptValue m_listener = createScriptFunction(
            window->scriptBindingInstance(), name, 1, bodyStr, error);
        return ValueRef::create(
            window->setInterval(timeoutHandler, value1, m_listener));
    }
}

static void animationFrameTimeoutHandler(Window* wnd, void* data)
{
    FunctionObjectRef* fn = (FunctionObjectRef*)data;
    callScriptFunction(wnd->scriptBindingInstance(), ValueRef::create(fn),
                       nullptr, 0, scriptUndefined());
}

// TODO : Pass "any... arguments" if exist
// TODO : First argument can be function or script source (currently allow
// function only)
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

    if (arg0->isFunction()) {
        return ValueRef::create(
            window->requestAnimationFrame(animationFrameTimeoutHandler, arg0));
    }

    return scriptUndefined();
}

#ifdef STARFISH_ENABLE_TEST
static ValueRef* debugPauseFunction(ExecutionStateRef* state,
                                    ValueRef* thisValue, size_t argc,
                                    ValueRef** argv, bool isNewExpression)
{
    GENERATE_WINDOW();

    window->starFish()->messageLoop()->addIdlerWithNoScriptInstanceEntering(
        [](size_t, void* data, void*) {
            StarFish* sf = (StarFish*)data;
            sf->pause();
        },
        window->starFish(), nullptr);
    return scriptUndefined();
}

static ValueRef* debugResumeFunction(ExecutionStateRef* state,
                                     ValueRef* thisValue, size_t argc,
                                     ValueRef** argv, bool isNewExpression)
{
    GENERATE_WINDOW();

    window->starFish()->messageLoop()->addIdlerWithNoScriptInstanceEntering(
        [](size_t, void* data, void*) {
            StarFish* sf = (StarFish*)data;
            sf->resume();
        },
        window->starFish(), nullptr);
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

static void screenShotTimeoutHandler(Window* wnd, void* data)
{
    FunctionObjectRef* p = (FunctionObjectRef*)data;
    callScriptFunction(
        wnd->scriptBindingInstance(), ValueRef::create(p), nullptr, 0,
        ValueRef::create(
            wnd->scriptBindingInstance()->scriptContext()->globalObject()));
}

static ValueRef* screenShotFunction(ExecutionStateRef* state,
                                    ValueRef* thisValue, size_t argc,
                                    ValueRef** argv, bool isNewExpression)
{
    GENERATE_WINDOW();

    std::string path = window->document()->documentURI()->baseURI()->utf8Data();
    path = path.substr(strlen("file://"));
    path += argv[0]->toString(state)->toStdUTF8String().data();
    window->screenShot(path);
    window->setTimeout(screenShotTimeoutHandler, 100, argv[1]);
    return ValueRef::createUndefined();
}

static ValueRef* screenShotRelativePathFunction(ExecutionStateRef* state,
                                                ValueRef* thisValue,
                                                size_t argc, ValueRef** argv,
                                                bool isNewExpression)
{
    GENERATE_WINDOW();

    char buff[1024];
    getcwd(buff, 1024);
    String* path =
        String::fromUTF8(buff)
            ->concat(String::fromUTF8("/"))
            ->concat(String::fromUTF8(
                getenv("SCREEN_SHOT_FILE") ? getenv("SCREEN_SHOT_FILE") : ""));
    window->screenShot(path->utf8Data());
    callScriptFunction(
        window->scriptBindingInstance(), argv[0], nullptr, 0,
        ValueRef::create(
            window->scriptBindingInstance()->scriptContext()->globalObject()));
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

    window->browsingContext()->renderingIfNeeds();

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
        result->set(state, ValueRef::create(StringRef::fromASCII("x")),
                    ValueRef::create(rect.x().toFloat()));
        result->set(state, ValueRef::create(StringRef::fromASCII("y")),
                    ValueRef::create(rect.y().toFloat()));
        result->set(state, ValueRef::create(StringRef::fromASCII("width")),
                    ValueRef::create(rect.width().toFloat()));
        result->set(state, ValueRef::create(StringRef::fromASCII("height")),
                    ValueRef::create(rect.height().toFloat()));
        return ValueRef::create(result);
    } else {
        // TODO
    }

    return scriptUndefined();
}

static ValueRef* simulateClickFunction(ExecutionStateRef* state,
                                       ValueRef* thisValue, size_t argc,
                                       ValueRef** argv, bool isNewExpression)
{
    GENERATE_WINDOW();

    ValueRef* arg0 = argv[0];
    ValueRef* arg1 = argv[1];
    double value0 = arg0->toNumber(state);
    double value1 = arg1->toNumber(state);

    window->simulateClick(value0, value1);
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

static ValueRef* testAssertFunction(ExecutionStateRef* state,
                                    ValueRef* thisValue, size_t argc,
                                    ValueRef** argv, bool isNewExpression)
{
    GENERATE_WINDOW();

    if (argv[0]->isString()) {
        bool su = false;
        String* scriptString = toBrowserString(state, argv[0]);
        ScriptValue result = evaluateString(
            window->scriptBindingInstance(), scriptString,
            String::createASCIIString("testAssertFunction"), &su);
        if (su && result->toBoolean(state)) {
            return scriptUndefined();
        }
        std::string errString = "[FAIL]assertion fail";
        STARFISH_LOG_ERROR("%s\n", errString.data());
        customExit(-1, window);
    } else {
        if (argv[0]->toBoolean(state)) {
        } else {
            std::string errString = "[FAIL]assertion fail";
            STARFISH_LOG_ERROR("%s\n", errString.data());
            customExit(-1, window);
        }
    }
    return scriptUndefined();
}

static ValueRef* testEndFunction(ExecutionStateRef* state, ValueRef* thisValue,
                                 size_t argc, ValueRef** argv,
                                 bool isNewExpression)
{
    puts("[PASS]");
    STARFISH_LOG_ERROR("%s\n", "[PASS]");
    GC_gcollect_and_unmap();
    GC_gcollect_and_unmap();
    GC_gcollect_and_unmap();
    GC_gcollect_and_unmap();
    GC_gcollect_and_unmap();
    GENERATE_WINDOW();
    customExit(0, window);

    return scriptUndefined();
}
#endif

static ValueRef* testImgDiffFunction(ExecutionStateRef* state,
                                     ValueRef* thisValue, size_t argc,
                                     ValueRef** argv, bool isNewExpression)
{
    GENERATE_WINDOW();

    std::string cmd = "./tool/imgdiff/imgdiff ";

    Window* wnd = window;
    std::string path = wnd->document()->documentURI()->baseURI()->utf8Data();
    path = path.substr(strlen("file://"));

    cmd += path;
    cmd += argv[0]->toString(state)->toStdUTF8String().data();
    cmd += " ";
    cmd += path;
    cmd += argv[1]->toString(state)->toStdUTF8String().data();

    STARFISH_LOG_INFO("%s\n", cmd.c_str());
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

    if (output.find("failed") != std::string::npos) {
        cmd = "test/tool/image_diff --diff ";
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
            RELEASE_ASSERT_NOT_REACHED();
        }

        std::string output;
        while ((ch = fgetc(fp)) != EOF) {
            output += ch;
        }

        STARFISH_LOG_ERROR("%s\n", "[FAIL]testImgDiff fail");
        customExit(-1, window);
    }

    pclose(fp);
    return scriptUndefined();
}

static ValueRef* virtualIdentifierCallback(ExecutionStateRef* state,
                                           ValueRef* key)
{
    Window* self = fetchWindow(state->context());

    auto callee = state->resolveCallee();
    if (callee) {
        void* data = callee->asObject()->extraData();
        if (data) {
            ScriptWrappable* w = (ScriptWrappable*)data;
            if (w->isAttributeEventFunction()) {
                auto elementDOMObject =
                    ((AttributeEventFunction*)w)->element()->scriptValue();
                if (elementDOMObject->isObject()) {
                    bool exist = elementDOMObject->asObject()->hasOwnProperty(
                        state, key);
                    if (exist) {
                        return elementDOMObject->asObject()->getOwnProperty(
                            state, key);
                    }
                }
            }
        }
    }

    String* name = toBrowserString(state, key);
    HTMLCollection* coll = self->namedAccess(name);
    if (coll) {
        if (coll->length()) {
            if (coll->length() > 1) {
                return coll->scriptValue();
            } else {
                return coll->item(0)->scriptValue();
            }
        }
    }

    return ValueRef::createEmpty();
}

void Window::postInit(ScriptBindingInstance* instance)
{
    ContextRef* context = instance->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(context);

    context->setVirtualIdentifierCallback(virtualIdentifierCallback);

#ifdef STARFISH_ENABLE_TEST
#define DEFINE_TEST_FUNCTION(name, length)                              \
    scriptObject()->defineDataProperty(                                 \
        state, ValueRef::create(StringRef::fromASCII(#name "")),        \
        ValueRef::create(FunctionObjectRef::create(                     \
            state, FunctionObjectRef::NativeFunctionInfo(               \
                       AtomicStringRef::create(context, #name ""),      \
                       name##Function, length, nullptr, true, false))), \
        true, true, true);

    DEFINE_TEST_FUNCTION(debugPause, 0);
    DEFINE_TEST_FUNCTION(debugResume, 0);
    DEFINE_TEST_FUNCTION(networkEnable, 0);
    DEFINE_TEST_FUNCTION(networkDisable, 0);
    DEFINE_TEST_FUNCTION(isPixelTest, 0);
    DEFINE_TEST_FUNCTION(screenShot, 2);
    DEFINE_TEST_FUNCTION(screenShotRelativePath, 2);
    DEFINE_TEST_FUNCTION(forceDisableOnloadCapture, 0);
    DEFINE_TEST_FUNCTION(getXYWH, 1);
    DEFINE_TEST_FUNCTION(simulateClick, 2);
    DEFINE_TEST_FUNCTION(simulateVisibilitychange, 1);
    DEFINE_TEST_FUNCTION(testAssert, 1);
    DEFINE_TEST_FUNCTION(testEnd, 0);
    DEFINE_TEST_FUNCTION(testImgDiff, 2);

#endif
}
}
