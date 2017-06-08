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

#include "core/dom/HTMLElement.h"
#include "core/style/CSSStyleDeclaration.h"

namespace StarFish {

using namespace escargot;

ESValue styleHTMLElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    CSSStyleDeclaration* result = originalObj->inlineStyle();
    return result->scriptValue();
}

ESValue styleHTMLElementSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    String* value0 = toBrowserString(arg0);

    originalObj->setStyleAttr(value0);
    return ESValue();
}
}
