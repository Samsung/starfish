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

#include "extra/History.h"
#include "extra/Location.h"
#include "extra/Navigator.h"
#include "dom/Document.h"
#include "dom/Element.h"
#include "dom/CSSStyleDeclaration.h"
#include "platform/window/Window.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
extern ESValue windowWindowGetterFunction(ESVMInstance* instance);

static ESValue documentGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    Document* result = nullptr;
    result = window->document();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

#ifdef STARFISH_ENABLE_MULTI_PAGE
static ESValue locationGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    Location* result = nullptr;
    result = window->location();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue locationSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    // Declare native value (empty when type is void)
    Location* forwards = nullptr;
    forwards = window->location();
    if (forwards) {
        forwards->setHref(value0);
    }
    return ESValue();
}
#endif

static ESValue historyGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    History* result = nullptr;
    result = window->history();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue navigatorGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    Navigator* result = nullptr;
    result = window->navigator();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue innerWidthGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    int32_t result;
    result = window->innerWidth();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue innerHeightGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    int32_t result;
    result = window->innerHeight();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue scrollXGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    double result;
    result = window->scrollX();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue scrollYGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    double result;
    result = window->scrollY();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue onabortGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onabort();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onabortSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnabort(value0);
    return ESValue();
}

static ESValue oncanplayGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->oncanplay();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue oncanplaySetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOncanplay(value0);
    return ESValue();
}

static ESValue oncanplaythroughGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->oncanplaythrough();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue oncanplaythroughSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOncanplaythrough(value0);
    return ESValue();
}

static ESValue onclickGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onclick();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onclickSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnclick(value0);
    return ESValue();
}

static ESValue ondurationchangeGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->ondurationchange();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue ondurationchangeSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOndurationchange(value0);
    return ESValue();
}

static ESValue onemptiedGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onemptied();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onemptiedSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnemptied(value0);
    return ESValue();
}

static ESValue onendedGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onended();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onendedSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnended(value0);
    return ESValue();
}

static ESValue onerrorGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onerror();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onerrorSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnerror(value0);
    return ESValue();
}

static ESValue onfocusGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onfocus();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onfocusSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnfocus(value0);
    return ESValue();
}

static ESValue onkeydownGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onkeydown();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onkeydownSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnkeydown(value0);
    return ESValue();
}

static ESValue onkeyupGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onkeyup();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onkeyupSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnkeyup(value0);
    return ESValue();
}

static ESValue onloadGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onload();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onloadSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnload(value0);
    return ESValue();
}

static ESValue onloadeddataGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onloadeddata();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onloadeddataSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnloadeddata(value0);
    return ESValue();
}

static ESValue onloadedmetadataGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onloadedmetadata();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onloadedmetadataSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnloadedmetadata(value0);
    return ESValue();
}

static ESValue onloadstartGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onloadstart();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onloadstartSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnloadstart(value0);
    return ESValue();
}

static ESValue onmouseoverGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onmouseover();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onmouseoverSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnmouseover(value0);
    return ESValue();
}

static ESValue onpauseGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onpause();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onpauseSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnpause(value0);
    return ESValue();
}

static ESValue onplayGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onplay();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onplaySetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnplay(value0);
    return ESValue();
}

static ESValue onplayingGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onplaying();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onplayingSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnplaying(value0);
    return ESValue();
}

static ESValue onprogressGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onprogress();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onprogressSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnprogress(value0);
    return ESValue();
}

static ESValue onratechangeGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onratechange();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onratechangeSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnratechange(value0);
    return ESValue();
}

static ESValue onseekedGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onseeked();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onseekedSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnseeked(value0);
    return ESValue();
}

static ESValue onseekingGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onseeking();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onseekingSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnseeking(value0);
    return ESValue();
}

static ESValue onstalledGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onstalled();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onstalledSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnstalled(value0);
    return ESValue();
}

static ESValue onsuspendGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onsuspend();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onsuspendSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnsuspend(value0);
    return ESValue();
}

static ESValue ontimeupdateGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->ontimeupdate();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue ontimeupdateSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOntimeupdate(value0);
    return ESValue();
}

static ESValue onvolumechangeGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onvolumechange();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onvolumechangeSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnvolumechange(value0);
    return ESValue();
}

static ESValue onwaitingGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onwaiting();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onwaitingSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnwaiting(value0);
    return ESValue();
}

static ESValue onunloadGetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = window->onunload();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onunloadSetterFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    window->setOnunload(value0);
    return ESValue();
}

// Implement for functions
extern ESValue requestAnimationFrameWindowFunction(ESVMInstance* instance);

static ESValue cancelAnimationFrameFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "cancelAnimationFrame",
                        "Window", reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    uint32_t value0;
    value0 = arg0.toUint32();
    // Call native function (nargs: 1)
    window->cancelAnimationFrame(value0);

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue getComputedStyleFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "getComputedStyle", "Window",
                        reason);
        THROW_EXCEPTION(msg);
    }
    size_t validArgCount = 2;
    // Declare native value (empty when type is void)
    CSSStyleDeclaration* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    // Handle argument arg1
    String* value1 = String::emptyString;
    if (arg1.isUndefined()) {
        validArgCount--;
    } else {
        value1 = toBrowserString(arg1);
    }
    // Handle argument arg0
    Element* value0 = nullptr;
    CHECK_TYPEOF(arg0, Element);
    value0 = (Element*)(arg0.asESPointer()->asESObject()->extraPointerData());
    // Call native function (nargs: 1-2)
    if (validArgCount == 1) {
        result = window->getComputedStyle(value0);
    } else if (validArgCount == 2) {
        result = window->getComputedStyle(value0, value1);
    }

    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

extern ESValue setTimeoutWindowFunction(ESVMInstance* instance);

static ESValue clearTimeoutFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    int32_t value0 = 0;
    if (!arg0.isUndefinedOrNull()) {
        value0 = arg0.toInt32();
    }
    // Call native function (nargs: 1)
    window->clearTimeout(value0);

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

extern ESValue setIntervalWindowFunction(ESVMInstance* instance);

static ESValue clearIntervalFunction(ESVMInstance* instance)
{
    GENERATE_WINDOW();
    // Declare native value (empty when type is void)
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    int32_t value0 = 0;
    if (!arg0.isUndefinedOrNull()) {
        value0 = arg0.toInt32();
    }
    // Call native function (nargs: 1)
    window->clearInterval(value0);

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

ESFunctionObject* bindingWindow(ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* WindowString = ESString::create("Window");
    ESFunctionObject* WindowFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, WindowString, 0, true, true);
    ESObject* WindowPrototypeObj =
        WindowFunction->protoType().asESPointer()->asESObject();
    WindowFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    WindowPrototypeObj->forceNonVectorHiddenClass(false);
    WindowPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnEventTarget()->protoType());
    WindowFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnEventTarget());

    return WindowFunction;
}

void Window::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(fetchData(instance)->fnWindow()->protoType());
    // Bind for attributes
    ESString* windowString = ESString::create("window");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), windowString, windowWindowGetterFunction, nullptr,
        true /* enumerable */, false /* configurable */);

    ESString* documentString = ESString::create("document");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), documentString, documentGetterFunction, nullptr,
        true /* enumerable */, false /* configurable */);

#ifdef STARFISH_ENABLE_MULTI_PAGE
    ESString* locationString = ESString::create("location");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), locationString, locationGetterFunction,
        locationSetterFunction, true /* enumerable */,
        false /* configurable */);
#endif

    ESString* historyString = ESString::create("history");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), historyString, historyGetterFunction, nullptr,
        true /* enumerable */, true /* configurable */);

    ESString* navigatorString = ESString::create("navigator");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), navigatorString, navigatorGetterFunction, nullptr,
        true /* enumerable */, true /* configurable */);

    ESString* innerWidthString = ESString::create("innerWidth");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), innerWidthString, innerWidthGetterFunction, nullptr,
        true /* enumerable */, true /* configurable */);

    ESString* innerHeightString = ESString::create("innerHeight");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), innerHeightString, innerHeightGetterFunction, nullptr,
        true /* enumerable */, true /* configurable */);

    ESString* scrollXString = ESString::create("scrollX");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), scrollXString, scrollXGetterFunction, nullptr,
        true /* enumerable */, true /* configurable */);

    ESString* scrollYString = ESString::create("scrollY");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), scrollYString, scrollYGetterFunction, nullptr,
        true /* enumerable */, true /* configurable */);

    ESString* onabortString = ESString::create("onabort");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onabortString, onabortGetterFunction,
        onabortSetterFunction, true /* enumerable */, true /* configurable */);

    ESString* oncanplayString = ESString::create("oncanplay");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), oncanplayString, oncanplayGetterFunction,
        oncanplaySetterFunction, true /* enumerable */,
        true /* configurable */);

    ESString* oncanplaythroughString = ESString::create("oncanplaythrough");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), oncanplaythroughString, oncanplaythroughGetterFunction,
        oncanplaythroughSetterFunction, true /* enumerable */,
        true /* configurable */);

    ESString* onclickString = ESString::create("onclick");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onclickString, onclickGetterFunction,
        onclickSetterFunction, true /* enumerable */, true /* configurable */);

    ESString* ondurationchangeString = ESString::create("ondurationchange");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), ondurationchangeString, ondurationchangeGetterFunction,
        ondurationchangeSetterFunction, true /* enumerable */,
        true /* configurable */);

    ESString* onemptiedString = ESString::create("onemptied");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onemptiedString, onemptiedGetterFunction,
        onemptiedSetterFunction, true /* enumerable */,
        true /* configurable */);

    ESString* onendedString = ESString::create("onended");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onendedString, onendedGetterFunction,
        onendedSetterFunction, true /* enumerable */, true /* configurable */);

    ESString* onerrorString = ESString::create("onerror");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onerrorString, onerrorGetterFunction,
        onerrorSetterFunction, true /* enumerable */, true /* configurable */);

    ESString* onfocusString = ESString::create("onfocus");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onfocusString, onfocusGetterFunction,
        onfocusSetterFunction, true /* enumerable */, true /* configurable */);

    ESString* onkeydownString = ESString::create("onkeydown");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onkeydownString, onkeydownGetterFunction,
        onkeydownSetterFunction, true /* enumerable */,
        true /* configurable */);

    ESString* onkeyupString = ESString::create("onkeyup");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onkeyupString, onkeyupGetterFunction,
        onkeyupSetterFunction, true /* enumerable */, true /* configurable */);

    ESString* onloadString = ESString::create("onload");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onloadString, onloadGetterFunction,
        onloadSetterFunction, true /* enumerable */, true /* configurable */);

    ESString* onloadeddataString = ESString::create("onloadeddata");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onloadeddataString, onloadeddataGetterFunction,
        onloadeddataSetterFunction, true /* enumerable */,
        true /* configurable */);

    ESString* onloadedmetadataString = ESString::create("onloadedmetadata");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onloadedmetadataString, onloadedmetadataGetterFunction,
        onloadedmetadataSetterFunction, true /* enumerable */,
        true /* configurable */);

    ESString* onloadstartString = ESString::create("onloadstart");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onloadstartString, onloadstartGetterFunction,
        onloadstartSetterFunction, true /* enumerable */,
        true /* configurable */);

    ESString* onmouseoverString = ESString::create("onmouseover");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onmouseoverString, onmouseoverGetterFunction,
        onmouseoverSetterFunction, true /* enumerable */,
        true /* configurable */);

    ESString* onpauseString = ESString::create("onpause");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onpauseString, onpauseGetterFunction,
        onpauseSetterFunction, true /* enumerable */, true /* configurable */);

    ESString* onplayString = ESString::create("onplay");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onplayString, onplayGetterFunction,
        onplaySetterFunction, true /* enumerable */, true /* configurable */);

    ESString* onplayingString = ESString::create("onplaying");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onplayingString, onplayingGetterFunction,
        onplayingSetterFunction, true /* enumerable */,
        true /* configurable */);

    ESString* onprogressString = ESString::create("onprogress");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onprogressString, onprogressGetterFunction,
        onprogressSetterFunction, true /* enumerable */,
        true /* configurable */);

    ESString* onratechangeString = ESString::create("onratechange");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onratechangeString, onratechangeGetterFunction,
        onratechangeSetterFunction, true /* enumerable */,
        true /* configurable */);

    ESString* onseekedString = ESString::create("onseeked");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onseekedString, onseekedGetterFunction,
        onseekedSetterFunction, true /* enumerable */, true /* configurable */);

    ESString* onseekingString = ESString::create("onseeking");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onseekingString, onseekingGetterFunction,
        onseekingSetterFunction, true /* enumerable */,
        true /* configurable */);

    ESString* onstalledString = ESString::create("onstalled");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onstalledString, onstalledGetterFunction,
        onstalledSetterFunction, true /* enumerable */,
        true /* configurable */);

    ESString* onsuspendString = ESString::create("onsuspend");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onsuspendString, onsuspendGetterFunction,
        onsuspendSetterFunction, true /* enumerable */,
        true /* configurable */);

    ESString* ontimeupdateString = ESString::create("ontimeupdate");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), ontimeupdateString, ontimeupdateGetterFunction,
        ontimeupdateSetterFunction, true /* enumerable */,
        true /* configurable */);

    ESString* onvolumechangeString = ESString::create("onvolumechange");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onvolumechangeString, onvolumechangeGetterFunction,
        onvolumechangeSetterFunction, true /* enumerable */,
        true /* configurable */);

    ESString* onwaitingString = ESString::create("onwaiting");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onwaitingString, onwaitingGetterFunction,
        onwaitingSetterFunction, true /* enumerable */,
        true /* configurable */);

    ESString* onunloadString = ESString::create("onunload");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        scriptObject(), onunloadString, onunloadGetterFunction,
        onunloadSetterFunction, true /* enumerable */, true /* configurable */);

    // Bind for functions
    ESString* requestAnimationFrameString =
        ESString::create("requestAnimationFrame");
    ESFunctionObject* requestAnimationFrameWindowESFn =
        ESFunctionObject::create(nullptr, requestAnimationFrameWindowFunction,
                                 requestAnimationFrameString, 1, false);
    scriptObject()->defineDataProperty(
        requestAnimationFrameString, true /* writable */, true /* enumerable */,
        true /* configurable */, requestAnimationFrameWindowESFn);

    ESString* cancelAnimationFrameString =
        ESString::create("cancelAnimationFrame");
    ESFunctionObject* cancelAnimationFrameESFn =
        ESFunctionObject::create(nullptr, cancelAnimationFrameFunction,
                                 cancelAnimationFrameString, 1, false);
    scriptObject()->defineDataProperty(
        cancelAnimationFrameString, true /* writable */, true /* enumerable */,
        true /* configurable */, cancelAnimationFrameESFn);

    ESString* getComputedStyleString = ESString::create("getComputedStyle");
    ESFunctionObject* getComputedStyleESFn = ESFunctionObject::create(
        nullptr, getComputedStyleFunction, getComputedStyleString, 1, false);
    scriptObject()->defineDataProperty(
        getComputedStyleString, true /* writable */, true /* enumerable */,
        true /* configurable */, getComputedStyleESFn);

    ESString* setTimeoutString = ESString::create("setTimeout");
    ESFunctionObject* setTimeoutWindowESFn = ESFunctionObject::create(
        nullptr, setTimeoutWindowFunction, setTimeoutString, 2, false);
    scriptObject()->defineDataProperty(
        setTimeoutString, true /* writable */, true /* enumerable */,
        true /* configurable */, setTimeoutWindowESFn);

    ESString* clearTimeoutString = ESString::create("clearTimeout");
    ESFunctionObject* clearTimeoutESFn = ESFunctionObject::create(
        nullptr, clearTimeoutFunction, clearTimeoutString, 0, false);
    scriptObject()->defineDataProperty(
        clearTimeoutString, true /* writable */, true /* enumerable */,
        true /* configurable */, clearTimeoutESFn);

    ESString* setIntervalString = ESString::create("setInterval");
    ESFunctionObject* setIntervalWindowESFn = ESFunctionObject::create(
        nullptr, setIntervalWindowFunction, setIntervalString, 2, false);
    scriptObject()->defineDataProperty(
        setIntervalString, true /* writable */, true /* enumerable */,
        true /* configurable */, setIntervalWindowESFn);

    ESString* clearIntervalString = ESString::create("clearInterval");
    ESFunctionObject* clearIntervalESFn = ESFunctionObject::create(
        nullptr, clearIntervalFunction, clearIntervalString, 0, false);
    scriptObject()->defineDataProperty(
        clearIntervalString, true /* writable */, true /* enumerable */,
        true /* configurable */, clearIntervalESFn);

    postInit(instance);
}

bool Window::isWindow() const
{
    return true;
}
}
