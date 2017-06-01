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
#include "core/dom/HTMLElement.h"
#include "core/style/CSSStyleDeclaration.h"

#include <EscargotPublic.h>
using namespace Escargot;

namespace StarFish {

ValueRef* styleHTMLElementGetterFunction(ExecutionStateRef* state,
                                         ValueRef* thisValue, size_t argc,
                                         ValueRef** argv, bool isNewExpression)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    CSSStyleDeclaration* result = originalObj->inlineStyle();
    return result->scriptValue();
}

ValueRef* styleHTMLElementSetterFunction(ExecutionStateRef* state,
                                         ValueRef* thisValue, size_t argc,
                                         ValueRef** argv, bool isNewExpression)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);

    ValueRef* arg0 = argv[0];
    String* value0 = toBrowserString(state, arg0);

    originalObj->setStyleAttr(value0);
    return scriptUndefined();
}
}
