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

    return originalObj->onclick();
}

ESValue onclickHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnclick(arg0);

    return ESValue();
}

ESValue onmouseoverHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    return originalObj->onmouseover();
}

ESValue onmouseoverHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnmouseover(arg0);

    return ESValue();
}

ESValue onloadHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    return originalObj->onload();
}

ESValue onloadHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnload(arg0);

    return ESValue();
}

ESValue onunloadHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    return originalObj->onunload();
}

ESValue onunloadHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnunload(arg0);

    return ESValue();
}

ESValue onkeydownHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    return originalObj->onkeydown();
}

ESValue onkeydownHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnkeydown(arg0);

    return ESValue();
}

ESValue onkeyupHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    return originalObj->onkeyup();
}

ESValue onkeyupHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnkeyup(arg0);

    return ESValue();
}

ESValue onfocusHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    return originalObj->onfocus();
}

ESValue onfocusHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnfocus(arg0);

    return ESValue();
}

ESValue onerrorHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    return originalObj->onerror();
}

ESValue onerrorHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnerror(arg0);

    return ESValue();
}
}
