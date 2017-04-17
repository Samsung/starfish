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
#include "ScriptBindingInstance.h"

#include "Binding.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

namespace StarFish {

using namespace escargot;

ESValue onclickHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_click;

    return originalObj->attributeEventListener(attr);
}

ESValue onclickHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_click;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onmouseoverHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_mouseover;

    return originalObj->attributeEventListener(attr);
}

ESValue onmouseoverHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_mouseover;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onloadHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_load;

    return originalObj->attributeEventListener(attr);
}

ESValue onloadHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_load;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onunloadHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_unload;

    return originalObj->attributeEventListener(attr);
}

ESValue onunloadHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_unload;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onkeydownHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_keydown;

    return originalObj->attributeEventListener(attr);
}

ESValue onkeydownHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_keydown;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onkeyupHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_keyup;

    return originalObj->attributeEventListener(attr);
}

ESValue onkeyupHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_keyup;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onfocusHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_focus;

    return originalObj->attributeEventListener(attr);
}

ESValue onfocusHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_keyup;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onerrorHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_error;

    return originalObj->attributeEventListener(attr);
}

ESValue onerrorHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_error;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        originalObj->setAttributeEventListener(attr, arg0);
    } else {
        originalObj->clearAttributeEventListener(attr);
    }

    return ESValue();
}
}
