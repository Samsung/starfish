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

ESValue namespaceURIElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    if (originalObj->name().namespaceURIAtomic() ==
        AtomicString::emptyAtomicString()) {
        return ESValue(ESValue::ESNull);
    }
    return toJSString(originalObj->name().namespaceURI());
}

ESValue idElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    return toJSString(originalObj->getAttribute(
        originalObj->document()->window()->starFish()->staticStrings()->m_id));
}

ESValue idElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    ESValue v = instance->currentExecutionContext()->readArgument(0);
    originalObj->setAttribute(
        originalObj->document()->window()->starFish()->staticStrings()->m_id,
        toBrowserString(v));

    return ESValue();
}

ESValue classNameElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    return toJSString(originalObj->getAttribute(originalObj->document()
                                                    ->window()
                                                    ->starFish()
                                                    ->staticStrings()
                                                    ->m_class));
}

ESValue classNameElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    ESValue v = instance->currentExecutionContext()->readArgument(0);
    originalObj->setAttribute(
        originalObj->document()->window()->starFish()->staticStrings()->m_class,
        toBrowserString(v));

    return ESValue();
}

ESValue classListElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    DOMTokenList* nd = originalObj->classList();
    if (nd == nullptr) {
        return ESValue(ESValue::ESUndefined);
    }
    return nd->scriptValue();
}
}
