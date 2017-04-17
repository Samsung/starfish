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

ESValue onloadHTMLBodyElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLBodyElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_load;

    return window->attributeEventListener(attr);
}

ESValue onLoadHTMLBodyElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLBodyElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_load;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        window->setAttributeEventListener(attr, arg0);
    } else {
        window->clearAttributeEventListener(attr);
    }

    return ESValue();
}

ESValue onunloadHTMLBodyElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLBodyElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_unload;

    return window->attributeEventListener(attr);
}

ESValue onunloadHTMLBodyElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLBodyElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_unload;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    if (arg0.isObject()) {
        window->setAttributeEventListener(attr, arg0);
    } else {
        window->clearAttributeEventListener(attr);
    }

    return ESValue();
}
}
