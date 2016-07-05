/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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
#include "ScriptWrappable.h"

#include "platform/window/Window.h"
#include "dom/DOM.h"

#include "vm/ESVMInstance.h"

#include "dom/binding/ScriptBindingInstance.h"
#include "dom/binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "style/CSSStyleLookupTrie.h"

namespace StarFish {

static ScriptBindingInstanceDataEscargot* fetchData(ScriptBindingInstance* instance)
{
    return (ScriptBindingInstanceDataEscargot*)instance->data();
}

ScriptWrappable::ScriptWrappable(void* extraPointerData)
{
    STARFISH_ASSERT(!((size_t)extraPointerData & (size_t)1));
    m_object = (escargot::ESFunctionObject*)((size_t)extraPointerData | (size_t)1);
}

ScriptObject ScriptWrappable::scriptObjectSlowCase()
{
    void* extraPointerData = (void*)((size_t)m_object - 1);
    m_object = escargot::ESObject::create(0);
    STARFISH_ASSERT(!((size_t)m_object & (size_t)1));
    m_object->setExtraPointerData(extraPointerData);

    Window* window = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
    initScriptObject(window->starFish()->scriptBindingInstance());

    return m_object;
}

void ScriptWrappable::initScriptWrappable(Window* window)
{
    m_object = escargot::ESVMInstance::currentInstance()->globalObject();
    auto data = fetchData(window->starFish()->scriptBindingInstance());
    scriptObject()->set__proto__(data->m_window->protoType());
    scriptObject()->setExtraData(ScriptWrappable::WindowObject);
    scriptObject()->setExtraPointerData(window);

#ifdef STARFISH_ENABLE_TEST
    escargot::ESFunctionObject* isPixelTestFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (v.isUndefinedOrNull() || v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            if (getenv("PIXEL_TEST") && strlen(getenv("PIXEL_TEST")))
                return escargot::ESValue(escargot::ESValue::ESTrue);
            else
                return escargot::ESValue(escargot::ESValue::ESFalse);
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, escargot::ESString::create("isPixelTest"), 0, false);
    ((escargot::ESObject*)this->m_object)->defineDataProperty(escargot::ESString::create("isPixelTest"), true, true, true, isPixelTestFunction);

    escargot::ESFunctionObject* screenShotFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (v.isUndefinedOrNull() || v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
            std::string path = wnd->document()->documentURI().baseURI()->utf8Data();
            path = path.substr(strlen("file://"));
            path += escargot::ESVMInstance::currentInstance()->currentExecutionContext()->readArgument(0).toString()->utf8Data();
            wnd->screenShot(path);
            callScriptFunction(instance->currentExecutionContext()->readArgument(1), { }, 0, instance->globalObject());
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, escargot::ESString::create("screenShot"), 0, false);
    ((escargot::ESObject*)this->m_object)->defineDataProperty(escargot::ESString::create("screenShot"), true, true, true, screenShotFunction);

    escargot::ESFunctionObject* screenShotRelativePathFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (v.isUndefinedOrNull() || v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
            char buff[1024];
            getcwd(buff, 1024);
            String* path = String::fromUTF8(buff)->concat(String::fromUTF8("/"))->concat(String::fromUTF8(getenv("SCREEN_SHOT_FILE") ? getenv("SCREEN_SHOT_FILE") : ""));
            wnd->screenShot(path->utf8Data());
            callScriptFunction(instance->currentExecutionContext()->readArgument(0), { }, 0, instance->globalObject());
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, escargot::ESString::create("screenShotRelativePath"), 0, false);
    ((escargot::ESObject*)this->m_object)->defineDataProperty(escargot::ESString::create("screenShotRelativePath"), true, true, true, screenShotRelativePathFunction);

    escargot::ESFunctionObject* forceDisableOnloadCaptureFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (v.isUndefinedOrNull() || v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
            wnd->forceDisableOnloadCapture();
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, escargot::ESString::create("forceDisableOnloadCapture"), 0, false);
    ((escargot::ESObject*)this->m_object)->defineDataProperty(escargot::ESString::create("forceDisableOnloadCapture"), true, true, true, forceDisableOnloadCaptureFunction);

    escargot::ESFunctionObject* simulateClickFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (v.isUndefinedOrNull() || v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
            wnd->simulateClick(escargot::ESVMInstance::currentInstance()->currentExecutionContext()->readArgument(0).toNumber(), escargot::ESVMInstance::currentInstance()->currentExecutionContext()->readArgument(1).toNumber());
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, escargot::ESString::create("simulateClick"), 2, false);
    ((escargot::ESObject*)this->m_object)->defineDataProperty(escargot::ESString::create("simulateClick"), true, true, true, simulateClickFunction);

    escargot::ESFunctionObject* simulateVisibilitychangeFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (v.isUndefinedOrNull() || v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
            wnd->simulateVisibilitychange(escargot::ESVMInstance::currentInstance()->currentExecutionContext()->readArgument(0).toBoolean());
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, escargot::ESString::create("simulateVisibilitychange"), 0, false);
    ((escargot::ESObject*)this->m_object)->defineDataProperty(escargot::ESString::create("simulateVisibilitychange"), true, true, true, simulateVisibilitychangeFunction);

    escargot::ESFunctionObject* testAssertFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        if (instance->currentExecutionContext()->readArgument(0).isESString()) {
            std::jmp_buf tryPosition;
            if (setjmp(escargot::ESVMInstance::currentInstance()->registerTryPos(&tryPosition)) == 0) {
                escargot::ESValue result = instance->evaluate(instance->currentExecutionContext()->readArgument(0).asESString());
                escargot::ESVMInstance::currentInstance()->unregisterTryPos(&tryPosition);
                if (result.toBoolean()) {

                } else {
                    escargot::ESStringBuilder builder;
                    builder.appendString("[FAIL]assertion fail : ");
                    builder.appendString(instance->currentExecutionContext()->readArgument(0).asESString());
                    escargot::ESString* s = builder.finalize();
                    puts(s->utf8Data());
                    STARFISH_LOG_ERROR("%s\n", s->utf8Data());
                    exit(-1);
                }
            } else {
                escargot::ESStringBuilder builder;
                builder.appendString("[FAIL]got exception while eval : ");
                builder.appendString(instance->currentExecutionContext()->readArgument(0).asESString());
                escargot::ESString* s = builder.finalize();
                puts(s->utf8Data());
                STARFISH_LOG_ERROR("%s\n", s->utf8Data());
                exit(-1);
            }
        } else {
            if (instance->currentExecutionContext()->readArgument(0).toBoolean()) {

            } else {
                escargot::ESStringBuilder builder;
                builder.appendString("[FAIL]testAssert fail");
                escargot::ESString* s = builder.finalize();
                puts(s->utf8Data());
                STARFISH_LOG_ERROR("%s\n", s->utf8Data());
                exit(-1);
            }
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, escargot::ESString::create("testAssert"), 0, false);
    ((escargot::ESObject*)this->m_object)->defineDataProperty(escargot::ESString::create("testAssert"), true, true, true, testAssertFunction);

    escargot::ESFunctionObject* testEndFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        puts("[PASS]");
        STARFISH_LOG_ERROR("%s\n", "[PASS]");
        exit(0);
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, escargot::ESString::create("testEnd"), 0, false);
    ((escargot::ESObject*)this->m_object)->defineDataProperty(escargot::ESString::create("testEnd"), true, true, true, testEndFunction);

#endif

    escargot::ESFunctionObject* testImgDiffFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        std::string cmd = "./tool/imgdiff/imgdiff ";

        Window* wnd = (Window*)instance->globalObject()->extraPointerData();
        std::string path = wnd->document()->documentURI().baseURI()->utf8Data();
        path = path.substr(strlen("file://"));

        cmd += path;
        cmd += instance->currentExecutionContext()->readArgument(0).toString()->utf8Data();
        cmd += " ";
        cmd += path;
        cmd += instance->currentExecutionContext()->readArgument(1).toString()->utf8Data();

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
            cmd += instance->currentExecutionContext()->readArgument(0).toString()->utf8Data();
            cmd += " ";
            cmd += path;
            cmd += instance->currentExecutionContext()->readArgument(1).toString()->utf8Data();
            cmd += " ";
            cmd += path;
            cmd += std::string(instance->currentExecutionContext()->readArgument(0).toString()->utf8Data()) + "_diff.png";
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

            escargot::ESStringBuilder builder;
            builder.appendString("[FAIL]testImgDiff fail");
            escargot::ESString* s = builder.finalize();
            puts(s->utf8Data());
            STARFISH_LOG_ERROR("%s\n", s->utf8Data());
            exit(-1);
        }

        pclose(fp);
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, escargot::ESString::create("testEnd"), 0, false);
    ((escargot::ESObject*)this->m_object)->defineDataProperty(escargot::ESString::create("testImgDiff"), true, true, true, testImgDiffFunction);

    // [setTimeout]
    // https://www.w3.org/TR/html5/webappapis.html#dom-windowtimers-settimeout
    // long setTimeout(Function handler, optional long timeout, any... arguments);

    // TODO : Pass "any... arguments" if exist
    escargot::ESFunctionObject* setTimeoutFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();

        auto handler = [](Window* wnd, void* data)
        {
            escargot::ESFunctionObject* fn = (escargot::ESFunctionObject*)data;
            std::jmp_buf tryPosition;
            if (setjmp(escargot::ESVMInstance::currentInstance()->registerTryPos(&tryPosition)) == 0) {
                escargot::ESFunctionObject::call(escargot::ESVMInstance::currentInstance(), fn, escargot::ESValue(), NULL, 0, false);
                escargot::ESVMInstance::currentInstance()->unregisterTryPos(&tryPosition);
            } else {
                escargot::ESValue err = escargot::ESVMInstance::currentInstance()->getCatchedError();
                STARFISH_LOG_INFO("Uncaught %s\n", err.toString()->utf8Data());
            }
        };

        if (v.isUndefinedOrNull() || v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            if (instance->currentExecutionContext()->readArgument(0).isESPointer()
                && instance->currentExecutionContext()->readArgument(0).asESPointer()
                && instance->currentExecutionContext()->readArgument(0).asESPointer()->isESFunctionObject()) {
                Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
                if (instance->currentExecutionContext()->readArgument(1).isUndefinedOrNull()) {
                    return escargot::ESValue(wnd->setTimeout(handler, 0,
                    instance->currentExecutionContext()->readArgument(0).asESPointer()));
                } else if (instance->currentExecutionContext()->readArgument(1).isNumber()) {
                    return escargot::ESValue(wnd->setTimeout(handler, instance->currentExecutionContext()->readArgument(1).toUint32(),
                    instance->currentExecutionContext()->readArgument(0).asESPointer()));
                }
            } else {
                String* bodyStr = toBrowserString(instance->currentExecutionContext()->readArgument(0).toString());
                String* name[] = {String::emptyString};
                bool error = false;
                ScriptValue m_listener = createScriptFunction(name, 1, bodyStr, error);
                Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
                if (instance->currentExecutionContext()->readArgument(1).isUndefinedOrNull()) {
                    return escargot::ESValue(wnd->setTimeout(handler, 0,
                    m_listener.asESPointer()));
                } else if (instance->currentExecutionContext()->readArgument(1).isNumber()) {
                    return escargot::ESValue(wnd->setTimeout(handler, instance->currentExecutionContext()->readArgument(1).toUint32(),
                    m_listener.asESPointer()));

                }
            }
        }
        return escargot::ESValue();
    }, escargot::ESString::create("setTimeout"), 1, false);
    ((escargot::ESObject*)this->m_object)->defineDataProperty(escargot::ESString::create("setTimeout"), true, true, true, setTimeoutFunction);

    // [clearTimeout]
    // https://www.w3.org/TR/html5/webappapis.html#dom-windowtimers-cleartimeout
    escargot::ESFunctionObject* clearTimeoutFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (v.isUndefinedOrNull() || v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            if (instance->currentExecutionContext()->readArgument(0).isNumber()) {
                Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
                wnd->clearTimeout(instance->currentExecutionContext()->readArgument(0).toUint32());
            }
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, escargot::ESString::create("clearTimeout"), 0, false);
    ((escargot::ESObject*)this->m_object)->defineDataProperty(escargot::ESString::create("clearTimeout"), true, true, true, clearTimeoutFunction);

    // https://www.w3.org/TR/html5/webappapis.html#dom-windowtimers-setinterval
    escargot::ESFunctionObject* setIntervalFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        auto handler = [](Window* wnd, void* data)
        {
            escargot::ESFunctionObject* fn = (escargot::ESFunctionObject*)data;
            std::jmp_buf tryPosition;
            if (setjmp(escargot::ESVMInstance::currentInstance()->registerTryPos(&tryPosition)) == 0) {
                escargot::ESFunctionObject::call(escargot::ESVMInstance::currentInstance(), fn, escargot::ESValue(), NULL, 0, false);
                escargot::ESVMInstance::currentInstance()->unregisterTryPos(&tryPosition);
            } else {
                escargot::ESValue err = escargot::ESVMInstance::currentInstance()->getCatchedError();
                STARFISH_LOG_INFO("Uncaught %s\n", err.toString()->utf8Data());
            }
        };

        if (v.isUndefinedOrNull() || v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            if (instance->currentExecutionContext()->readArgument(0).isESPointer()
                && instance->currentExecutionContext()->readArgument(0).asESPointer()
                && instance->currentExecutionContext()->readArgument(0).asESPointer()->isESFunctionObject()) {
                if (instance->currentExecutionContext()->readArgument(1).isNumber()) {
                    Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
                    return escargot::ESValue(wnd->setInterval(handler,
                    instance->currentExecutionContext()->readArgument(1).toUint32(),
                    instance->currentExecutionContext()->readArgument(0).asESPointer()));
                }
            } else {
                String* bodyStr = toBrowserString(instance->currentExecutionContext()->readArgument(0).toString());
                String* name[] = {String::emptyString};
                bool error = false;
                ScriptValue m_listener = createScriptFunction(name, 1, bodyStr, error);

                Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
                if (instance->currentExecutionContext()->readArgument(1).isNumber()) {

                    return escargot::ESValue(wnd->setInterval(handler, instance->currentExecutionContext()->readArgument(1).toUint32(),
                    m_listener.asESPointer()));
                }
            }
        }
        return escargot::ESValue();
    }, escargot::ESString::create("setInterval"), 1, false);
    ((escargot::ESObject*)this->m_object)->defineDataProperty(escargot::ESString::create("setInterval"), true, true, true, setIntervalFunction);

    // https://www.w3.org/TR/html5/webappapis.html#dom-windowtimers-clearinterval
    escargot::ESFunctionObject* clearIntervalFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (v.isUndefinedOrNull() || v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            if (instance->currentExecutionContext()->readArgument(0).isNumber()) {
                Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
                wnd->clearInterval(instance->currentExecutionContext()->readArgument(0).toUint32());
            }
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, escargot::ESString::create("clearInterval"), 0, false);
    ((escargot::ESObject*)this->m_object)->defineDataProperty(escargot::ESString::create("clearInterval"), true, true, true, clearIntervalFunction);

    // TODO : Pass "any... arguments" if exist
    // TODO : First argument can be function or script source (currently allow function only)
    escargot::ESFunctionObject* requestAnimationFrameFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (v.isUndefinedOrNull() || v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            if (instance->currentExecutionContext()->readArgument(0).isESPointer()
                && instance->currentExecutionContext()->readArgument(0).asESPointer()
                && instance->currentExecutionContext()->readArgument(0).asESPointer()->isESFunctionObject()) {
                    Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
                    return escargot::ESValue(wnd->requestAnimationFrame([](Window* wnd, void* data) {
                        escargot::ESFunctionObject* fn = (escargot::ESFunctionObject*)data;
                        std::jmp_buf tryPosition;
                        if (setjmp(escargot::ESVMInstance::currentInstance()->registerTryPos(&tryPosition)) == 0) {
                            escargot::ESFunctionObject::call(escargot::ESVMInstance::currentInstance(), fn, escargot::ESValue(), NULL, 0, false);
                            escargot::ESVMInstance::currentInstance()->unregisterTryPos(&tryPosition);
                        } else {
                            std::jmp_buf tryPosition;
                            escargot::ESValue err = escargot::ESVMInstance::currentInstance()->getCatchedError();
                            if (setjmp(escargot::ESVMInstance::currentInstance()->registerTryPos(&tryPosition)) == 0) {
                                STARFISH_LOG_INFO("Uncaught %s\n", err.toString()->utf8Data());
                                escargot::ESVMInstance::currentInstance()->unregisterTryPos(&tryPosition);
                            } else {
                                STARFISH_LOG_INFO("Uncaught Error\n");
                            }
                        }
                    }, instance->currentExecutionContext()->readArgument(0).asESPointer()));
            }
        }
        return escargot::ESValue();
    }, escargot::ESString::create("requestAnimationFrame"), 1, false);
    ((escargot::ESObject*)this->m_object)->defineDataProperty(escargot::ESString::create("requestAnimationFrame"), false, false, false, requestAnimationFrameFunction);

    // https://www.w3.org/TR/html5/webappapis.html
    escargot::ESFunctionObject* cancelAnimationFrameFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (v.isUndefinedOrNull() || v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            if (instance->currentExecutionContext()->readArgument(0).isNumber()) {
                Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
                wnd->cancelAnimationFrame(instance->currentExecutionContext()->readArgument(0).toUint32());
            }
        }
        return escargot::ESValue();
    }, escargot::ESString::create("cancelAnimationFrame"), 1, false);
    ((escargot::ESObject*)this->m_object)->defineDataProperty(escargot::ESString::create("cancelAnimationFrame"), false, false, false, cancelAnimationFrameFunction);

    // https://www.w3.org/TR/cssom-view/#dom-window-innerwidth
    ((escargot::ESObject*)this->m_object)->defineAccessorProperty(escargot::ESString::create("innerWidth"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        escargot::ESValue v = originalObj;
        if (v.isObject() && v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            double innerWidth = ((Window*)originalObj->extraPointerData())->innerWidth();
            return escargot::ESValue(innerWidth);
        }
        return escargot::ESValue(0);
        },
        NULL, true, true, true);

    // https://www.w3.org/TR/cssom-view/#dom-window-innerheight
    ((escargot::ESObject*)this->m_object)->defineAccessorProperty(escargot::ESString::create("innerHeight"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        escargot::ESValue v = originalObj;
        if (v.isObject() && v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            double innerHeight = ((Window*)originalObj->extraPointerData())->innerHeight();
            return escargot::ESValue(innerHeight);
        }
        return escargot::ESValue(0);
        },
        NULL, true, true, true);

    ((escargot::ESObject*)this->m_object)->defineAccessorProperty(escargot::ESString::create("onclick"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        escargot::ESValue v = originalObj;
        if (v.isObject() && v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
            auto eventType = wnd->starFish()->staticStrings()->m_click;
            return wnd->attributeEventListener(eventType);
        }
        return escargot::ESValue();
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, ::escargot::ESString* propertyName, const escargot::ESValue& value)
        {
        escargot::ESValue v = originalObj;
        if (v.isObject() && v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
            auto eventType = wnd->starFish()->staticStrings()->m_click;
            if (value.isObject() || (value.isESPointer() && value.asESPointer()->isESFunctionObject())) {
                wnd->setAttributeEventListener(eventType, value);
            } else {
                wnd->clearAttributeEventListener(eventType);
            }
        }
        },
        true, true, true);

    ((escargot::ESObject*)this->m_object)->defineAccessorProperty(escargot::ESString::create("onload"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        escargot::ESValue v = originalObj;
        if (v.isObject() && v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
            auto eventType = wnd->starFish()->staticStrings()->m_load;
            return wnd->attributeEventListener(eventType);
        }
        return escargot::ESValue();
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, ::escargot::ESString* propertyName, const escargot::ESValue& value)
        {
        escargot::ESValue v = originalObj;
        if (v.isObject() && v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
            auto eventType = wnd->starFish()->staticStrings()->m_load;
            if (value.isObject() || (value.isESPointer() && value.asESPointer()->isESFunctionObject())) {
                wnd->setAttributeEventListener(eventType, value);
            } else {
                wnd->clearAttributeEventListener(eventType);
            }
        }
        },
        true, true, true);

    ((escargot::ESObject*)this->m_object)->defineAccessorProperty(escargot::ESString::create("onunload"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        escargot::ESValue v = originalObj;
        if (v.isObject() && v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
            auto eventType = wnd->starFish()->staticStrings()->m_unload;
            return wnd->attributeEventListener(eventType);
        }
        return escargot::ESValue();
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, ::escargot::ESString* propertyName, const escargot::ESValue& value)
        {
        escargot::ESValue v = originalObj;
        if (v.isObject() && v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
            auto eventType = wnd->starFish()->staticStrings()->m_unload;
            if (value.isObject() || (value.isESPointer() && value.asESPointer()->isESFunctionObject())) {
                wnd->setAttributeEventListener(eventType, value);
            } else {
                wnd->clearAttributeEventListener(eventType);
            }
        }
        },
        true, true, true);

    scriptObject()->setPropertyInterceptor([](const escargot::ESValue& key, escargot::ESObject* obj) -> escargot::ESValue {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::WindowObject);
        Window* self = (Window*)obj->extraPointerData();

        if (self->document()->elementExecutionStackForAttributeStringEventFunctionObject().size()) {
            ScriptValue v = self->document()->elementExecutionStackForAttributeStringEventFunctionObject().back()->scriptValue();
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
        return escargot::ESValue(escargot::ESValue::ESDeletedValue);
    }, [](const escargot::ESValue& key, const escargot::ESValue& val, escargot::ESObject* obj) -> bool {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::WindowObject);
        return false;
    }, [](escargot::ESObject* obj) -> escargot::ESValueVector {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::WindowObject);
        size_t len = 0;
        escargot::ESValueVector v(len);
        return v;
    });
}

void ScriptWrappable::initScriptWrappable(Node* ptr)
{
    Node* node = (Node*)this;
    initScriptWrappable(ptr, node->document()->scriptBindingInstance());
}

void ScriptWrappable::initScriptWrappable(Node* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->node()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(Element* element)
{
    Node* node = (Node*)this;
    initScriptWrappable(element, node->document()->scriptBindingInstance());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(DocumentType* element)
{
    auto data = fetchData(element->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->documentType()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(Element* element, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->element()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(Document*)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->document()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(DocumentFragment* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->documentFragment()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

#ifdef STARFISH_EXP
void ScriptWrappable::initScriptWrappable(DOMImplementation* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->m_domImplementation()->protoType());
    scriptObject()->setExtraData(DOMImplementationObject);
}
#endif

void ScriptWrappable::initScriptWrappable(HTMLDocument*)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlDocument()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(CharacterData* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->characterData()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(Text* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->text()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(Comment* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->comment()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLHtmlElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlHtmlElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLHeadElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlHeadElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLBodyElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlBodyElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLStyleElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlStyleElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLLinkElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlLinkElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLScriptElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlScriptElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLImageElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlImageElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLDivElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlDivElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLBRElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlBrElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLMetaElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlMetaElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLParagraphElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlParagraphElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLSpanElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlSpanElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

#ifdef STARFISH_ENABLE_AUDIO
void ScriptWrappable::initScriptWrappable(HTMLAudioElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlAudioElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}
#endif

void ScriptWrappable::initScriptWrappable(HTMLUnknownElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlUnknownElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(XMLHttpRequest* xhr)
{
    ScriptBindingInstance* instance = xhr->networkRequest().document()->window()->starFish()->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->xhrElement()->protoType());
    scriptObject()->setExtraData(XMLHttpRequestObject);
}

void ScriptWrappable::initScriptWrappable(Blob* blob)
{
    Window* window = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
    ScriptBindingInstance* instance = window->starFish()->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->blobElement()->protoType());
    scriptObject()->setExtraData(BlobObject);
}

void ScriptWrappable::initScriptWrappable(URL* url, ScriptBindingInstance* instance)
{
    // auto data = fetchData(instance);
    // scriptObject()->set__proto__(data->urlElement()->protoType());
    // scriptObject()->setExtraData(URLObject);
}

void ScriptWrappable::initScriptWrappable(DOMException* exception, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->domException()->protoType());
    scriptObject()->setExtraData(DOMExceptionObject);

    scriptObject()->defineDataProperty(escargot::ESString::create("code"), false, false, false, escargot::ESValue(exception->code()));
}

bool ScriptWrappable::hasProperty(String* name)
{
    return m_object->escargot::ESObject::hasProperty(createScriptString(name));
}

void ScriptWrappable::initScriptWrappable(Event* event)
{
    Window* window = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
    ScriptBindingInstance* instance = window->starFish()->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->event()->protoType());
    scriptObject()->setExtraData(EventObject);
}

void ScriptWrappable::initScriptWrappable(UIEvent* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->uiEvent()->protoType());
    scriptObject()->setExtraData(EventObject);
}

void ScriptWrappable::initScriptWrappable(MouseEvent* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->mouseEvent()->protoType());
    scriptObject()->setExtraData(EventObject);
}

void ScriptWrappable::initScriptWrappable(ProgressEvent* ptr)
{
    Window* window = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
    ScriptBindingInstance* instance = window->starFish()->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->progressEvent()->protoType());
    scriptObject()->setExtraData(EventObject);
}

void ScriptWrappable::initScriptWrappable(HTMLCollection* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->htmlCollection()->protoType());
    scriptObject()->setExtraData(HTMLCollectionObject);

    scriptObject()->setPropertyInterceptor([](const escargot::ESValue& key, escargot::ESObject* obj) -> escargot::ESValue {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::HTMLCollectionObject);
        HTMLCollection* self = (HTMLCollection*)obj->extraPointerData();
        uint32_t idx = key.toIndex();
        if (idx == escargot::ESValue::ESInvalidIndexValue) {
            Element* e = self->namedItem(toBrowserString(key));
            if (e != nullptr)
                return e->scriptValue();
        } else if (idx < self->length()) {
            return self->item(idx)->scriptValue();
        }
        return escargot::ESValue(escargot::ESValue::ESDeletedValue);
    }, [](const escargot::ESValue& key, const escargot::ESValue& val, escargot::ESObject* obj) -> bool {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::HTMLCollectionObject);
        return false;
    }, [](escargot::ESObject* obj) -> escargot::ESValueVector {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::HTMLCollectionObject);
        HTMLCollection* self = (HTMLCollection*)obj->extraPointerData();
        size_t len = self->length();
        escargot::ESValueVector v(len);
        for (size_t i = 0; i < len; i ++) {
            v[i] = escargot::ESValue(i);
        }
        return v;
    }, true);
}

void ScriptWrappable::initScriptWrappable(NodeList* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->nodeList()->protoType());
    scriptObject()->setExtraData(NodeListObject);

    scriptObject()->setPropertyInterceptor([](const escargot::ESValue& key, escargot::ESObject* obj) -> escargot::ESValue {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::NodeListObject);
        NodeList* self = (NodeList*)obj->extraPointerData();
        uint32_t idx = key.toIndex();
        if (idx < self->length())
            return self->item(idx)->scriptValue();
        return escargot::ESValue(escargot::ESValue::ESDeletedValue);
    }, [](const escargot::ESValue& key, const escargot::ESValue& val, escargot::ESObject* obj) -> bool {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::NodeListObject);
        return false;
    }, [](escargot::ESObject* obj) -> escargot::ESValueVector {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::NodeListObject);
        NodeList* self = (NodeList*)obj->extraPointerData();
        size_t len = self->length();
        escargot::ESValueVector v(len);
        for (size_t i = 0; i < len; i ++) {
            v[i] = escargot::ESValue(i);
        }
        return v;
    }, true);
}

void ScriptWrappable::initScriptWrappable(DOMTokenList* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->domTokenList()->protoType());
    scriptObject()->setExtraData(DOMTokenListObject);

    scriptObject()->setPropertyInterceptor([](const escargot::ESValue& key, escargot::ESObject* obj) -> escargot::ESValue {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::DOMTokenListObject);
        DOMTokenList* self = (DOMTokenList*)obj->extraPointerData();
        uint32_t idx = key.toIndex();
        if (idx < self->length())
            return createScriptString(self->item(idx));
        return escargot::ESValue(escargot::ESValue::ESDeletedValue);
    }, [](const escargot::ESValue& key, const escargot::ESValue& val, escargot::ESObject* obj) -> bool {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::DOMTokenListObject);
        return false;
    }, [](escargot::ESObject* obj) -> escargot::ESValueVector {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::DOMTokenListObject);
        DOMTokenList* self = (DOMTokenList*)obj->extraPointerData();
        size_t len = self->length();
        escargot::ESValueVector v(len);
        for (size_t i = 0; i < len; i ++) {
            v[i] = escargot::ESValue(i);
        }
        return v;
    }, true);
}

void ScriptWrappable::initScriptWrappable(DOMSettableTokenList* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->domSettableTokenList()->protoType());
    scriptObject()->setExtraData(DOMSettableTokenListObject);
}

void ScriptWrappable::initScriptWrappable(NamedNodeMap* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->namedNodeMap()->protoType());
    scriptObject()->setExtraData(NamedNodeMapObject);

    scriptObject()->setPropertyInterceptor([](const escargot::ESValue& key, escargot::ESObject* obj) -> escargot::ESValue {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::NamedNodeMapObject);
        NamedNodeMap* self = (NamedNodeMap*)obj->extraPointerData();
        uint32_t idx = key.toIndex();
        if (idx == escargot::ESValue::ESInvalidIndexValue) {
            Attr* e = self->getNamedItem(QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAttrAtomicString(((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish(), key.asESString()->utf8Data())));
            if (e != nullptr)
                return e->scriptValue();
        } else if (idx < self->length()) {
            return self->item(idx)->scriptValue();
        }
        return escargot::ESValue(escargot::ESValue::ESDeletedValue);
    }, [](const escargot::ESValue& key, const escargot::ESValue& val, escargot::ESObject* obj) -> bool {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::NamedNodeMapObject);
        return false;
    }, [](escargot::ESObject* obj) -> escargot::ESValueVector {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::NamedNodeMapObject);
        NamedNodeMap* self = (NamedNodeMap*)obj->extraPointerData();
        size_t len = self->length();
        escargot::ESValueVector v(len);
        for (size_t i = 0; i < len; i ++) {
            v[i] = escargot::ESValue(i);
        }
        return v;
    }, true);
}

void ScriptWrappable::initScriptWrappable(Attr* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->attr()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(CSSStyleDeclaration* ptr)
{
    auto data = fetchData(ptr->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->cssStyleDeclaration()->protoType());
    scriptObject()->setExtraData(CSSStyleDeclarationObject);

    scriptObject()->setPropertyInterceptor([](const escargot::ESValue& key, escargot::ESObject* obj) -> escargot::ESValue {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::CSSStyleDeclarationObject);
        CSSStyleDeclaration* self = (CSSStyleDeclaration*)obj->extraPointerData();
        uint32_t idx = key.toIndex();
        if (idx < self->length()) {
            return escargot::ESString::create(self->item(idx)->utf8Data());
        }

        if (idx == escargot::ESValue::ESInvalidIndexValue) {
            const char* str = toBrowserString(key)->utf8Data();
            CSSStyleKind kind = lookupCSSStyleCamelCase(str, strlen(str));

            if (kind == CSSStyleKind::Unknown) {
                return escargot::ESValue(escargot::ESValue::ESDeletedValue);
            } else {
                if (false) {

                }
#define GET_ATTR(name, nameLower, nameCSSCase) \
                else if (kind == CSSStyleKind::name) { \
                    return createScriptString(self->name()); \
                }
                FOR_EACH_STYLE_ATTRIBUTE_TOTAL(GET_ATTR)

            }
        }

        return escargot::ESString::create("");
    }, [](const escargot::ESValue& key, const escargot::ESValue& val, escargot::ESObject* obj) -> bool {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::CSSStyleDeclarationObject);
        CSSStyleDeclaration* self = (CSSStyleDeclaration*)obj->extraPointerData();
        const char* str = toBrowserString(key)->utf8Data();
        CSSStyleKind kind = lookupCSSStyleCamelCase(str, strlen(str));
        if (kind == CSSStyleKind::Unknown) {
            kind = lookupCSSStyle(str, strlen(str));
        }
        if (kind == CSSStyleKind::Unknown) {
            return false;
        } else {
            if (false) {

            }
#define SET_ATTR(name, nameLower, nameCSSCase) \
            else if (kind == CSSStyleKind::name) { \
                self->set##name(toBrowserString(val)); \
                return true; \
            }
            FOR_EACH_STYLE_ATTRIBUTE_TOTAL(SET_ATTR)
        }

        return false;
    }, [](escargot::ESObject* obj) -> escargot::ESValueVector {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::CSSStyleDeclarationObject);
        CSSStyleDeclaration* self = (CSSStyleDeclaration*)obj->extraPointerData();
        size_t len = self->length();
        escargot::ESValueVector v(len);
        for (size_t i = 0; i < len; i ++) {
            v[i] = escargot::ESValue(i);
        }

#define ENUM_ATTR(name, nameLower, nameCSSCase) \
        v.push_back(escargot::ESString::create(#nameLower));

        FOR_EACH_STYLE_ATTRIBUTE_TOTAL(ENUM_ATTR)
        return v;
    }, true);
}

void ScriptWrappable::initScriptWrappable(CSSStyleRule* ptr)
{
    auto data = fetchData(ptr->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->cssStyleRule()->protoType());
    scriptObject()->setExtraData(CSSStyleRuleObject);
}
#ifdef STARFISH_ENABLE_TEST
void Window::testStart()
{
    ScriptBindingInstanceEnterer enter(starFish()->scriptBindingInstance());
    escargot::ESValue v = escargot::ESVMInstance::currentInstance()->globalObject()->get(escargot::ESString::create("testStart"));
    if (!v.isUndefined()) {
        callScriptFunction(v, { }, 0, escargot::ESVMInstance::currentInstance()->globalObject());
    }
}
#endif

static int utf32ToUtf16(char32_t i, char16_t *u)
{
    if (i < 0xffff) {
        *u= (char16_t)(i & 0xffff);
        return 1;
    } else if (i < 0x10ffff) {
        i-= 0x10000;
        *u++= 0xd800 | (i >> 10);
        *u= 0xdc00 | (i & 0x3ff);
        return 2;
    } else {
        // produce error char
        *u = 0xFFFD;
        return 1;
    }
}


ScriptValue createScriptString(String* str)
{
    if (str->isASCIIString()) {
        ASCIIString s(str->asASCIIString()->begin(), str->asASCIIString()->end());
        return escargot::ESString::create(std::move(s));
    } else {
        escargot::UTF16String out;
        for (size_t i = 0; i < str->length(); i++) {
            char32_t src = str->charAt(i);
            char16_t dst[2];
            int ret = utf32ToUtf16(src, dst);

            if (ret == 1) {
                out.push_back(src);
            } else if (ret == 2) {
                out.push_back(dst[0]);
                out.push_back(dst[1]);
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }

        }

        return escargot::ESString::create(std::move(out));
    }
}

ScriptValue createScriptFunction(String** argNames, size_t argc, String* functionBody, bool& error)
{
    error = false;
    escargot::ESVMInstance* instance = escargot::ESVMInstance::currentInstance();

    escargot::ESValueVector arg(0);
    for (size_t i = 0; i < argc; i++) {
        arg.push_back(createScriptString(argNames[i]));
    }

    arg.push_back(createScriptString(functionBody));

    ScriptValue result;
    std::jmp_buf tryPosition;
    if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
        result = escargot::ESFunctionObject::call(instance, instance->globalObject()->function(), escargot::ESValue(),
            arg.data(), argc + 1, false);
        instance->unregisterTryPos(&tryPosition);
    } else {
        result = instance->getCatchedError();
        error = true;
        STARFISH_LOG_INFO("Uncaught %s\n", result.toString()->utf8Data());
    }
    return result;
}

struct AttributeStringEventFunctionInnerData : public gc {
    escargot::ESValue function;
    Element* m_target;
};

ScriptValue createAttributeStringEventFunction(Element* target, String* functionBody, bool& result)
{
    String* name[] = {String::createASCIIString("event")};
    escargot::ESValue fn = createScriptFunction(name, 1, functionBody, result);
    escargot::ESFunctionObject* wrapper = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::FunctionEnvironmentRecordWithArgumentsObject* record = (escargot::FunctionEnvironmentRecordWithArgumentsObject*)escargot::ESVMInstance::currentInstance()->currentExecutionContext()->environment()->record();
        escargot::ESFunctionObject* callee = record->callee();
        STARFISH_ASSERT(callee->extraData() == ScriptWrappable::AttributeStringEventFunctionObject);
        AttributeStringEventFunctionInnerData* data = (AttributeStringEventFunctionInnerData*)callee->extraPointerData();

        Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
        wnd->document()->elementExecutionStackForAttributeStringEventFunctionObject().push_back(data->m_target);

        escargot::ESVMInstance::currentInstance()->globalObject()->setIdentifierInterceptor([](const escargot::ESValue& key, escargot::ESObject* obj) -> escargot::ESValue {
            Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
            Element* e = wnd->document()->elementExecutionStackForAttributeStringEventFunctionObject().back();
            if (e->scriptValue().asESPointer()->asESObject()->hasOwnProperty(key, true)) {
                return e->scriptValue().asESPointer()->asESObject()->getOwnProperty(key);
            }
            return escargot::ESValue(escargot::ESValue::ESDeletedValue);
        });

        std::jmp_buf tryPosition;
        bool hasError = false;
        escargot::ESValue result;
        if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
            result = escargot::ESFunctionObject::call(escargot::ESVMInstance::currentInstance(), data->function,
                escargot::ESVMInstance::currentInstance()->currentExecutionContext()->resolveThisBinding(),
                escargot::ESVMInstance::currentInstance()->currentExecutionContext()->arguments(),
                escargot::ESVMInstance::currentInstance()->currentExecutionContext()->argumentCount(), false);
            instance->unregisterTryPos(&tryPosition);
            hasError = false;
        } else {
            hasError = true;
            result = instance->getCatchedError();
        }

        wnd->document()->elementExecutionStackForAttributeStringEventFunctionObject().pop_back();

        if (wnd->document()->elementExecutionStackForAttributeStringEventFunctionObject().size() == 0) {
            escargot::ESVMInstance::currentInstance()->globalObject()->setIdentifierInterceptor(nullptr);
        }

        if (hasError) {
            instance->throwError(result);
        }

        return result;
    }, escargot::ESString::create(""), 0, false);

    wrapper->codeBlock()->m_needsToPrepareGenerateArgumentsObject = true;
    wrapper->setExtraData(ScriptWrappable::AttributeStringEventFunctionObject);
    AttributeStringEventFunctionInnerData* data = new AttributeStringEventFunctionInnerData();
    data->m_target = target;
    data->function = fn;
    wrapper->setExtraPointerData(data);
    return wrapper;
}

ScriptValue callScriptFunction(ScriptValue fn, ScriptValue* argv, size_t argc, ScriptValue thisValue)
{
    ScriptValue result;
    if (fn.isESPointer() && fn.asESPointer()->isESFunctionObject()) {
        escargot::ESVMInstance* instance = escargot::ESVMInstance::currentInstance();
        std::jmp_buf tryPosition;
        if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
            result = escargot::ESFunctionObject::call(instance, fn, thisValue, argv, argc, false);
            instance->unregisterTryPos(&tryPosition);
        } else {
            result = instance->getCatchedError();
            STARFISH_LOG_INFO("Uncaught %s\n", result.toString()->utf8Data());
        }
    }
    return result;
}

ScriptValue parseJSON(String* jsonData)
{
    ScriptValue ret;
    escargot::ESVMInstance* instance = escargot::ESVMInstance::currentInstance();
    ScriptValue json_arg[1] = { ScriptValue(createScriptString(jsonData)) };
    ScriptValue json_parse_fn = instance->globalObject()->json()->get(ScriptValue(createScriptString(String::fromUTF8("parse"))));
    return callScriptFunction(json_parse_fn, json_arg, 1, instance->globalObject()->json());
}

bool isCallableScriptValue(ScriptValue v)
{
    if (v.isESPointer() && v.asESPointer()->isESFunctionObject()) {
        return true;
    }
    return false;
}

}
