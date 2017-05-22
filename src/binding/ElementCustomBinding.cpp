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

#include "core/dom/DOMTokenList.h"
#include "core/dom/Element.h"

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

ESValue classListElementSetterFunction(ESVMInstance* instance)
{
    /* GENERATE_THIS_AND_CHECK_TYPE(Element);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    // Declare native value (empty when type is void)
    DOMTokenList* forwards = nullptr;
    forwards = originalObj->classList();
    if (forwards) {
        forwards->setValue(value0);
    }
    return ESValue(); */
    STARFISH_ASSERT_NOT_REACHED();
}
}
