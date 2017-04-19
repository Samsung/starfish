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
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "dom/DOMException.h"
#include "dom/HTMLBodyElement.h"
#include "dom/HTMLHeadElement.h"

namespace StarFish {

using namespace escargot;

ESValue bodyDocumentSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue v = instance->currentExecutionContext()->readArgument(0);
    CHECK_TYPEOF(v, HTMLElement);
    CHECK_TYPEOF_WITH_ERRCODE(v, HTMLBodyElement, instance,
                              DOMException::HIERARCHY_REQUEST_ERR);

    HTMLBodyElement* body = originalObj->body();
    HTMLHtmlElement* html = originalObj->rootElement();
    HTMLBodyElement* newBody =
        ((ScriptWrappable*)v.asESPointer()->asESObject()->extraPointerData())
            ->asHTMLBodyElement();

    if (body) {
        html->removeChild(body);
    }
    html->appendChild(newBody);

    return ESValue();
}

ESValue defaultViewDocumentGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    Window* window = originalObj->window();
    if (window != nullptr) {
        return window->scriptValue();
    }
    return ESValue(ESValue::ESNull);
}

ESValue locationDocumentSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue v = instance->currentExecutionContext()->readArgument(0);

    originalObj->location()->setHref(
        String::fromUTF8(v.toString()->utf8Data()));
    return ESValue();
}
}
