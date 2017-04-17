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
#if defined(STARFISH_ENABLE_MULTIMEDIA)
#include "StarFishConfig.h"
#include "ScriptBindingInstance.h"

#include "Binding.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

namespace StarFish {

using namespace escargot;

ESValue onprogressHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_progress;

    String* result = originalObj->getAttribute(attr);

    return toJSString(result);
}

ESValue onprogressHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_progress;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onsuspendHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_suspend;

    String* result = originalObj->getAttribute(attr);

    return toJSString(result);
}

ESValue onsuspendHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_suspend;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onabortHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_abort;

    String* result = originalObj->getAttribute(attr);

    return toJSString(result);
}

ESValue onabortHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_abort;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onemptiedHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_emptied;

    String* result = originalObj->getAttribute(attr);

    return toJSString(result);
}

ESValue onemptiedHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_emptied;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onstalledHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_stalled;

    String* result = originalObj->getAttribute(attr);

    return toJSString(result);
}

ESValue onstalledHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_stalled;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onloadedmetadataHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_loadedmetadata;

    String* result = originalObj->getAttribute(attr);

    return toJSString(result);
}

ESValue onloadedmetadataHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_loadedmetadata;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onloadeddataHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_loadeddata;

    String* result = originalObj->getAttribute(attr);

    return toJSString(result);
}

ESValue onloadeddataHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_loadeddata;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onloadstartHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_loadstart;

    String* result = originalObj->getAttribute(attr);

    return toJSString(result);
}

ESValue onloadstartHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_loadstart;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue oncanplayHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_canplay;

    String* result = originalObj->getAttribute(attr);

    return toJSString(result);
}

ESValue oncanplayHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_canplay;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue oncanplaythroughHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_canplaythrough;

    String* result = originalObj->getAttribute(attr);

    return toJSString(result);
}

ESValue oncanplaythroughHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_canplaythrough;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onplayingHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_playing;

    String* result = originalObj->getAttribute(attr);

    return toJSString(result);
}

ESValue onplayingHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_playing;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onwaitingHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_waiting;

    String* result = originalObj->getAttribute(attr);

    return toJSString(result);
}

ESValue onwaitingHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_waiting;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onseekingHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_seeking;

    String* result = originalObj->getAttribute(attr);

    return toJSString(result);
}

ESValue onseekingHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_seeking;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onseekedHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_seeked;

    String* result = originalObj->getAttribute(attr);

    return toJSString(result);
}

ESValue onseekedHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_seeked;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onendedHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_ended;

    String* result = originalObj->getAttribute(attr);

    return toJSString(result);
}

ESValue onendedHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_ended;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue ondurationchangeHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_durationchange;

    String* result = originalObj->getAttribute(attr);

    return toJSString(result);
}

ESValue ondurationchangeHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_durationchange;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue ontimeupdateHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_timeupdate;

    String* result = originalObj->getAttribute(attr);

    return toJSString(result);
}

ESValue ontimeupdateHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_timeupdate;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onplayHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_play;

    String* result = originalObj->getAttribute(attr);

    return toJSString(result);
}

ESValue onplayHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_play;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onpauseHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_pause;

    String* result = originalObj->getAttribute(attr);

    return toJSString(result);
}

ESValue onpauseHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_pause;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onratechangeHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_ratechange;

    String* result = originalObj->getAttribute(attr);

    return toJSString(result);
}

ESValue onratechangeHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_ratechange;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onvolumechangeHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_volumechange;

    String* result = originalObj->getAttribute(attr);

    return toJSString(result);
}

ESValue onvolumechangeHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_volumechange;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}
}
#endif
