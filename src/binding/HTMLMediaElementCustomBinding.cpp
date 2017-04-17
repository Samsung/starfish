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

    return window->attributeEventListener(attr);
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

    return window->attributeEventListener(attr);
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

    return window->attributeEventListener(attr);
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

    return window->attributeEventListener(attr);
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

    return window->attributeEventListener(attr);
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

    return window->attributeEventListener(attr);
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

    return window->attributeEventListener(attr);
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

    return window->attributeEventListener(attr);
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

    return window->attributeEventListener(attr);
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

    return window->attributeEventListener(attr);
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

    return window->attributeEventListener(attr);
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

    return window->attributeEventListener(attr);
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

    return window->attributeEventListener(attr);
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

    return window->attributeEventListener(attr);
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

    return window->attributeEventListener(attr);
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

    return window->attributeEventListener(attr);
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

    return window->attributeEventListener(attr);
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

    return window->attributeEventListener(attr);
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

    return window->attributeEventListener(attr);
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

    return window->attributeEventListener(attr);
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

    return window->attributeEventListener(attr);
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
