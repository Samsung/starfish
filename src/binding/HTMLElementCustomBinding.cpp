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

#include "StarFishConfig.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "dom/DOMException.h"
#include "dom/HTMLElement.h"

namespace StarFish {

using namespace escargot;

ESValue onclickHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    return originalObj->onclickEventListener();
}

ESValue onclickHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnclickEventListener(arg0);

    return ESValue();
}

ESValue onmouseoverHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    return originalObj->onmouseoverEventListener();
}

ESValue onmouseoverHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnmouseoverEventListener(arg0);

    return ESValue();
}

ESValue onloadHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    return originalObj->onloadEventListener();
}

ESValue onloadHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnloadEventListener(arg0);

    return ESValue();
}

ESValue onunloadHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    return originalObj->onunloadEventListener();
}

ESValue onunloadHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnunloadEventListener(arg0);

    return ESValue();
}

ESValue onkeydownHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    return originalObj->onkeydownEventListener();
}

ESValue onkeydownHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnkeydownEventListener(arg0);

    return ESValue();
}

ESValue onkeyupHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    return originalObj->onkeyupEventListener();
}

ESValue onkeyupHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnkeyupEventListener(arg0);

    return ESValue();
}

ESValue onfocusHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    return originalObj->onfocusEventListener();
}

ESValue onfocusHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnfocusEventListener(arg0);

    return ESValue();
}

ESValue onerrorHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    return originalObj->onerrorEventListener();
}

ESValue onerrorHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnerrorEventListener(arg0);

    return ESValue();
}
}
