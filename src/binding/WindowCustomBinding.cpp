/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLCollection.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBox.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/window/Window.h"

namespace StarFish {

using namespace escargot;

ESValue windowWindowGetterFunction(ESVMInstance* instance)
{
    return instance->globalObject();
}

static void timeoutHandler(Window* wnd, void* data)
{
    ESFunctionObject* fn = (ESFunctionObject*)data;
    std::jmp_buf tryPosition;
    if (setjmp(ESVMInstance::currentInstance()->registerTryPos(&tryPosition)) ==
        0) {
        ESFunctionObject::call(ESVMInstance::currentInstance(), fn, ESValue(),
                               NULL, 0, false);
        ESVMInstance::currentInstance()->unregisterTryPos(&tryPosition);
    } else {
        ESValue err = ESVMInstance::currentInstance()->getCatchedError();
        STARFISH_LOG_INFO("Uncaught %s\n", err.toString()->utf8Data());
    }
}

// TODO : Pass "any... arguments" if exist
ESValue setTimeoutWindowFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 2) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "2", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "setTimeout", "Window", reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    int32_t result;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    ESValue arg2 = instance->currentExecutionContext()->readArgument(2);

    // Handle argument arg2
    ScriptValue value2;
    value2 = arg2;
    // Handle argument arg1
    int32_t value1 = 0;
    if (!arg1.isUndefinedOrNull()) {
        value1 = arg1.toInt32();
    }

    if (arg0.isFunction()) {
        return ESValue(
            window->setTimeout(timeoutHandler, value1, arg0.asESPointer()));
    } else {
        String* bodyStr = toBrowserString(arg0.toString());
        String* name[] = { String::emptyString };
        bool error = false;
        ScriptValue m_listener = createScriptFunction(name, 1, bodyStr, error);
        return ESValue(window->setTimeout(timeoutHandler, value1,
                                          m_listener.asESPointer()));
    }
}

ESValue setIntervalWindowFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    size_t argCount = instance->currentExecutionContext()->argumentCount();
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
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    ESValue arg2 = instance->currentExecutionContext()->readArgument(2);

    // Handle argument arg2
    ScriptValue value2;
    value2 = arg2;
    // Handle argument arg1
    int32_t value1 = 0;
    if (!arg1.isUndefinedOrNull()) {
        value1 = arg1.toInt32();
    }

    if (arg0.isFunction()) {
        return ESValue(
            window->setInterval(timeoutHandler, value1, arg0.asESPointer()));
    } else {
        String* bodyStr = toBrowserString(arg0.toString());
        String* name[] = { String::emptyString };
        bool error = false;
        ScriptValue m_listener = createScriptFunction(name, 1, bodyStr, error);
        return ESValue(window->setInterval(timeoutHandler, value1,
                                           m_listener.asESPointer()));
    }
}

static void animationFrameTimeoutHandler(Window* wnd, void* data)
{
    ESFunctionObject* fn = (ESFunctionObject*)data;
    std::jmp_buf tryPosition;
    if (setjmp(ESVMInstance::currentInstance()->registerTryPos(&tryPosition)) ==
        0) {
        ESFunctionObject::call(ESVMInstance::currentInstance(), fn, ESValue(),
                               NULL, 0, false);
        ESVMInstance::currentInstance()->unregisterTryPos(&tryPosition);
    } else {
        std::jmp_buf tryPosition;
        ESValue err = ESVMInstance::currentInstance()->getCatchedError();
        if (setjmp(ESVMInstance::currentInstance()->registerTryPos(
                &tryPosition)) == 0) {
            STARFISH_LOG_INFO("Uncaught %s\n", err.toString()->utf8Data());
            ESVMInstance::currentInstance()->unregisterTryPos(&tryPosition);
        } else {
            STARFISH_LOG_INFO("Uncaught Error\n");
        }
    }
}

// TODO : Pass "any... arguments" if exist
// TODO : First argument can be function or script source (currently allow
// function only)
ESValue requestAnimationFrameWindowFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    size_t argCount = instance->currentExecutionContext()->argumentCount();
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
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isFunction()) {
        return ESValue(window->requestAnimationFrame(
            animationFrameTimeoutHandler, arg0.asESPointer()));
    }

    return ESValue();
}

#ifdef STARFISH_ENABLE_TEST
static ESValue debugPauseFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();

    window->starFish()->messageLoop()->addIdlerWithNoScriptInstanceEntering(
        [](size_t, void* data, void*) {
            StarFish* sf = (StarFish*)data;
            sf->pause();
        },
        window->starFish(), nullptr);
    return ESValue();
}

static ESValue debugResumeFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();

    window->starFish()->messageLoop()->addIdlerWithNoScriptInstanceEntering(
        [](size_t, void* data, void*) {
            StarFish* sf = (StarFish*)data;
            sf->resume();
        },
        window->starFish(), nullptr);
    return ESValue();
}

static ESValue networkEnableFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();

    window->setNetworkState(true);
    return ESValue();
}

static ESValue networkDisableFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();

    window->setNetworkState(false);
    return ESValue();
}

static ESValue isPixelTestFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();

    if (getenv("PIXEL_TEST") && strlen(getenv("PIXEL_TEST"))) {
        return ESValue(ESValue::ESTrue);
    } else {
        return ESValue(ESValue::ESFalse);
    }
}

static void screenShotTimeoutHandler(Window* wnd, void* data)
{
    ESFunctionObject* p = (ESFunctionObject*)data;
    callScriptFunction(p, {}, 0,
                       ESVMInstance::currentInstance()->globalObject());
}

static ESValue screenShotFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();

    std::string path = window->document()->documentURI()->baseURI()->utf8Data();
    path = path.substr(strlen("file://"));
    path += ESVMInstance::currentInstance()
                ->currentExecutionContext()
                ->readArgument(0)
                .toString()
                ->utf8Data();
    window->screenShot(path);
    window->setTimeout(
        screenShotTimeoutHandler, 100,
        instance->currentExecutionContext()->readArgument(1).asESPointer());
    return ESValue();
}

static ESValue screenShotRelativePathFunction(ESVMInstance* instance)
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
    callScriptFunction(instance->currentExecutionContext()->readArgument(0), {},
                       0, instance->globalObject());
    return ESValue();
}

static ESValue forceDisableOnloadCaptureFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();

    window->forceDisableOnloadCapture();
    return ESValue();
}

static ESValue getXYWHFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();

    window->renderingIfNeeds();

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    Node* value0 = nullptr;
    if (!arg0.isUndefinedOrNull()) {
        CHECK_TYPEOF(arg0, Node);

        value0 = (Node*)(arg0.asESPointer()->asESObject()->extraPointerData());
    }
    Frame* fr = (Frame*)value0->frame();
    if (!fr) {
        return ESValue(ESValue::ESNull);
    } else if (fr->isFrameBox()) {
        LayoutRect rect = fr->asFrameBox()->absoluteRect(
            value0->document()->frame()->asFrameBox());
        ESObject* result = ESObject::create();
        result->set(ESString::create("x"), ESValue(rect.x().toFloat()));
        result->set(ESString::create("y"), ESValue(rect.y().toFloat()));
        result->set(ESString::create("width"), ESValue(rect.width().toFloat()));
        result->set(ESString::create("height"),
                    ESValue(rect.height().toFloat()));
        return ESValue(result);
    } else {
        // TODO
    }

    return ESValue();
}

static ESValue simulateClickFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    double value0 = arg0.toNumber();
    double value1 = arg1.toNumber();

    window->simulateClick(value0, value1);
    return ESValue();
}

static ESValue simulateVisibilityChangeFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    bool value0 = arg0.toBoolean();

    window->simulateVisibilitychange(value0);

    return ESValue();
}

static ESValue testAssertFunction(ESVMInstance* instance)
{
    if (instance->currentExecutionContext()->readArgument(0).isESString()) {
        std::jmp_buf tryPosition;
        if (setjmp(ESVMInstance::currentInstance()->registerTryPos(
                &tryPosition)) == 0) {
            ESValue result =
                instance->evaluate(instance->currentExecutionContext()
                                       ->readArgument(0)
                                       .asESString());
            ESVMInstance::currentInstance()->unregisterTryPos(&tryPosition);
            ESVMInstance::currentInstance()->unregisterCheckedObjectAll();
            if (result.toBoolean()) {
            } else {
                ESStringBuilder builder;
                builder.appendString("[FAIL]assertion fail : ");
                builder.appendString(instance->currentExecutionContext()
                                         ->readArgument(0)
                                         .asESString());
                ESString* s = builder.finalize();
                puts(s->utf8Data());
                STARFISH_LOG_ERROR("%s\n", s->utf8Data());
                exit(-1);
            }
        } else {
            ESStringBuilder builder;
            builder.appendString("[FAIL]got exception while eval : ");
            builder.appendString(instance->currentExecutionContext()
                                     ->readArgument(0)
                                     .asESString());
            ESString* s = builder.finalize();
            puts(s->utf8Data());
            STARFISH_LOG_ERROR("%s\n", s->utf8Data());
            exit(-1);
        }
    } else {
        if (instance->currentExecutionContext()->readArgument(0).toBoolean()) {
        } else {
            ESStringBuilder builder;
            builder.appendString("[FAIL]testAssert fail");
            ESString* s = builder.finalize();
            puts(s->utf8Data());
            STARFISH_LOG_ERROR("%s\n", s->utf8Data());
            exit(-1);
        }
    }
    return ESValue(ESValue::ESUndefined);
}

static ESValue testEndFunction(ESVMInstance* instance)
{
    puts("[PASS]");
    STARFISH_LOG_ERROR("%s\n", "[PASS]");
    GC_gcollect_and_unmap();
    GC_gcollect_and_unmap();
    GC_gcollect_and_unmap();
    GC_gcollect_and_unmap();
    GC_gcollect_and_unmap();
    exit(0);
    return ESValue(ESValue::ESUndefined);
}
#endif

static ESValue testImgDiffFunction(ESVMInstance* instance)
{
    std::string cmd = "./tool/imgdiff/imgdiff ";

    Window* wnd = (Window*)instance->globalObject()->extraPointerData();
    std::string path = wnd->document()->documentURI()->baseURI()->utf8Data();
    path = path.substr(strlen("file://"));

    cmd += path;
    cmd += instance->currentExecutionContext()
               ->readArgument(0)
               .toString()
               ->utf8Data();
    cmd += " ";
    cmd += path;
    cmd += instance->currentExecutionContext()
               ->readArgument(1)
               .toString()
               ->utf8Data();

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
        cmd += instance->currentExecutionContext()
                   ->readArgument(0)
                   .toString()
                   ->utf8Data();
        cmd += " ";
        cmd += path;
        cmd += instance->currentExecutionContext()
                   ->readArgument(1)
                   .toString()
                   ->utf8Data();
        cmd += " ";
        cmd += path;
        cmd += std::string(instance->currentExecutionContext()
                               ->readArgument(0)
                               .toString()
                               ->utf8Data()) +
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

        ESStringBuilder builder;
        builder.appendString("[FAIL]testImgDiff fail");
        ESString* s = builder.finalize();
        puts(s->utf8Data());
        STARFISH_LOG_ERROR("%s\n", s->utf8Data());
        exit(-1);
    }

    pclose(fp);
    return ESValue(ESValue::ESUndefined);
}

static ESValue readCallbackFunction(const ESValue& key, ESObject* obj)
{
    STARFISH_ASSERT(obj == ESVMInstance::currentInstance()->globalObject());
    Window* self = (Window*)obj->extraPointerData();

    if (self->document()
            ->elementExecutionStackForAttributeStringEventFunctionObject()
            .size()) {
        ScriptValue v =
            self->document()
                ->elementExecutionStackForAttributeStringEventFunctionObject()
                .back()
                ->scriptValue();
        bool t = v.asESPointer()->asESObject()->hasOwnProperty(key);
        if (t) {
            return v.asESPointer()->asESObject()->get(key);
        }
    }

    String* name = toBrowserString(key);
    HTMLCollection* coll = self->namedAccess(name);
    if (coll) {
        if (coll->length()) {
            if (coll->length() > 1) {
                return coll->scriptValue();
            } else {
                return coll->item(0)->scriptObject();
            }
        }
    }
    return ESValue(ESValue::ESDeletedValue);
}

static bool writeCallbackFunction(const ESValue& key, const ESValue& val,
                                  ESObject* obj)
{
    STARFISH_ASSERT(obj == ESVMInstance::currentInstance()->globalObject());
    return false;
}

static ESValueVector enumerateCallbackFunction(ESObject* obj)
{
    STARFISH_ASSERT(obj == ESVMInstance::currentInstance()->globalObject());
    size_t len = 0;
    ESValueVector v(len);
    return v;
}

void Window::postInit(ScriptBindingInstance* instance)
{
#ifdef STARFISH_ENABLE_TEST
    scriptObject()->defineDataProperty(
        ESString::create("debugPause"), true, true, true,
        ESFunctionObject::create(NULL, debugPauseFunction,
                                 ESString::create("debugPause"), 0, false));

    scriptObject()->defineDataProperty(
        ESString::create("debugResume"), true, true, true,
        ESFunctionObject::create(NULL, debugResumeFunction,
                                 ESString::create("debugResume"), 0, false));

    scriptObject()->defineDataProperty(
        ESString::create("networkEnable"), true, true, true,
        ESFunctionObject::create(NULL, networkEnableFunction,
                                 ESString::create("networkEnable"), 0, false));

    scriptObject()->defineDataProperty(
        ESString::create("networkDisable"), true, true, true,
        ESFunctionObject::create(NULL, networkDisableFunction,
                                 ESString::create("networkDisable"), 0, false));

    scriptObject()->defineDataProperty(
        ESString::create("isPixelTest"), true, true, true,
        ESFunctionObject::create(NULL, isPixelTestFunction,
                                 ESString::create("isPixelTest"), 0, false));

    scriptObject()->defineDataProperty(
        ESString::create("screenShot"), true, true, true,
        ESFunctionObject::create(NULL, screenShotFunction,
                                 ESString::create("screenShot"), 0, false));

    scriptObject()->defineDataProperty(
        ESString::create("screenShotRelativePath"), true, true, true,
        ESFunctionObject::create(NULL, screenShotRelativePathFunction,
                                 ESString::create("screenShotRelativePath"), 0,
                                 false));

    scriptObject()->defineDataProperty(
        ESString::create("forceDisableOnloadCapture"), true, true, true,
        ESFunctionObject::create(NULL, forceDisableOnloadCaptureFunction,
                                 ESString::create("forceDisableOnloadCapture"),
                                 0, false));

    scriptObject()->defineDataProperty(
        ESString::create("getXYWH"), true, true, true,
        ESFunctionObject::create(NULL, getXYWHFunction,
                                 ESString::create("getXYWH"), 2, false));

    scriptObject()->defineDataProperty(
        ESString::create("simulateClick"), true, true, true,
        ESFunctionObject::create(NULL, simulateClickFunction,
                                 ESString::create("simulateClick"), 2, false));

    scriptObject()->defineDataProperty(
        ESString::create("simulateVisibilitychange"), true, true, true,
        ESFunctionObject::create(NULL, simulateVisibilityChangeFunction,
                                 ESString::create("simulateVisibilitychange"),
                                 0, false));

    scriptObject()->defineDataProperty(
        ESString::create("testAssert"), true, true, true,
        ESFunctionObject::create(NULL, testAssertFunction,
                                 ESString::create("testAssert"), 0, false));

    scriptObject()->defineDataProperty(
        ESString::create("testEnd"), true, true, true,
        ESFunctionObject::create(NULL, testEndFunction,
                                 ESString::create("testEnd"), 0, false));

    scriptObject()->defineDataProperty(
        ESString::create("testImgDiff"), true, true, true,
        ESFunctionObject::create(NULL, testImgDiffFunction,
                                 ESString::create("testImgDiff"), 0, false));
#endif

    scriptObject()->setPropertyInterceptor(
        readCallbackFunction, writeCallbackFunction, enumerateCallbackFunction);
}
}
