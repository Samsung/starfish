/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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
#include "platform/message_loop/MessageLoop.h"
#include "Binding.h"

#include "vm/ESVMInstance.h"

#include "binding/ScriptBindingInstance.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"
#include "dom/Document.h"

#include "style/CSSStyleLookupTrie.h"

#include "layout/Frame.h"
#include "layout/FrameBox.h"

#include "StarFish.h"

namespace StarFish {

using namespace escargot;

ScriptWrappable::ScriptWrappable(void* extraPointerData)
{
    STARFISH_ASSERT(!((size_t)extraPointerData & (size_t)1));
    m_object = (ESFunctionObject*)((size_t)extraPointerData | (size_t)1);
}

ScriptObject ScriptWrappable::scriptObjectSlowCase()
{
    void* extraPointerData = (void*)((size_t)m_object - 1);
    m_object = ESObject::create(0);
    STARFISH_ASSERT(!((size_t)m_object & (size_t)1));
    m_object->setExtraPointerData(extraPointerData);
    m_object->setExtraData(kEscargotObjectCheckMagic);

    Window* window = (Window*)ESVMInstance::currentInstance()
                         ->globalObject()
                         ->extraPointerData();
    initScriptObject(window->scriptBindingInstance());

    return m_object;
}

#ifdef STARFISH_ENABLE_TEST
static ESValue debugPauseFunction(ESVMInstance* instance)
{
    ESValue v = instance->currentExecutionContext()->resolveThisBinding();
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() == instance->globalObject()) {
        Window* wnd = (Window*)ESVMInstance::currentInstance()
                          ->globalObject()
                          ->extraPointerData();
        wnd->starFish()->messageLoop()->addIdlerWithNoScriptInstanceEntering(
            [](size_t, void* data, void*) {
                StarFish* sf = (StarFish*)data;
                sf->pause();
            },
            wnd->starFish(), nullptr);
    }
    return ESValue(ESValue::ESUndefined);
}

static ESValue debugResumeFunction(ESVMInstance* instance)
{
    ESValue v = instance->currentExecutionContext()->resolveThisBinding();
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() == instance->globalObject()) {
        Window* wnd = (Window*)ESVMInstance::currentInstance()
                          ->globalObject()
                          ->extraPointerData();
        wnd->starFish()->messageLoop()->addIdlerWithNoScriptInstanceEntering(
            [](size_t, void* data, void*) {
                StarFish* sf = (StarFish*)data;
                sf->resume();
            },
            wnd->starFish(), nullptr);
    }
    return ESValue(ESValue::ESUndefined);
}

static ESValue networkEnableFunction(ESVMInstance* instance)
{
    ESValue v = instance->currentExecutionContext()->resolveThisBinding();
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() == instance->globalObject()) {
        Window* wnd = (Window*)ESVMInstance::currentInstance()
                          ->globalObject()
                          ->extraPointerData();
        wnd->setNetworkState(true);
    }
    return ESValue(ESValue::ESUndefined);
}

static ESValue networkDisableFunction(ESVMInstance* instance)
{
    ESValue v = instance->currentExecutionContext()->resolveThisBinding();
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() == instance->globalObject()) {
        Window* wnd = (Window*)ESVMInstance::currentInstance()
                          ->globalObject()
                          ->extraPointerData();
        wnd->setNetworkState(false);
    }
    return ESValue(ESValue::ESUndefined);
}

static ESValue isPixelTestFunction(ESVMInstance* instance)
{
    ESValue v = instance->currentExecutionContext()->resolveThisBinding();
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() == instance->globalObject()) {
        if (getenv("PIXEL_TEST") && strlen(getenv("PIXEL_TEST"))) {
            return ESValue(ESValue::ESTrue);
        } else {
            return ESValue(ESValue::ESFalse);
        }
    }
    return ESValue(ESValue::ESUndefined);
}

static void screenShotTimeoutHandler(Window* wnd, void* data)
{
    ESFunctionObject* p = (ESFunctionObject*)data;
    callScriptFunction(p, {}, 0,
                       ESVMInstance::currentInstance()->globalObject());
}

static ESValue screenShotFunction(ESVMInstance* instance)
{
    ESValue v = instance->currentExecutionContext()->resolveThisBinding();
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() == instance->globalObject()) {
        Window* wnd = (Window*)ESVMInstance::currentInstance()
                          ->globalObject()
                          ->extraPointerData();
        std::string path =
            wnd->document()->documentURI()->baseURI()->utf8Data();
        path = path.substr(strlen("file://"));
        path += ESVMInstance::currentInstance()
                    ->currentExecutionContext()
                    ->readArgument(0)
                    .toString()
                    ->utf8Data();
        wnd->screenShot(path);
        wnd->setTimeout(
            screenShotTimeoutHandler, 100,
            instance->currentExecutionContext()->readArgument(1).asESPointer());
    }
    return ESValue(ESValue::ESUndefined);
}

static ESValue screenShotRelativePathFunction(ESVMInstance* instance)
{
    ESValue v = instance->currentExecutionContext()->resolveThisBinding();
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() == instance->globalObject()) {
        Window* wnd = (Window*)ESVMInstance::currentInstance()
                          ->globalObject()
                          ->extraPointerData();
        char buff[1024];
        getcwd(buff, 1024);
        String* path =
            String::fromUTF8(buff)
                ->concat(String::fromUTF8("/"))
                ->concat(String::fromUTF8(getenv("SCREEN_SHOT_FILE")
                                              ? getenv("SCREEN_SHOT_FILE")
                                              : ""));
        wnd->screenShot(path->utf8Data());
        callScriptFunction(instance->currentExecutionContext()->readArgument(0),
                           {}, 0, instance->globalObject());
    }
    return ESValue(ESValue::ESUndefined);
}

static ESValue forceDisableOnloadCaptureFunction(ESVMInstance* instance)
{
    ESValue v = instance->currentExecutionContext()->resolveThisBinding();
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() == instance->globalObject()) {
        Window* wnd = (Window*)ESVMInstance::currentInstance()
                          ->globalObject()
                          ->extraPointerData();
        wnd->forceDisableOnloadCapture();
    }
    return ESValue(ESValue::ESUndefined);
}

static ESValue getXYWHFunction(ESVMInstance* instance)
{
    ESValue v = instance->currentExecutionContext()->resolveThisBinding();
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() == instance->globalObject()) {
        Window* wnd = (Window*)ESVMInstance::currentInstance()
                          ->globalObject()
                          ->extraPointerData();
        wnd->renderingIfNeeds();
        Node* node = (Node*)instance->currentExecutionContext()
                         ->readArgument(0)
                         .asESPointer()
                         ->asESObject()
                         ->extraPointerData();
        Frame* fr = (Frame*)node->frame();
        if (!fr) {
            return ESValue(ESValue::ESNull);
        } else if (fr->isFrameBox()) {
            LayoutRect rect = fr->asFrameBox()->absoluteRect(
                node->document()->frame()->asFrameBox());
            ESObject* result = ESObject::create();
            result->set(ESString::create("x"), ESValue(rect.x().toFloat()));
            result->set(ESString::create("y"), ESValue(rect.y().toFloat()));
            result->set(ESString::create("width"),
                        ESValue(rect.width().toFloat()));
            result->set(ESString::create("height"),
                        ESValue(rect.height().toFloat()));
            return ESValue(result);
        } else {
            // TODO
        }
    }
    return ESValue(ESValue::ESUndefined);
}

static ESValue simulateClickFunction(ESVMInstance* instance)
{
    ESValue v = instance->currentExecutionContext()->resolveThisBinding();
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() == instance->globalObject()) {
        Window* wnd = (Window*)ESVMInstance::currentInstance()
                          ->globalObject()
                          ->extraPointerData();
        wnd->simulateClick(ESVMInstance::currentInstance()
                               ->currentExecutionContext()
                               ->readArgument(0)
                               .toNumber(),
                           ESVMInstance::currentInstance()
                               ->currentExecutionContext()
                               ->readArgument(1)
                               .toNumber());
    }
    return ESValue(ESValue::ESUndefined);
}

static ESValue simulateVisibilityChangeFunction(ESVMInstance* instance)
{
    ESValue v = instance->currentExecutionContext()->resolveThisBinding();
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() == instance->globalObject()) {
        Window* wnd = (Window*)ESVMInstance::currentInstance()
                          ->globalObject()
                          ->extraPointerData();
        wnd->simulateVisibilitychange(ESVMInstance::currentInstance()
                                          ->currentExecutionContext()
                                          ->readArgument(0)
                                          .toBoolean());
    }
    return ESValue(ESValue::ESUndefined);
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
static ESValue setTimeoutFunction(ESVMInstance* instance)
{
    ESValue v = instance->currentExecutionContext()->resolveThisBinding();

    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() == instance->globalObject()) {
        if (instance->currentExecutionContext()
                ->readArgument(0)
                .isESPointer() &&
            instance->currentExecutionContext()
                ->readArgument(0)
                .asESPointer() &&
            instance->currentExecutionContext()
                ->readArgument(0)
                .asESPointer()
                ->isESFunctionObject()) {
            Window* wnd = (Window*)ESVMInstance::currentInstance()
                              ->globalObject()
                              ->extraPointerData();
            if (instance->currentExecutionContext()
                    ->readArgument(1)
                    .isUndefinedOrNull()) {
                return ESValue(wnd->setTimeout(
                    timeoutHandler, 0, instance->currentExecutionContext()
                                           ->readArgument(0)
                                           .asESPointer()));
            } else if (instance->currentExecutionContext()
                           ->readArgument(1)
                           .isNumber()) {
                return ESValue(wnd->setTimeout(
                    timeoutHandler, instance->currentExecutionContext()
                                        ->readArgument(1)
                                        .toUint32(),
                    instance->currentExecutionContext()
                        ->readArgument(0)
                        .asESPointer()));
            }
        } else {
            String* bodyStr =
                toBrowserString(instance->currentExecutionContext()
                                    ->readArgument(0)
                                    .toString());
            String* name[] = { String::emptyString };
            bool error = false;
            ScriptValue m_listener =
                createScriptFunction(name, 1, bodyStr, error);
            Window* wnd = (Window*)ESVMInstance::currentInstance()
                              ->globalObject()
                              ->extraPointerData();
            if (instance->currentExecutionContext()
                    ->readArgument(1)
                    .isUndefinedOrNull()) {
                return ESValue(wnd->setTimeout(timeoutHandler, 0,
                                               m_listener.asESPointer()));
            } else if (instance->currentExecutionContext()
                           ->readArgument(1)
                           .isNumber()) {
                return ESValue(wnd->setTimeout(
                    timeoutHandler, instance->currentExecutionContext()
                                        ->readArgument(1)
                                        .toUint32(),
                    m_listener.asESPointer()));
            }
        }
    }
    return ESValue();
}

static ESValue clearTimeoutFunction(ESVMInstance* instance)
{
    ESValue v = instance->currentExecutionContext()->resolveThisBinding();
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() == instance->globalObject()) {
        if (instance->currentExecutionContext()->readArgument(0).isNumber()) {
            Window* wnd = (Window*)ESVMInstance::currentInstance()
                              ->globalObject()
                              ->extraPointerData();
            wnd->clearTimeout(instance->currentExecutionContext()
                                  ->readArgument(0)
                                  .toUint32());
        }
    }
    return ESValue(ESValue::ESUndefined);
}

static ESValue setIntervalFunction(ESVMInstance* instance)
{
    ESValue v = instance->currentExecutionContext()->resolveThisBinding();

    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() == instance->globalObject()) {
        if (instance->currentExecutionContext()
                ->readArgument(0)
                .isESPointer() &&
            instance->currentExecutionContext()
                ->readArgument(0)
                .asESPointer() &&
            instance->currentExecutionContext()
                ->readArgument(0)
                .asESPointer()
                ->isESFunctionObject()) {
            if (instance->currentExecutionContext()
                    ->readArgument(1)
                    .isNumber()) {
                Window* wnd = (Window*)ESVMInstance::currentInstance()
                                  ->globalObject()
                                  ->extraPointerData();
                return ESValue(wnd->setInterval(
                    timeoutHandler, instance->currentExecutionContext()
                                        ->readArgument(1)
                                        .toUint32(),
                    instance->currentExecutionContext()
                        ->readArgument(0)
                        .asESPointer()));
            }
        } else {
            String* bodyStr =
                toBrowserString(instance->currentExecutionContext()
                                    ->readArgument(0)
                                    .toString());
            String* name[] = { String::emptyString };
            bool error = false;
            ScriptValue m_listener =
                createScriptFunction(name, 1, bodyStr, error);

            Window* wnd = (Window*)ESVMInstance::currentInstance()
                              ->globalObject()
                              ->extraPointerData();
            if (instance->currentExecutionContext()
                    ->readArgument(1)
                    .isNumber()) {
                return ESValue(wnd->setInterval(
                    timeoutHandler, instance->currentExecutionContext()
                                        ->readArgument(1)
                                        .toUint32(),
                    m_listener.asESPointer()));
            }
        }
    }
    return ESValue();
}

static ESValue clearIntervalFunction(ESVMInstance* instance)
{
    ESValue v = instance->currentExecutionContext()->resolveThisBinding();
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() == instance->globalObject()) {
        if (instance->currentExecutionContext()->readArgument(0).isNumber()) {
            Window* wnd = (Window*)ESVMInstance::currentInstance()
                              ->globalObject()
                              ->extraPointerData();
            wnd->clearInterval(instance->currentExecutionContext()
                                   ->readArgument(0)
                                   .toUint32());
        }
    }
    return ESValue(ESValue::ESUndefined);
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
static ESValue requestAnimationFrameFunction(ESVMInstance* instance)
{
    ESValue v = instance->currentExecutionContext()->resolveThisBinding();
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() == instance->globalObject()) {
        if (instance->currentExecutionContext()
                ->readArgument(0)
                .isESPointer() &&
            instance->currentExecutionContext()
                ->readArgument(0)
                .asESPointer() &&
            instance->currentExecutionContext()
                ->readArgument(0)
                .asESPointer()
                ->isESFunctionObject()) {
            Window* wnd = (Window*)ESVMInstance::currentInstance()
                              ->globalObject()
                              ->extraPointerData();
            return ESValue(
                wnd->requestAnimationFrame(animationFrameTimeoutHandler,
                                           instance->currentExecutionContext()
                                               ->readArgument(0)
                                               .asESPointer()));
        }
    }
    return ESValue();
}

static ESValue cancelAnimationFrameFunction(ESVMInstance* instance)
{
    ESValue v = instance->currentExecutionContext()->resolveThisBinding();
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() == instance->globalObject()) {
        if (instance->currentExecutionContext()->readArgument(0).isNumber()) {
            Window* wnd = (Window*)ESVMInstance::currentInstance()
                              ->globalObject()
                              ->extraPointerData();
            wnd->cancelAnimationFrame(instance->currentExecutionContext()
                                          ->readArgument(0)
                                          .toUint32());
        }
    }
    return ESValue();
}

static ESValue innerWidthGetterFunction(ESObject* obj, ESObject* originalObj,
                                        ESString* propertyName)
{
    ESValue v = originalObj;
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() ==
            ESVMInstance::currentInstance()->globalObject()) {
        double innerWidth =
            ((Window*)originalObj->extraPointerData())->innerWidth();
        return ESValue(innerWidth);
    }
    return ESValue(0);
}

static ESValue innerHeightGetterFunction(ESObject* obj, ESObject* originalObj,
                                         ESString* propertyName)
{
    ESValue v = originalObj;
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() ==
            ESVMInstance::currentInstance()->globalObject()) {
        double innerHeight =
            ((Window*)originalObj->extraPointerData())->innerHeight();
        return ESValue(innerHeight);
    }
    return ESValue(0);
}

static ESValue scrollXGetterFunction(ESObject* obj, ESObject* originalObj,
                                     ESString* propertyName)
{
    ESValue v = originalObj;
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() ==
            ESVMInstance::currentInstance()->globalObject()) {
        double scrollX = ((Window*)originalObj->extraPointerData())->scrollX();
        return ESValue(scrollX);
    }
    return ESValue(0);
}

static ESValue pageXOffsetGetterFunction(ESObject* obj, ESObject* originalObj,
                                         ESString* propertyName)
{
    ESValue v = originalObj;
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() ==
            ESVMInstance::currentInstance()->globalObject()) {
        double scrollX = ((Window*)originalObj->extraPointerData())->scrollX();
        return ESValue(scrollX);
    }
    return ESValue(0);
}

static ESValue scrollYGetterFunction(ESObject* obj, ESObject* originalObj,
                                     ESString* propertyName)
{
    ESValue v = originalObj;
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() ==
            ESVMInstance::currentInstance()->globalObject()) {
        double scrollX = ((Window*)originalObj->extraPointerData())->scrollY();
        return ESValue(scrollX);
    }
    return ESValue(0);
}

static ESValue pageYOffsetGetterFunction(ESObject* obj, ESObject* originalObj,
                                         ESString* propertyName)
{
    ESValue v = originalObj;
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() ==
            ESVMInstance::currentInstance()->globalObject()) {
        double scrollX = ((Window*)originalObj->extraPointerData())->scrollY();
        return ESValue(scrollX);
    }
    return ESValue(0);
}

static ESValue onClickGetterFunction(ESObject* obj, ESObject* originalObj,
                                     ESString* propertyName)
{
    ESValue v = originalObj;
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() ==
            ESVMInstance::currentInstance()->globalObject()) {
        Window* wnd = (Window*)ESVMInstance::currentInstance()
                          ->globalObject()
                          ->extraPointerData();
        auto eventType = wnd->starFish()->staticStrings()->m_click;
        return wnd->attributeEventListener(eventType);
    }
    return ESValue();
}

static void onClickSetterFunction(ESObject* obj, ESObject* originalObj,
                                  ESString* propertyName, const ESValue& value)
{
    ESValue v = originalObj;
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() ==
            ESVMInstance::currentInstance()->globalObject()) {
        Window* wnd = (Window*)ESVMInstance::currentInstance()
                          ->globalObject()
                          ->extraPointerData();
        auto eventType = wnd->starFish()->staticStrings()->m_click;
        if (value.isObject() || (value.isESPointer() &&
                                 value.asESPointer()->isESFunctionObject())) {
            wnd->setAttributeEventListener(eventType, value);
        } else {
            wnd->clearAttributeEventListener(eventType);
        }
    }
}

static ESValue onMouseOverGetterFunction(ESObject* obj, ESObject* originalObj,
                                         ESString* propertyName)
{
    ESValue v = originalObj;
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() ==
            ESVMInstance::currentInstance()->globalObject()) {
        Window* wnd = (Window*)ESVMInstance::currentInstance()
                          ->globalObject()
                          ->extraPointerData();
        auto eventType = wnd->starFish()->staticStrings()->m_mouseover;
        return wnd->attributeEventListener(eventType);
    }
    return ESValue();
}

static void onMouseOverSetterFunction(ESObject* obj, ESObject* originalObj,
                                      ESString* propertyName,
                                      const ESValue& value)
{
    ESValue v = originalObj;
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() ==
            ESVMInstance::currentInstance()->globalObject()) {
        Window* wnd = (Window*)ESVMInstance::currentInstance()
                          ->globalObject()
                          ->extraPointerData();
        auto eventType = wnd->starFish()->staticStrings()->m_mouseover;
        if (value.isObject() || (value.isESPointer() &&
                                 value.asESPointer()->isESFunctionObject())) {
            wnd->setAttributeEventListener(eventType, value);
        } else {
            wnd->clearAttributeEventListener(eventType);
        }
    }
}

static ESValue onKeyDownGetterFunction(ESObject* obj, ESObject* originalObj,
                                       ESString* propertyName)
{
    ESValue v = originalObj;
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() ==
            ESVMInstance::currentInstance()->globalObject()) {
        Window* wnd = (Window*)ESVMInstance::currentInstance()
                          ->globalObject()
                          ->extraPointerData();
        auto eventType = wnd->starFish()->staticStrings()->m_keydown;
        return wnd->attributeEventListener(eventType);
    }
    return ESValue();
}

static void onKeyDownSetterFunction(ESObject* obj, ESObject* originalObj,
                                    ESString* propertyName,
                                    const ESValue& value)
{
    ESValue v = originalObj;
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() ==
            ESVMInstance::currentInstance()->globalObject()) {
        Window* wnd = (Window*)ESVMInstance::currentInstance()
                          ->globalObject()
                          ->extraPointerData();
        auto eventType = wnd->starFish()->staticStrings()->m_keydown;
        if (value.isObject() || (value.isESPointer() &&
                                 value.asESPointer()->isESFunctionObject())) {
            wnd->setAttributeEventListener(eventType, value);
        } else {
            wnd->clearAttributeEventListener(eventType);
        }
    }
}

static ESValue onFocusGetterFunction(ESObject* obj, ESObject* originalObj,
                                     ESString* propertyName)
{
    ESValue v = originalObj;
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() ==
            ESVMInstance::currentInstance()->globalObject()) {
        Window* wnd = (Window*)ESVMInstance::currentInstance()
                          ->globalObject()
                          ->extraPointerData();
        auto eventType = wnd->starFish()->staticStrings()->m_focus;
        return wnd->attributeEventListener(eventType);
    }
    return ESValue();
}

static void onFocusSetterFunction(ESObject* obj, ESObject* originalObj,
                                  ESString* propertyName, const ESValue& value)
{
    ESValue v = originalObj;
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() ==
            ESVMInstance::currentInstance()->globalObject()) {
        Window* wnd = (Window*)ESVMInstance::currentInstance()
                          ->globalObject()
                          ->extraPointerData();
        auto eventType = wnd->starFish()->staticStrings()->m_focus;
        if (value.isObject() || (value.isESPointer() &&
                                 value.asESPointer()->isESFunctionObject())) {
            wnd->setAttributeEventListener(eventType, value);
        } else {
            wnd->clearAttributeEventListener(eventType);
        }
    }
}

static ESValue onLoadGetterFunction(ESObject* obj, ESObject* originalObj,
                                    ESString* propertyName)
{
    ESValue v = originalObj;
    if (v.isUndefinedOrNull() ||
        v.asESPointer()->asESObject() ==
            ESVMInstance::currentInstance()->globalObject()) {
        Window* wnd = (Window*)ESVMInstance::currentInstance()
                          ->globalObject()
                          ->extraPointerData();
        auto eventType = wnd->starFish()->staticStrings()->m_load;
        return wnd->attributeEventListener(eventType);
    }
    return ESValue();
}

static void onLoadSetterFunction(ESObject* obj, ESObject* originalObj,
                                 ESString* propertyName, const ESValue& value)
{
    ESValue v = originalObj;
    if (v.isObject() &&
        v.asESPointer() == ESVMInstance::currentInstance()->globalObject()) {
        Window* wnd = (Window*)ESVMInstance::currentInstance()
                          ->globalObject()
                          ->extraPointerData();
        auto eventType = wnd->starFish()->staticStrings()->m_load;
        if (value.isObject() || (value.isESPointer() &&
                                 value.asESPointer()->isESFunctionObject())) {
            wnd->setAttributeEventListener(eventType, value);
        } else {
            wnd->clearAttributeEventListener(eventType);
        }
    }
}

static ESValue onUnLoadGetterFunction(ESObject* obj, ESObject* originalObj,
                                      ESString* propertyName)
{
    ESValue v = originalObj;
    if (v.isObject() &&
        v.asESPointer() == ESVMInstance::currentInstance()->globalObject()) {
        Window* wnd = (Window*)ESVMInstance::currentInstance()
                          ->globalObject()
                          ->extraPointerData();
        auto eventType = wnd->starFish()->staticStrings()->m_unload;
        return wnd->attributeEventListener(eventType);
    }
    return ESValue();
}

static void onUnLoadSetterFunction(ESObject* obj, ESObject* originalObj,
                                   ESString* propertyName, const ESValue& value)
{
    ESValue v = originalObj;
    if (v.isObject() &&
        v.asESPointer() == ESVMInstance::currentInstance()->globalObject()) {
        Window* wnd = (Window*)ESVMInstance::currentInstance()
                          ->globalObject()
                          ->extraPointerData();
        auto eventType = wnd->starFish()->staticStrings()->m_unload;
        if (value.isObject() || (value.isESPointer() &&
                                 value.asESPointer()->isESFunctionObject())) {
            wnd->setAttributeEventListener(eventType, value);
        } else {
            wnd->clearAttributeEventListener(eventType);
        }
    }
}

static ESValue windowReadCallbackFunction(const ESValue& key, ESObject* obj)
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

static bool windowWriteCallbackFunction(const ESValue& key, const ESValue& val,
                                        ESObject* obj)
{
    STARFISH_ASSERT(obj == ESVMInstance::currentInstance()->globalObject());
    return false;
}

static ESValueVector windowEnumerateCallbackFunction(ESObject* obj)
{
    STARFISH_ASSERT(obj == ESVMInstance::currentInstance()->globalObject());
    size_t len = 0;
    ESValueVector v(len);
    return v;
}

void ScriptWrappable::initScriptWrappable(Window* window)
{
    m_object = ESVMInstance::currentInstance()->globalObject();
    auto data = fetchData(window->scriptBindingInstance());
    scriptObject()->set__proto__(data->m_fnWindow->protoType());
    scriptObject()->setExtraData(kEscargotObjectCheckMagic);
    scriptObject()->setExtraPointerData(window);

#ifdef STARFISH_ENABLE_TEST
    ((ESObject*)this->m_object)
        ->defineDataProperty(
            ESString::create("debugPause"), true, true, true,
            ESFunctionObject::create(NULL, debugPauseFunction,
                                     ESString::create("debugPause"), 0, false));

    ((ESObject*)this->m_object)
        ->defineDataProperty(ESString::create("debugResume"), true, true, true,
                             ESFunctionObject::create(
                                 NULL, debugResumeFunction,
                                 ESString::create("debugResume"), 0, false));

    ((ESObject*)this->m_object)
        ->defineDataProperty(
            ESString::create("networkEnable"), true, true, true,
            ESFunctionObject::create(NULL, networkEnableFunction,
                                     ESString::create("networkEnable"), 0,
                                     false));

    ((ESObject*)this->m_object)
        ->defineDataProperty(
            ESString::create("networkDisable"), true, true, true,
            ESFunctionObject::create(NULL, networkDisableFunction,
                                     ESString::create("networkDisable"), 0,
                                     false));

    ((ESObject*)this->m_object)
        ->defineDataProperty(ESString::create("isPixelTest"), true, true, true,
                             ESFunctionObject::create(
                                 NULL, isPixelTestFunction,
                                 ESString::create("isPixelTest"), 0, false));

    ((ESObject*)this->m_object)
        ->defineDataProperty(
            ESString::create("screenShot"), true, true, true,
            ESFunctionObject::create(NULL, screenShotFunction,
                                     ESString::create("screenShot"), 0, false));

    ((ESObject*)this->m_object)
        ->defineDataProperty(
            ESString::create("screenShotRelativePath"), true, true, true,
            ESFunctionObject::create(NULL, screenShotRelativePathFunction,
                                     ESString::create("screenShotRelativePath"),
                                     0, false));

    ((ESObject*)this->m_object)
        ->defineDataProperty(
            ESString::create("forceDisableOnloadCapture"), true, true, true,
            ESFunctionObject::create(
                NULL, forceDisableOnloadCaptureFunction,
                ESString::create("forceDisableOnloadCapture"), 0, false));

    ((ESObject*)this->m_object)
        ->defineDataProperty(
            ESString::create("getXYWH"), true, true, true,
            ESFunctionObject::create(NULL, getXYWHFunction,
                                     ESString::create("getXYWH"), 2, false));

    ((ESObject*)this->m_object)
        ->defineDataProperty(
            ESString::create("simulateClick"), true, true, true,
            ESFunctionObject::create(NULL, simulateClickFunction,
                                     ESString::create("simulateClick"), 2,
                                     false));

    ((ESObject*)this->m_object)
        ->defineDataProperty(
            ESString::create("simulateVisibilitychange"), true, true, true,
            ESFunctionObject::create(
                NULL, simulateVisibilityChangeFunction,
                ESString::create("simulateVisibilitychange"), 0, false));

    ((ESObject*)this->m_object)
        ->defineDataProperty(
            ESString::create("testAssert"), true, true, true,
            ESFunctionObject::create(NULL, testAssertFunction,
                                     ESString::create("testAssert"), 0, false));

    ((ESObject*)this->m_object)
        ->defineDataProperty(
            ESString::create("testEnd"), true, true, true,
            ESFunctionObject::create(NULL, testEndFunction,
                                     ESString::create("testEnd"), 0, false));

#endif

    ((ESObject*)this->m_object)
        ->defineDataProperty(
            ESString::create("testImgDiff"), true, true, true,
            ESFunctionObject::create(NULL, testImgDiffFunction,
                                     ESString::create("testEnd"), 0, false));

    // [setTimeout]
    // https://www.w3.org/TR/html5/webappapis.html#dom-windowtimers-settimeout
    // long setTimeout(Function handler, optional long timeout, any...
    // arguments);

    ((ESObject*)this->m_object)
        ->defineDataProperty(
            ESString::create("setTimeout"), true, true, true,
            ESFunctionObject::create(NULL, setTimeoutFunction,
                                     ESString::create("setTimeout"), 1, false));

    // [clearTimeout]
    // https://www.w3.org/TR/html5/webappapis.html#dom-windowtimers-cleartimeout
    ((ESObject*)this->m_object)
        ->defineDataProperty(ESString::create("clearTimeout"), true, true, true,
                             ESFunctionObject::create(
                                 NULL, clearTimeoutFunction,
                                 ESString::create("clearTimeout"), 0, false));

    // https://www.w3.org/TR/html5/webappapis.html#dom-windowtimers-setinterval
    ((ESObject*)this->m_object)
        ->defineDataProperty(ESString::create("setInterval"), true, true, true,
                             ESFunctionObject::create(
                                 NULL, setIntervalFunction,
                                 ESString::create("setInterval"), 1, false));

    // https://www.w3.org/TR/html5/webappapis.html#dom-windowtimers-clearinterval
    ((ESObject*)this->m_object)
        ->defineDataProperty(
            ESString::create("clearInterval"), true, true, true,
            ESFunctionObject::create(NULL, clearIntervalFunction,
                                     ESString::create("clearInterval"), 0,
                                     false));

    ((ESObject*)this->m_object)
        ->defineDataProperty(
            ESString::create("requestAnimationFrame"), false, false, false,
            ESFunctionObject::create(NULL, requestAnimationFrameFunction,
                                     ESString::create("requestAnimationFrame"),
                                     1, false));

    // https://www.w3.org/TR/html5/webappapis.html
    ((ESObject*)this->m_object)
        ->defineDataProperty(
            ESString::create("cancelAnimationFrame"), false, false, false,
            ESFunctionObject::create(NULL, cancelAnimationFrameFunction,
                                     ESString::create("cancelAnimationFrame"),
                                     1, false));

    // https://www.w3.org/TR/cssom-view/#dom-window-innerwidth
    ((ESObject*)this->m_object)
        ->defineAccessorProperty(ESString::create("innerWidth"),
                                 innerWidthGetterFunction, NULL, true, true,
                                 true);

    // https://www.w3.org/TR/cssom-view/#dom-window-innerheight
    ((ESObject*)this->m_object)
        ->defineAccessorProperty(ESString::create("innerHeight"),
                                 innerHeightGetterFunction, NULL, true, true,
                                 true);

    // https://drafts.csswg.org/cssom-view/#dom-window-scrollx
    ((ESObject*)this->m_object)
        ->defineAccessorProperty(ESString::create("scrollX"),
                                 scrollXGetterFunction, NULL, true, true, true);

    // https://drafts.csswg.org/cssom-view/#dom-window-pagexoffset
    ((ESObject*)this->m_object)
        ->defineAccessorProperty(ESString::create("pageXOffset"),
                                 pageXOffsetGetterFunction, NULL, true, true,
                                 true);

    // https://drafts.csswg.org/cssom-view/#dom-window-scrolly
    ((ESObject*)this->m_object)
        ->defineAccessorProperty(ESString::create("scrollY"),
                                 scrollYGetterFunction, NULL, true, true, true);

    // https://drafts.csswg.org/cssom-view/#dom-window-pageyoffset
    ((ESObject*)this->m_object)
        ->defineAccessorProperty(ESString::create("pageYOffset"),
                                 pageYOffsetGetterFunction, NULL, true, true,
                                 true);

    ((ESObject*)this->m_object)
        ->defineAccessorProperty(ESString::create("onclick"),
                                 onClickGetterFunction, onClickSetterFunction,
                                 true, true, true);

    ((ESObject*)this->m_object)
        ->defineAccessorProperty(ESString::create("onmouseover"),
                                 onMouseOverGetterFunction,
                                 onMouseOverSetterFunction, true, true, true);

    ((ESObject*)this->m_object)
        ->defineAccessorProperty(ESString::create("onkeydown"),
                                 onKeyDownGetterFunction,
                                 onKeyDownSetterFunction, true, true, true);

    ((ESObject*)this->m_object)
        ->defineAccessorProperty(ESString::create("onfocus"),
                                 onFocusGetterFunction, onFocusSetterFunction,
                                 true, true, true);

    ((ESObject*)this->m_object)
        ->defineAccessorProperty(ESString::create("onload"),
                                 onLoadGetterFunction, onLoadSetterFunction,
                                 true, true, true);

    ((ESObject*)this->m_object)
        ->defineAccessorProperty(ESString::create("onunload"),
                                 onUnLoadGetterFunction, onUnLoadSetterFunction,
                                 true, true, true);

    scriptObject()->setPropertyInterceptor(windowReadCallbackFunction,
                                           windowWriteCallbackFunction,
                                           windowEnumerateCallbackFunction);
}

void ScriptWrappable::initScriptWrappable(Node* ptr)
{
    Node* node = (Node*)this;
    initScriptWrappable(ptr, node->document()->scriptBindingInstance());
}

void ScriptWrappable::initScriptWrappable(Node* ptr,
                                          ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnNode()->protoType());
}

void ScriptWrappable::initScriptWrappable(Element* element)
{
    Node* node = (Node*)this;
    initScriptWrappable(element, node->document()->scriptBindingInstance());
}

void ScriptWrappable::initScriptWrappable(DocumentType* element)
{
    auto data = fetchData(element->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnDocumentType()->protoType());
}

void ScriptWrappable::initScriptWrappable(Element* element,
                                          ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnElement()->protoType());
}

void ScriptWrappable::initScriptWrappable(Document*)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnDocument()->protoType());
}

void ScriptWrappable::initScriptWrappable(DocumentFragment* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnDocumentFragment()->protoType());
}

#ifdef STARFISH_EXP
void ScriptWrappable::initScriptWrappable(DOMImplementation* ptr,
                                          ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->m_fnDOMImplementation->protoType());
}
#endif

void ScriptWrappable::initScriptWrappable(Location* ptr)
{
    Location* nav = (Location*)this;
    auto data = fetchData(nav->starFish()->window()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnLocation()->protoType());
}

void ScriptWrappable::initScriptWrappable(Navigator* ptr)
{
    Navigator* nav = (Navigator*)this;
    auto data = fetchData(nav->starFish()->window()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnNavigator()->protoType());
}

void ScriptWrappable::initScriptWrappable(History* ptr)
{
    History* history = (History*)this;
    auto data =
        fetchData(history->starFish()->window()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHistory()->protoType());
}

void ScriptWrappable::initScriptWrappable(Geolocation* ptr)
{
    Geolocation* nav = (Geolocation*)this;
    auto data = fetchData(nav->starFish()->window()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnGeolocation()->protoType());
}

void ScriptWrappable::initScriptWrappable(Geoposition* ptr)
{
    auto data = fetchData(ptr->starFish()->window()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnGeoposition()->protoType());
}

void ScriptWrappable::initScriptWrappable(Coordinates* ptr)
{
    auto data = fetchData(ptr->starFish()->window()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnCoordinates()->protoType());
}

void ScriptWrappable::initScriptWrappable(PositionError* ptr)
{
    auto data = fetchData(ptr->starFish()->window()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnPositionError()->protoType());
}

#ifdef STARFISH_ENABLE_DOMPARSER
void ScriptWrappable::initScriptWrappable(DOMParser* ptr)
{
    auto data = fetchData(ptr->starFish()->window()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnDOMParser()->protoType());
}
#endif

void ScriptWrappable::initScriptWrappable(HTMLDocument*)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHTMLDocument()->protoType());
}

void ScriptWrappable::initScriptWrappable(CharacterData* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnCharacterData()->protoType());
}

void ScriptWrappable::initScriptWrappable(Text* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnText()->protoType());
}

void ScriptWrappable::initScriptWrappable(CDataSection* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnCDataSection()->protoType());
}

void ScriptWrappable::initScriptWrappable(Comment* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnComment()->protoType());
}

#ifdef STARFISH_ENABLE_MULTI_PAGE
void ScriptWrappable::initScriptWrappable(HTMLAnchorElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHTMLAnchorElement()->protoType());
}
#endif

#ifdef STARFISH_ENABLE_MULTIMEDIA
void ScriptWrappable::initScriptWrappable(HTMLMediaElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHTMLMediaElement()->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLVideoElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHTMLVideoElement()->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLAudioElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHTMLAudioElement()->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLTrackElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHTMLTrackElement()->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLSourceElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHTMLSourceElement()->protoType());
}

void ScriptWrappable::initScriptWrappable(TextTrack* ptr)
{
    Window* window = (Window*)ESVMInstance::currentInstance()
                         ->globalObject()
                         ->extraPointerData();
    ScriptBindingInstance* instance = window->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnTextTrack()->protoType());
}

static ESValue textTrackListReadCallbackFunction(const ESValue& key,
                                                 ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    TextTrackList* self = (TextTrackList*)obj->extraPointerData();
    STARFISH_ASSERT(self->isTextTrackList());
    uint32_t idx = key.toIndex();
    if (idx != ESValue::ESInvalidIndexValue && idx < self->size()) {
        TextTrack* e = (*self)[idx];
        if (e != nullptr) {
            return e->scriptValue();
        }
    }
    return ESValue(ESValue::ESDeletedValue);
}

static bool textTrackListWriteCallbackFunction(const ESValue& key,
                                               const ESValue& val,
                                               ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    return false;
}

static ESValueVector textTrackListEnumerateCallbackFunction(ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    TextTrackList* self = (TextTrackList*)obj->extraPointerData();
    STARFISH_ASSERT(self->isTextTrackList());
    size_t len = self->size();
    ESValueVector v(len);
    for (size_t i = 0; i < len; i++) {
        v[i] = ESValue(i);
    }
    return v;
}

void ScriptWrappable::initScriptWrappable(TextTrackList* ptr)
{
    Window* window = (Window*)ESVMInstance::currentInstance()
                         ->globalObject()
                         ->extraPointerData();
    ScriptBindingInstance* instance = window->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnTextTrackList()->protoType());

    scriptObject()->setPropertyInterceptor(
        textTrackListReadCallbackFunction, textTrackListWriteCallbackFunction,
        textTrackListEnumerateCallbackFunction, true);
}

void ScriptWrappable::initScriptWrappable(TextTrackCue* ptr)
{
    Window* window = (Window*)ESVMInstance::currentInstance()
                         ->globalObject()
                         ->extraPointerData();
    ScriptBindingInstance* instance = window->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnTextTrackCue()->protoType());
}

static ESValue textTrackCueListReadCallbackFunction(const ESValue& key,
                                                    ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    TextTrackCueList* self = (TextTrackCueList*)obj->extraPointerData();
    STARFISH_ASSERT(self->isTextTrackCueList());
    uint32_t idx = key.toIndex();
    if (idx != ESValue::ESInvalidIndexValue && idx < self->size()) {
        TextTrackCue* e = (*self)[idx];
        if (e != nullptr) {
            return e->scriptValue();
        }
    }
    return ESValue(ESValue::ESDeletedValue);
}

static bool textTrackCueListWriteCallbackFunction(const ESValue& key,
                                                  const ESValue& val,
                                                  ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    return false;
}

static ESValueVector textTrackCueListEnumerateCallbackFunction(ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    TextTrackCueList* self = (TextTrackCueList*)obj->extraPointerData();
    STARFISH_ASSERT(self->isTextTrackCueList());
    size_t len = self->size();
    ESValueVector v(len);
    for (size_t i = 0; i < len; i++) {
        v[i] = ESValue(i);
    }
    return v;
}

void ScriptWrappable::initScriptWrappable(TextTrackCueList* ptr)
{
    Window* window = (Window*)ESVMInstance::currentInstance()
                         ->globalObject()
                         ->extraPointerData();
    ScriptBindingInstance* instance = window->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnTextTrackCueList()->protoType());

    scriptObject()->setPropertyInterceptor(
        textTrackCueListReadCallbackFunction,
        textTrackCueListWriteCallbackFunction,
        textTrackCueListEnumerateCallbackFunction, true);
}

void ScriptWrappable::initScriptWrappable(VTTCue* ptr)
{
    Window* window = (Window*)ESVMInstance::currentInstance()
                         ->globalObject()
                         ->extraPointerData();
    ScriptBindingInstance* instance = window->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnVTTCue()->protoType());
}

void ScriptWrappable::initScriptWrappable(TimeRanges* ptr)
{
    Window* window = (Window*)ESVMInstance::currentInstance()
                         ->globalObject()
                         ->extraPointerData();
    ScriptBindingInstance* instance = window->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnTimeRanges()->protoType());
}

void ScriptWrappable::initScriptWrappable(MediaSource* ptr)
{
    Window* window = (Window*)ESVMInstance::currentInstance()
                         ->globalObject()
                         ->extraPointerData();
    ScriptBindingInstance* instance = window->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnMediaSource()->protoType());
}

void ScriptWrappable::initScriptWrappable(SourceBuffer* ptr)
{
    Window* window = (Window*)ESVMInstance::currentInstance()
                         ->globalObject()
                         ->extraPointerData();
    ScriptBindingInstance* instance = window->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnSourceBuffer()->protoType());
}

static ESValue sourceBufferListReadCallbackFunction(const ESValue& key,
                                                    ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    SourceBufferList* self = (SourceBufferList*)obj->extraPointerData();
    STARFISH_ASSERT(self->isSourceBufferList());
    uint32_t idx = key.toIndex();
    if (idx != ESValue::ESInvalidIndexValue && idx < self->length()) {
        SourceBuffer* e = (*self)[idx];
        STARFISH_ASSERT(e);
        return e->scriptValue();
    }
    return ESValue(ESValue::ESDeletedValue);
}

static bool sourceBufferListWriteCallbackFunction(const ESValue& key,
                                                  const ESValue& val,
                                                  ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    return false;
}

static ESValueVector sourceBufferListEnumerateCallbackFunction(ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    SourceBufferList* self = (SourceBufferList*)obj->extraPointerData();
    STARFISH_ASSERT(self->isSourceBufferList());
    size_t len = self->length();
    ESValueVector v(len);
    for (size_t i = 0; i < len; i++) {
        v[i] = ESValue(i);
    }
    return v;
}

void ScriptWrappable::initScriptWrappable(SourceBufferList* ptr)
{
    Window* window = (Window*)ESVMInstance::currentInstance()
                         ->globalObject()
                         ->extraPointerData();
    ScriptBindingInstance* instance = window->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnSourceBufferList()->protoType());

    scriptObject()->setPropertyInterceptor(
        sourceBufferListReadCallbackFunction,
        sourceBufferListWriteCallbackFunction,
        sourceBufferListEnumerateCallbackFunction, true);
}
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
void ScriptWrappable::initScriptWrappable(WebApis* ptr)
{
    WebApis* webApis = (WebApis*)this;
    auto data =
        fetchData(webApis->starFish()->window()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnWebApis()->protoType());
}
void ScriptWrappable::initScriptWrappable(Avplay* ptr)
{
    Avplay* avPlay = (Avplay*)this;
    auto data =
        fetchData(avPlay->starFish()->window()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnAvPlay()->protoType());
}
#endif
#endif

void ScriptWrappable::initScriptWrappable(HTMLElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHTMLElement()->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLHtmlElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHTMLHtmlElement()->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLHeadElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHTMLHeadElement()->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLBodyElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHTMLBodyElement()->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLStyleElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHTMLStyleElement()->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLLinkElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHTMLLinkElement()->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLScriptElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHTMLScriptElement()->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLImageElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHTMLImageElement()->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLDivElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHTMLDivElement()->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLBRElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHTMLBRElement()->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLObjectElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHTMLObjectElement()->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLMetaElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHTMLMetaElement()->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLParagraphElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHTMLParagraphElement()->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLPreElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHTMLPreElement()->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLSpanElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHTMLSpanElement()->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLUnknownElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnHTMLUnknownElement()->protoType());
}

void ScriptWrappable::initScriptWrappable(PseudoElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnPseudoElement()->protoType());
}

void ScriptWrappable::initScriptWrappable(XMLHttpRequest* xhr)
{
    ScriptBindingInstance* instance =
        xhr->networkRequest().document()->window()->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnXMLHttpRequest()->protoType());
}

void ScriptWrappable::initScriptWrappable(Blob* blob)
{
    Window* window = (Window*)ESVMInstance::currentInstance()
                         ->globalObject()
                         ->extraPointerData();
    ScriptBindingInstance* instance = window->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnBlob()->protoType());
}

void ScriptWrappable::initScriptWrappable(URL* url,
                                          ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnURL()->protoType());
}

void ScriptWrappable::initScriptWrappable(DOMRectReadOnly* rect,
                                          ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnDOMRectReadOnly()->protoType());
}

void ScriptWrappable::initScriptWrappable(DOMRect* rect,
                                          ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnDOMRect()->protoType());
}

void ScriptWrappable::initScriptWrappable(DOMPointReadOnly* point,
                                          ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnDOMPointReadOnly()->protoType());
}

void ScriptWrappable::initScriptWrappable(DOMPoint* point,
                                          ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnDOMPoint()->protoType());
}

void ScriptWrappable::initScriptWrappable(DOMQuad* quad,
                                          ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnDOMQuad()->protoType());
}

static ESValue domRectListReadCallbackFunction(const ESValue& key,
                                               ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    DOMRectList* self = (DOMRectList*)obj->extraPointerData();
    STARFISH_ASSERT(self->isDOMRectList());
    uint32_t idx = key.toIndex();
    if (idx < self->length()) {
        return self->item(idx)->scriptValue();
    }
    return ESValue(ESValue::ESDeletedValue);
}

static bool domRectListWriteCallbackFunction(const ESValue& key,
                                             const ESValue& val, ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    return false;
}

static ESValueVector domRectListEnumerateCallbackFunction(ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    DOMRectList* self = (DOMRectList*)obj->extraPointerData();
    STARFISH_ASSERT(self->isDOMRectList());
    size_t len = self->length();
    ESValueVector v(len);
    for (size_t i = 0; i < len; i++) {
        v[i] = ESValue(i);
    }
    return v;
}

void ScriptWrappable::initScriptWrappable(DOMRectList* list,
                                          ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnDOMRectList()->protoType());

    scriptObject()->setPropertyInterceptor(
        domRectListReadCallbackFunction, domRectListWriteCallbackFunction,
        domRectListEnumerateCallbackFunction, true);
}

void ScriptWrappable::initScriptWrappable(DOMException* exception,
                                          ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnDOMException()->protoType());

    scriptObject()->defineDataProperty(ESString::create("code"), false, false,
                                       false, ESValue(exception->code()));
}

bool ScriptWrappable::hasProperty(String* name)
{
    return m_object->ESObject::hasProperty(createScriptString(name));
}

void ScriptWrappable::initScriptWrappable(Event* event)
{
    Window* window = (Window*)ESVMInstance::currentInstance()
                         ->globalObject()
                         ->extraPointerData();
    ScriptBindingInstance* instance = window->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnEvent()->protoType());
}

void ScriptWrappable::initScriptWrappable(UIEvent* ptr)
{
    Window* window = (Window*)ESVMInstance::currentInstance()
                         ->globalObject()
                         ->extraPointerData();
    ScriptBindingInstance* instance = window->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnUIEvent()->protoType());
}

void ScriptWrappable::initScriptWrappable(MouseEvent* ptr)
{
    Window* window = (Window*)ESVMInstance::currentInstance()
                         ->globalObject()
                         ->extraPointerData();
    ScriptBindingInstance* instance = window->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnMouseEvent()->protoType());
}

void ScriptWrappable::initScriptWrappable(TouchEvent* ptr)
{
    Window* window = (Window*)ESVMInstance::currentInstance()
                         ->globalObject()
                         ->extraPointerData();
    ScriptBindingInstance* instance = window->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnTouchEvent()->protoType());
}

void ScriptWrappable::initScriptWrappable(KeyboardEvent* ptr)
{
    Window* window = (Window*)ESVMInstance::currentInstance()
                         ->globalObject()
                         ->extraPointerData();
    ScriptBindingInstance* instance = window->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnKeyboardEvent()->protoType());
}

void ScriptWrappable::initScriptWrappable(FocusEvent* ptr)
{
    Window* window = (Window*)ESVMInstance::currentInstance()
                         ->globalObject()
                         ->extraPointerData();
    ScriptBindingInstance* instance = window->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnFocusEvent()->protoType());
}

void ScriptWrappable::initScriptWrappable(ProgressEvent* ptr)
{
    Window* window = (Window*)ESVMInstance::currentInstance()
                         ->globalObject()
                         ->extraPointerData();
    ScriptBindingInstance* instance = window->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnProgressEvent()->protoType());
}

static ESValue htmlCollectionReadCallbackFunction(const ESValue& key,
                                                  ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    HTMLCollection* self = (HTMLCollection*)obj->extraPointerData();
    STARFISH_ASSERT(self->isHTMLCollection());
    uint32_t idx = key.toIndex();
    if (idx == ESValue::ESInvalidIndexValue) {
        Element* e = self->namedItem(toBrowserString(key));
        if (e != nullptr) {
            return e->scriptValue();
        }
    } else if (idx < self->length()) {
        return self->item(idx)->scriptValue();
    }
    return ESValue(ESValue::ESDeletedValue);
}

static bool htmlCollectionWriteCallbackFunction(const ESValue& key,
                                                const ESValue& val,
                                                ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    return false;
}

static ESValueVector htmlCollectionEnumerateCallbackFunction(ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    HTMLCollection* self = (HTMLCollection*)obj->extraPointerData();
    STARFISH_ASSERT(self->isHTMLCollection());
    size_t len = self->length();
    ESValueVector v(len);
    for (size_t i = 0; i < len; i++) {
        v[i] = ESValue(i);
    }
    return v;
}

void ScriptWrappable::initScriptWrappable(HTMLCollection* ptr,
                                          ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnHTMLCollection()->protoType());

    scriptObject()->setPropertyInterceptor(
        htmlCollectionReadCallbackFunction, htmlCollectionWriteCallbackFunction,
        htmlCollectionEnumerateCallbackFunction, true);
}

static ESValue nodeListReadCallbackFunction(const ESValue& key, ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    NodeList* self = (NodeList*)obj->extraPointerData();
    STARFISH_ASSERT(self->isNodeList());
    uint32_t idx = key.toIndex();
    if (idx < self->length()) {
        return self->item(idx)->scriptValue();
    }
    return ESValue(ESValue::ESDeletedValue);
}

static bool nodeListWriteCallbackFunction(const ESValue& key,
                                          const ESValue& val, ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    return false;
}

static ESValueVector nodeListEnumerateCallbackFunction(ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    NodeList* self = (NodeList*)obj->extraPointerData();
    STARFISH_ASSERT(self->isNodeList());
    size_t len = self->length();
    ESValueVector v(len);
    for (size_t i = 0; i < len; i++) {
        v[i] = ESValue(i);
    }
    return v;
}

void ScriptWrappable::initScriptWrappable(NodeList* ptr,
                                          ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnNodeList()->protoType());

    scriptObject()->setPropertyInterceptor(
        nodeListReadCallbackFunction, nodeListWriteCallbackFunction,
        nodeListEnumerateCallbackFunction, true);
}

static ESValue domTokenListReadCallbackFunction(const ESValue& key,
                                                ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    DOMTokenList* self = (DOMTokenList*)obj->extraPointerData();
    STARFISH_ASSERT(self->isDOMTokenList());
    uint32_t idx = key.toIndex();
    if (idx < self->length()) {
        Nullable<String*> result = self->item(idx);
        if (result.hasValue()) {
            return createScriptString(result.getValue());
        }
    }
    return ESValue(ESValue::ESDeletedValue);
}

static bool domTokenListWriteCallbackFunction(const ESValue& key,
                                              const ESValue& val, ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    return false;
}

static ESValueVector domTokenListEnumerateCallbackFunction(ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    DOMTokenList* self = (DOMTokenList*)obj->extraPointerData();
    STARFISH_ASSERT(self->isDOMTokenList());
    size_t len = self->length();
    ESValueVector v(len);
    for (size_t i = 0; i < len; i++) {
        v[i] = ESValue(i);
    }
    return v;
}

void ScriptWrappable::initScriptWrappable(DOMTokenList* ptr,
                                          ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnDOMTokenList()->protoType());

    scriptObject()->setPropertyInterceptor(
        domTokenListReadCallbackFunction, domTokenListWriteCallbackFunction,
        domTokenListEnumerateCallbackFunction, true);
}

void ScriptWrappable::initScriptWrappable(DOMSettableTokenList* ptr,
                                          ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnDOMSettableTokenList()->protoType());
}

static ESValue namedNodeMapReadCallbackFunction(const ESValue& key,
                                                ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    NamedNodeMap* self = (NamedNodeMap*)obj->extraPointerData();
    STARFISH_ASSERT(self->isNamedNodeMap());
    uint32_t idx = key.toIndex();
    if (idx == ESValue::ESInvalidIndexValue) {
        String* str = toBrowserString(key);
        auto attrName = self->element()->document()->createAttributeName(str);
        Attr* e = self->getNamedItem(attrName);
        if (e != nullptr) {
            return e->scriptValue();
        }
    } else if (idx < self->length()) {
        return self->item(idx)->scriptValue();
    }
    return ESValue(ESValue::ESDeletedValue);
}

static bool namedNodeMapWriteCallbackFunction(const ESValue& key,
                                              const ESValue& val, ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    return false;
}

static ESValueVector namedNodeMapEnumerateCallbackFunction(ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    NamedNodeMap* self = (NamedNodeMap*)obj->extraPointerData();
    STARFISH_ASSERT(self->isNamedNodeMap());
    size_t len = self->length();
    ESValueVector v(len);
    for (size_t i = 0; i < len; i++) {
        v[i] = ESValue(i);
    }
    return v;
}

void ScriptWrappable::initScriptWrappable(NamedNodeMap* ptr,
                                          ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnNamedNodeMap()->protoType());

    scriptObject()->setPropertyInterceptor(
        namedNodeMapReadCallbackFunction, namedNodeMapWriteCallbackFunction,
        namedNodeMapEnumerateCallbackFunction, true);
}

void ScriptWrappable::initScriptWrappable(Attr* ptr,
                                          ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->fnAttr()->protoType());
}

static ESValue cssStyleDeclarationReadCallbackFunction(const ESValue& key,
                                                       ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    CSSStyleDeclaration* self = (CSSStyleDeclaration*)obj->extraPointerData();
    STARFISH_ASSERT(self->isCSSStyleDeclaration());
    uint32_t idx = key.toIndex();
    if (idx < self->length()) {
        return ESString::create(self->item(idx)->utf8Data());
    }

    if (idx == ESValue::ESInvalidIndexValue) {
        const char* str = toBrowserString(key)->utf8Data();
        CSSStyleKind kind = lookupCSSStyleCamelCase(str, strlen(str));

        if (kind == CSSStyleKind::Unknown) {
            kind = lookupCSSStyle(str, strlen(str));
        }
        if (kind == CSSStyleKind::Unknown) {
            return ESValue(ESValue::ESDeletedValue);
        } else {
            if (false) {
            }
#define GET_ATTR(name, nameLower, nameCSSCase)   \
    else if (kind == CSSStyleKind::name)         \
    {                                            \
        return createScriptString(self->name()); \
    }
            FOR_EACH_STYLE_ATTRIBUTE_TOTAL(GET_ATTR)
#undef GET_ATTR
        }
    }

    return ESString::create("");
}

static bool cssStyleDeclarationWriteCallbackFunction(const ESValue& key,
                                                     const ESValue& val,
                                                     ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    CSSStyleDeclaration* self = (CSSStyleDeclaration*)obj->extraPointerData();
    STARFISH_ASSERT(self->isCSSStyleDeclaration());
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
#define SET_ATTR(name, nameLower, nameCSSCase)        \
    else if (kind == CSSStyleKind::name)              \
    {                                                 \
        self->set##name(toBrowserString(val), false); \
        return true;                                  \
    }
        FOR_EACH_STYLE_ATTRIBUTE_TOTAL(SET_ATTR)
#undef SET_ATTR
    }

    return false;
}

static ESValueVector cssStyleDeclarationEnumerateCallbackFunction(ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    CSSStyleDeclaration* self = (CSSStyleDeclaration*)obj->extraPointerData();
    STARFISH_ASSERT(self->isCSSStyleDeclaration());
    size_t len = self->length();
    ESValueVector v(len);
    for (size_t i = 0; i < len; i++) {
        v[i] = ESValue(i);
    }

#define ENUM_ATTR(name, nameLower, nameCSSCase) \
    v.push_back(ESString::create(#nameLower));

    FOR_EACH_STYLE_ATTRIBUTE_TOTAL(ENUM_ATTR)
#undef ENUM_ATTR
    return v;
}

void ScriptWrappable::initScriptWrappable(CSSStyleDeclaration* ptr)
{
    auto data = fetchData(ptr->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnCSSStyleDeclaration()->protoType());

    scriptObject()->setPropertyInterceptor(
        cssStyleDeclarationReadCallbackFunction,
        cssStyleDeclarationWriteCallbackFunction,
        cssStyleDeclarationEnumerateCallbackFunction, true);
}

void ScriptWrappable::initScriptWrappable(CSSStyleRule* ptr)
{
    auto data = fetchData(ptr->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->fnCSSStyleRule()->protoType());
}
#ifdef STARFISH_ENABLE_TEST
void Window::testStart()
{
    ESValue v = ESVMInstance::currentInstance()->globalObject()->get(
        ESString::create("testStart"));
    if (!v.isUndefined()) {
        callScriptFunction(v, {}, 0,
                           ESVMInstance::currentInstance()->globalObject());
    }
}
#endif

static int utf32ToUtf16(char32_t i, char16_t* u)
{
    if (i < 0xffff) {
        *u = (char16_t)(i & 0xffff);
        return 1;
    } else if (i < 0x10ffff) {
        i -= 0x10000;
        *u++ = 0xd800 | (i >> 10);
        *u = 0xdc00 | (i & 0x3ff);
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
        escargot::ASCIIString s(str->asASCIIString()->begin(),
                                str->asASCIIString()->end());
        return ESString::create(std::move(s));
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

        return ESString::create(std::move(out));
    }
}

ScriptValue createScriptFunction(String** argNames, size_t argc,
                                 String* functionBody, bool& error)
{
    error = false;
    ESVMInstance* instance = ESVMInstance::currentInstance();

    ESValueVector arg(0);
    for (size_t i = 0; i < argc; i++) {
        arg.push_back(createScriptString(argNames[i]));
    }

    arg.push_back(createScriptString(functionBody));

    ScriptValue result;
    std::jmp_buf tryPosition;
    if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
        result = ESFunctionObject::call(instance,
                                        instance->globalObject()->function(),
                                        ESValue(), arg.data(), argc + 1, false);
        instance->unregisterTryPos(&tryPosition);
    } else {
        result = instance->getCatchedError();
        error = true;
        STARFISH_LOG_INFO("Uncaught %s\n", result.toString()->utf8Data());
    }
    return result;
}

struct AttributeStringEventFunctionInnerData : public gc {
    ESValue function;
    Element* m_target;
};

static ESValue globalObjectReadCallbackFunction(const ESValue& key,
                                                ESObject* obj)
{
    Window* wnd = (Window*)ESVMInstance::currentInstance()
                      ->globalObject()
                      ->extraPointerData();
    Element* e =
        wnd->document()
            ->elementExecutionStackForAttributeStringEventFunctionObject()
            .back();
    if (e->scriptValue().asESPointer()->asESObject()->hasOwnProperty(key,
                                                                     true)) {
        return e->scriptValue().asESPointer()->asESObject()->getOwnProperty(
            key);
    }
    return ESValue(ESValue::ESDeletedValue);
}

static ESValue attributeStringEventFunction(ESVMInstance* instance)
{
    FunctionEnvironmentRecordWithArgumentsObject* record =
        (FunctionEnvironmentRecordWithArgumentsObject*)
            ESVMInstance::currentInstance()
                ->currentExecutionContext()
                ->environment()
                ->record();
    ESFunctionObject* callee = record->callee();
    STARFISH_ASSERT(callee->extraData() == kEventStringAttributeCheckMagic);
    AttributeStringEventFunctionInnerData* data =
        (AttributeStringEventFunctionInnerData*)callee->extraPointerData();

    Window* wnd = (Window*)ESVMInstance::currentInstance()
                      ->globalObject()
                      ->extraPointerData();
    wnd->document()
        ->elementExecutionStackForAttributeStringEventFunctionObject()
        .push_back(data->m_target);

    ESVMInstance::currentInstance()->globalObject()->setIdentifierInterceptor(
        globalObjectReadCallbackFunction);

    std::jmp_buf tryPosition;
    bool hasError = false;
    ESValue result;
    if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
        result = ESFunctionObject::call(ESVMInstance::currentInstance(),
                                        data->function,
                                        ESVMInstance::currentInstance()
                                            ->currentExecutionContext()
                                            ->resolveThisBinding(),
                                        ESVMInstance::currentInstance()
                                            ->currentExecutionContext()
                                            ->arguments(),
                                        ESVMInstance::currentInstance()
                                            ->currentExecutionContext()
                                            ->argumentCount(),
                                        false);
        instance->unregisterTryPos(&tryPosition);
        hasError = false;
    } else {
        hasError = true;
        result = instance->getCatchedError();
    }

    wnd->document()
        ->elementExecutionStackForAttributeStringEventFunctionObject()
        .pop_back();

    if (wnd->document()
            ->elementExecutionStackForAttributeStringEventFunctionObject()
            .size() == 0) {
        ESVMInstance::currentInstance()
            ->globalObject()
            ->setIdentifierInterceptor(nullptr);
    }

    if (hasError) {
        instance->throwError(result);
    }

    return result;
}

ScriptValue createAttributeStringEventFunction(Element* target,
                                               String* functionBody,
                                               bool& result)
{
    String* name[] = { String::createASCIIString("event") };
    ESValue fn = createScriptFunction(name, 1, functionBody, result);
    ESFunctionObject* wrapper = ESFunctionObject::create(
        NULL, attributeStringEventFunction, ESString::create(""), 0, false);

    wrapper->codeBlock()->m_needsToPrepareGenerateArgumentsObject = true;
    wrapper->setExtraData(kEventStringAttributeCheckMagic);
    AttributeStringEventFunctionInnerData* data =
        new AttributeStringEventFunctionInnerData();
    data->m_target = target;
    data->function = fn;
    wrapper->setExtraPointerData(data);
    return wrapper;
}

ScriptValue callScriptFunction(ScriptValue fn, ScriptValue* argv, size_t argc,
                               ScriptValue thisValue)
{
    ScriptValue result;
    if (fn.isESPointer() && fn.asESPointer()->isESFunctionObject()) {
        ESVMInstance* instance = ESVMInstance::currentInstance();
        std::jmp_buf tryPosition;
        if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
            result = ESFunctionObject::call(instance, fn, thisValue, argv, argc,
                                            false);
            instance->unregisterTryPos(&tryPosition);
        } else {
            result = instance->getCatchedError();
            STARFISH_LOG_INFO("Uncaught %s\n", result.toString()->utf8Data());
        }
    }
    return result;
}

ScriptValue createArrayBuffer(void* bufferSrc, size_t len)
{
#ifdef USE_ES6_FEATURE
    ESArrayBufferObject* obj = ESArrayBufferObject::create();
    obj->attachArrayBuffer(bufferSrc, len);
    return obj;
#else
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
#endif
}

ScriptValue parseJSON(String* jsonData)
{
    ScriptValue ret;
    ESVMInstance* instance = ESVMInstance::currentInstance();
    ScriptValue json_arg[1] = { ScriptValue(createScriptString(jsonData)) };
    ScriptValue json_parse_fn = instance->globalObject()->json()->get(
        ScriptValue(createScriptString(String::fromUTF8("parse"))));
    return callScriptFunction(json_parse_fn, json_arg, 1,
                              instance->globalObject()->json());
}

String* jsonStringify(ESValue value)
{
    ESVMInstance* instance = ESVMInstance::currentInstance();
    ScriptValue json_parse_fn = instance->globalObject()->json()->get(
        ScriptValue(createScriptString(String::fromUTF8("stringify"))));
    return toBrowserString(callScriptFunction(
        json_parse_fn, &value, 1, instance->globalObject()->json()));
}

bool isCallableScriptValue(ScriptValue v)
{
    if (v.isESPointer() && v.asESPointer()->isESFunctionObject()) {
        return true;
    }
    return false;
}

#ifdef USE_ES6_FEATURE
Promise::Promise()
    : m_scriptValue(ESPromiseObject::create())
{
    // TODO remove below line if escargot fixed
    m_scriptValue.asESPointer()->asESPromiseObject()->set__proto__(
        ESVMInstanceCurrentInstance()->globalObject()->promisePrototype());
}

void Promise::fulfill(ScriptValue v)
{
    m_scriptValue.asESPointer()->asESPromiseObject()->fulfillPromise(
        ESVMInstanceCurrentInstance(), v);
}

void Promise::reject(ScriptValue v)
{
    m_scriptValue.asESPointer()->asESPromiseObject()->rejectPromise(
        ESVMInstanceCurrentInstance(), v);
}
#endif
}
