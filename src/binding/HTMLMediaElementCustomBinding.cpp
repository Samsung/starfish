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

    return originalObj->onprogress();
}

ESValue onprogressHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnprogress(arg0);

    return ESValue();
}

ESValue onsuspendHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onsuspend();
}

ESValue onsuspendHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnsuspend(arg0);

    return ESValue();
}

ESValue onabortHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onabort();
}

ESValue onabortHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnabort(arg0);

    return ESValue();
}

ESValue onemptiedHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onemptied();
}

ESValue onemptiedHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnemptied(arg0);

    return ESValue();
}

ESValue onstalledHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onstalled();
}

ESValue onstalledHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnstalled(arg0);

    return ESValue();
}

ESValue onloadedmetadataHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onloadedmetadata();
}

ESValue onloadedmetadataHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnloadedmetadata(arg0);

    return ESValue();
}

ESValue onloadeddataHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onloadeddata();
}

ESValue onloadeddataHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnloadeddata(arg0);

    return ESValue();
}

ESValue onloadstartHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onloadstart();
}

ESValue onloadstartHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnloadstart(arg0);

    return ESValue();
}

ESValue oncanplayHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->oncanplay();
}

ESValue oncanplayHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOncanplay(arg0);

    return ESValue();
}

ESValue oncanplaythroughHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->oncanplay();
}

ESValue oncanplaythroughHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOncanplaythrough(arg0);

    return ESValue();
}

ESValue onplayingHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onplaying();
}

ESValue onplayingHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnplaying(arg0);

    return ESValue();
}

ESValue onwaitingHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onwaiting();
}

ESValue onwaitingHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnwaiting(arg0);

    return ESValue();
}

ESValue onseekingHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onseeking();
}

ESValue onseekingHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnseeking(arg0);

    return ESValue();
}

ESValue onseekedHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onseeked();
}

ESValue onseekedHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnseeked(arg0);

    return ESValue();
}

ESValue onendedHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onended();
}

ESValue onendedHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnended(arg0);

    return ESValue();
}

ESValue ondurationchangeHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->ondurationchange();
}

ESValue ondurationchangeHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOndurationchange(arg0);

    return ESValue();
}

ESValue ontimeupdateHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->ontimeupdate();
}

ESValue ontimeupdateHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOntimeupdate(arg0);

    return ESValue();
}

ESValue onplayHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onplay();
}

ESValue onplayHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnplay(arg0);

    return ESValue();
}

ESValue onpauseHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onpause();
}

ESValue onpauseHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnpause(arg0);

    return ESValue();
}

ESValue onratechangeHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onratechange();
}

ESValue onratechangeHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnratechange(arg0);

    return ESValue();
}

ESValue onvolumechangeHTMLMediaElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    return originalObj->onvolumechange();
}

ESValue onvolumechangeHTMLMediaElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLMediaElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnvolumechange(arg0);

    return ESValue();
}
}
#endif
