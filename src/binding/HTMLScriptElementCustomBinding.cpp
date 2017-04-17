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

ESValue srcHTMLScriptElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLScriptElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_src;

    String* result = URL::getURLString(originalObj->document()->urlString(),
                                       originalObj->getAttribute(attr));

    return toJSString(result);
}

ESValue srcHTMLScriptElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLScriptElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_src;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    String* value0 = toBrowserString(arg0);

    originalObj->setAttribute(attr, value0);

    return ESValue();
}

ESValue typeHTMLScriptElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLScriptElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_type;

    String* result = originalObj->getAttribute(attr);

    return toJSString(result);
}

ESValue typeHTMLScriptElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLScriptElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_type;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    String* value0 = toBrowserString(arg0);

    originalObj->setAttribute(attr, value0);

    return ESValue();
}

ESValue charsetHTMLScriptElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLScriptElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_charset;

    String* result = originalObj->getAttribute(attr);

    return toJSString(result);
}

ESValue charsetHTMLScriptElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLScriptElement);

    Window* window = originalObj->document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_charset;

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    String* value0 = toBrowserString(arg0);

    originalObj->setAttribute(attr, value0);

    return ESValue();
}
}
