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
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "dom/DOMException.h"
#include "dom/HTMLMediaElement.h"

namespace StarFish {

using namespace escargot;

ESValue onprogressHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onprogressEventListener();
}

ESValue onprogressHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnprogressEventListener(arg0);

    return ESValue();
}

ESValue onsuspendHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onsuspendEventListener();
}

ESValue onsuspendHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnsuspendEventListener(arg0);

    return ESValue();
}

ESValue onabortHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onabortEventListener();
}

ESValue onabortHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnabortEventListener(arg0);

    return ESValue();
}

ESValue onemptiedHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onemptiedEventListener();
}

ESValue onemptiedHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnemptiedEventListener(arg0);

    return ESValue();
}

ESValue onstalledHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onstalledEventListener();
}

ESValue onstalledHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnstalledEventListener(arg0);

    return ESValue();
}

ESValue onloadedmetadataHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onloadedmetadataEventListener();
}

ESValue onloadedmetadataHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnloadedmetadataEventListener(arg0);

    return ESValue();
}

ESValue onloadeddataHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onloadeddataEventListener();
}

ESValue onloadeddataHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnloadeddataEventListener(arg0);

    return ESValue();
}

ESValue onloadstartHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onloadstartEventListener();
}

ESValue onloadstartHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnloadstartEventListener(arg0);

    return ESValue();
}

ESValue oncanplayHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->oncanplayEventListener();
}

ESValue oncanplayHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOncanplayEventListener(arg0);

    return ESValue();
}

ESValue oncanplaythroughHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->oncanplayEventListener();
}

ESValue oncanplaythroughHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOncanplaythroughEventListener(arg0);

    return ESValue();
}

ESValue onplayingHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onplayingEventListener();
}

ESValue onplayingHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnplayingEventListener(arg0);

    return ESValue();
}

ESValue onwaitingHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onwaitingEventListener();
}

ESValue onwaitingHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnwaitingEventListener(arg0);

    return ESValue();
}

ESValue onseekingHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onseekingEventListener();
}

ESValue onseekingHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnseekingEventListener(arg0);

    return ESValue();
}

ESValue onseekedHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onseekedEventListener();
}

ESValue onseekedHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnseekedEventListener(arg0);

    return ESValue();
}

ESValue onendedHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onendedEventListener();
}

ESValue onendedHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnendedEventListener(arg0);

    return ESValue();
}

ESValue ondurationchangeHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->ondurationchangeEventListener();
}

ESValue ondurationchangeHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOndurationchangeEventListener(arg0);

    return ESValue();
}

ESValue ontimeupdateHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->ontimeupdateEventListener();
}

ESValue ontimeupdateHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOntimeupdateEventListener(arg0);

    return ESValue();
}

ESValue onplayHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onplayEventListener();
}

ESValue onplayHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnplayEventListener(arg0);

    return ESValue();
}

ESValue onpauseHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onpauseEventListener();
}

ESValue onpauseHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnpauseEventListener(arg0);

    return ESValue();
}

ESValue onratechangeHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onratechangeEventListener();
}

ESValue onratechangeHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnratechangeEventListener(arg0);

    return ESValue();
}

ESValue onvolumechangeHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onvolumechangeEventListener();
}

ESValue onvolumechangeHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnvolumechangeEventListener(arg0);

    return ESValue();
}
}
#endif
