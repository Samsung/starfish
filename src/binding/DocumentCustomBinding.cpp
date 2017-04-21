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

#include "dom/Document.h"
#include "dom/DOMException.h"
#include "dom/HTMLBodyElement.h"
#include "dom/HTMLHeadElement.h"
#include "dom/HTMLHtmlElement.h"
#include "extra/Location.h"
#include "platform/window/Window.h"

namespace StarFish {

using namespace escargot;

ESValue bodyDocumentSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    GENERATE_ARG_AND_CHECK_TYPE(0, HTMLElement);

    try {
        originalObj->setBody(val0);
        return ESValue();
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
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
