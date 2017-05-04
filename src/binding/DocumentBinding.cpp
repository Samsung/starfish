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

#include "dom/HTMLHeadElement.h"
#include "dom/Text.h"
#include "dom/DocumentFragment.h"
#include "extra/Location.h"
#include "dom/DOMException.h"
#include "dom/HTMLCollection.h"
#include "dom/Attr.h"
#include "dom/HTMLElement.h"
#include "dom/DocumentType.h"
#include "dom/Comment.h"
#include "dom/NodeList.h"
#include "dom/Element.h"
#include "dom/Document.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
#ifdef STARFISH_EXP
static ESValue implementationGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    DOMImplementation* result = nullptr;
    result = originalObj->implementation();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}
#endif

static ESValue URLGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->urlString();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue documentURIGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->urlString();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue compatModeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->compatMode();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue characterSetGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->characterSet();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue charsetGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->characterSet();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue contentTypeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->contentType();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue doctypeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    DocumentType* result = nullptr;
    result = originalObj->doctype();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue documentElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    Element* result = nullptr;
    result = originalObj->documentElement();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue locationGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    Location* result = nullptr;
    result = originalObj->location();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue locationSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    // Declare native value (empty when type is void)
    Location* forwards = nullptr;
    forwards = originalObj->location();
    if (forwards) {
        forwards->setLocation(value0);
    }
    return ESValue();
}

static ESValue bodyGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    HTMLElement* result = nullptr;
    result = originalObj->body();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue bodySetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    HTMLElement* value0 = nullptr;
    if (!arg0.isUndefinedOrNull()) {
        CHECK_TYPEOF(arg0, HTMLElement);
        value0 = (HTMLElement*)(arg0.asESPointer()
                                    ->asESObject()
                                    ->extraPointerData());
    }
    try {
        originalObj->setBody(value0);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    return ESValue();
}

static ESValue headGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    HTMLHeadElement* result = nullptr;
    result = originalObj->head();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

extern ESValue defaultViewDocumentGetterFunction(ESVMInstance* instance);

static ESValue hiddenGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    bool result;
    result = originalObj->hidden();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue visibilityStateGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->visibilityState();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue onabortGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onabort();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onabortSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnabort(value0);
    return ESValue();
}

static ESValue oncanplayGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->oncanplay();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue oncanplaySetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOncanplay(value0);
    return ESValue();
}

static ESValue oncanplaythroughGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->oncanplaythrough();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue oncanplaythroughSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOncanplaythrough(value0);
    return ESValue();
}

static ESValue onclickGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onclick();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onclickSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnclick(value0);
    return ESValue();
}

static ESValue ondurationchangeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->ondurationchange();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue ondurationchangeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOndurationchange(value0);
    return ESValue();
}

static ESValue onemptiedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onemptied();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onemptiedSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnemptied(value0);
    return ESValue();
}

static ESValue onendedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onended();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onendedSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnended(value0);
    return ESValue();
}

static ESValue onerrorGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onerror();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onerrorSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnerror(value0);
    return ESValue();
}

static ESValue onfocusGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onfocus();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onfocusSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnfocus(value0);
    return ESValue();
}

static ESValue onkeydownGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onkeydown();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onkeydownSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnkeydown(value0);
    return ESValue();
}

static ESValue onkeyupGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onkeyup();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onkeyupSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnkeyup(value0);
    return ESValue();
}

static ESValue onloadGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onload();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onloadSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnload(value0);
    return ESValue();
}

static ESValue onloadeddataGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onloadeddata();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onloadeddataSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnloadeddata(value0);
    return ESValue();
}

static ESValue onloadedmetadataGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onloadedmetadata();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onloadedmetadataSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnloadedmetadata(value0);
    return ESValue();
}

static ESValue onloadstartGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onloadstart();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onloadstartSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnloadstart(value0);
    return ESValue();
}

static ESValue onmouseoverGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onmouseover();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onmouseoverSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnmouseover(value0);
    return ESValue();
}

static ESValue onpauseGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onpause();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onpauseSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnpause(value0);
    return ESValue();
}

static ESValue onplayGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onplay();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onplaySetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnplay(value0);
    return ESValue();
}

static ESValue onplayingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onplaying();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onplayingSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnplaying(value0);
    return ESValue();
}

static ESValue onprogressGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onprogress();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onprogressSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnprogress(value0);
    return ESValue();
}

static ESValue onratechangeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onratechange();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onratechangeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnratechange(value0);
    return ESValue();
}

static ESValue onseekedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onseeked();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onseekedSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnseeked(value0);
    return ESValue();
}

static ESValue onseekingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onseeking();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onseekingSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnseeking(value0);
    return ESValue();
}

static ESValue onstalledGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onstalled();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onstalledSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnstalled(value0);
    return ESValue();
}

static ESValue onsuspendGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onsuspend();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onsuspendSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnsuspend(value0);
    return ESValue();
}

static ESValue ontimeupdateGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->ontimeupdate();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue ontimeupdateSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOntimeupdate(value0);
    return ESValue();
}

static ESValue onvolumechangeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onvolumechange();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onvolumechangeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnvolumechange(value0);
    return ESValue();
}

static ESValue onwaitingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onwaiting();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onwaitingSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnwaiting(value0);
    return ESValue();
}

static ESValue childrenGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    HTMLCollection* result = nullptr;
    result = originalObj->children();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue firstElementChildGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    Element* result = nullptr;
    result = originalObj->firstElementChild();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue lastElementChildGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    Element* result = nullptr;
    result = originalObj->lastElementChild();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue childElementCountGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->childElementCount();
    // Return ESValue from native value
    return ESValue(result);
}

// Implement for functions
static ESValue getElementsByTagNameFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                        "getElementsByTagName", "Document", "1", buffer);
    }
    // Declare native value (empty when type is void)
    HTMLCollection* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    result = originalObj->getElementsByTagName(value0);

    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue getElementsByClassNameFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                        "getElementsByClassName", "Document", "1", buffer);
    }
    // Declare native value (empty when type is void)
    HTMLCollection* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    result = originalObj->getElementsByClassName(value0);

    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue createElementFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                        "createElement", "Document", "1", buffer);
    }
    // Declare native value (empty when type is void)
    Element* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    try {
        result = originalObj->createElement(value0);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue createDocumentFragmentFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare native value (empty when type is void)
    DocumentFragment* result = nullptr;
    // Call native function (nargs: 0)
    try {
        result = originalObj->createDocumentFragment();
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue createTextNodeFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                        "createTextNode", "Document", "1", buffer);
    }
    // Declare native value (empty when type is void)
    Text* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    result = originalObj->createTextNode(value0);

    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue createCommentFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                        "createComment", "Document", "1", buffer);
    }
    // Declare native value (empty when type is void)
    Comment* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    result = originalObj->createComment(value0);

    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue createAttributeFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                        "createAttribute", "Document", "1", buffer);
    }
    // Declare native value (empty when type is void)
    Attr* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    try {
        result = originalObj->createAttribute(value0);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue elementFromPointFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 2) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                        "elementFromPoint", "Document", "2", buffer);
    }
    // Declare native value (empty when type is void)
    Element* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    // Handle argument arg1
    double value1;
    value1 = arg1.toNumber();

    // Handle argument arg0
    double value0;
    value0 = arg0.toNumber();

    // Call native function (nargs: 2)
    result = originalObj->elementFromPoint(value0, value1);

    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue getElementByIdFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                        "getElementById", "Document", "1", buffer);
    }
    // Declare native value (empty when type is void)
    Element* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    result = originalObj->getElementById(value0);

    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue querySelectorFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                        "querySelector", "Document", "1", buffer);
    }
    // Declare native value (empty when type is void)
    Element* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    try {
        result = originalObj->querySelector(value0);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    return ESValue(ESValue::ESNull);
}

static ESValue querySelectorAllFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                        "querySelectorAll", "Document", "1", buffer);
    }
    // Declare native value (empty when type is void)
    NodeList* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    try {
        result = originalObj->querySelectorAll(value0);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

ESFunctionObject* bindingDocument(ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* DocumentString = ESString::create("Document");
    ESFunctionObject* DocumentFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, DocumentString, 0, true, true);
    ESObject* DocumentPrototypeObj =
        DocumentFunction->protoType().asESPointer()->asESObject();
    DocumentFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    DocumentPrototypeObj->forceNonVectorHiddenClass(false);
    DocumentPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnNode()->protoType());
    DocumentFunction->set__proto__(fetchData(scriptBindingInstance)->fnNode());

// Bind for attributes
#ifdef STARFISH_EXP
    ESString* implementationString = ESString::create("implementation");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, implementationString,
        implementationGetterFunction, nullptr);
#endif

    ESString* URLString = ESString::create("URL");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, URLString, URLGetterFunction, nullptr);

    ESString* documentURIString = ESString::create("documentURI");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, documentURIString, documentURIGetterFunction,
        nullptr);

    ESString* compatModeString = ESString::create("compatMode");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, compatModeString, compatModeGetterFunction,
        nullptr);

    ESString* characterSetString = ESString::create("characterSet");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, characterSetString, characterSetGetterFunction,
        nullptr);

    ESString* charsetString = ESString::create("charset");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, charsetString, charsetGetterFunction, nullptr);

    ESString* contentTypeString = ESString::create("contentType");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, contentTypeString, contentTypeGetterFunction,
        nullptr);

    ESString* doctypeString = ESString::create("doctype");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, doctypeString, doctypeGetterFunction, nullptr);

    ESString* documentElementString = ESString::create("documentElement");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, documentElementString,
        documentElementGetterFunction, nullptr);

    ESString* locationString = ESString::create("location");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, locationString, locationGetterFunction,
        locationSetterFunction);

    ESString* bodyString = ESString::create("body");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, bodyString, bodyGetterFunction,
        bodySetterFunction);

    ESString* headString = ESString::create("head");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, headString, headGetterFunction, nullptr);

    ESString* defaultViewString = ESString::create("defaultView");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, defaultViewString,
        defaultViewDocumentGetterFunction, nullptr);

    ESString* hiddenString = ESString::create("hidden");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, hiddenString, hiddenGetterFunction, nullptr);

    ESString* visibilityStateString = ESString::create("visibilityState");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, visibilityStateString,
        visibilityStateGetterFunction, nullptr);

    ESString* onabortString = ESString::create("onabort");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onabortString, onabortGetterFunction,
        onabortSetterFunction);

    ESString* oncanplayString = ESString::create("oncanplay");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, oncanplayString, oncanplayGetterFunction,
        oncanplaySetterFunction);

    ESString* oncanplaythroughString = ESString::create("oncanplaythrough");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, oncanplaythroughString,
        oncanplaythroughGetterFunction, oncanplaythroughSetterFunction);

    ESString* onclickString = ESString::create("onclick");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onclickString, onclickGetterFunction,
        onclickSetterFunction);

    ESString* ondurationchangeString = ESString::create("ondurationchange");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, ondurationchangeString,
        ondurationchangeGetterFunction, ondurationchangeSetterFunction);

    ESString* onemptiedString = ESString::create("onemptied");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onemptiedString, onemptiedGetterFunction,
        onemptiedSetterFunction);

    ESString* onendedString = ESString::create("onended");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onendedString, onendedGetterFunction,
        onendedSetterFunction);

    ESString* onerrorString = ESString::create("onerror");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onerrorString, onerrorGetterFunction,
        onerrorSetterFunction);

    ESString* onfocusString = ESString::create("onfocus");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onfocusString, onfocusGetterFunction,
        onfocusSetterFunction);

    ESString* onkeydownString = ESString::create("onkeydown");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onkeydownString, onkeydownGetterFunction,
        onkeydownSetterFunction);

    ESString* onkeyupString = ESString::create("onkeyup");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onkeyupString, onkeyupGetterFunction,
        onkeyupSetterFunction);

    ESString* onloadString = ESString::create("onload");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onloadString, onloadGetterFunction,
        onloadSetterFunction);

    ESString* onloadeddataString = ESString::create("onloadeddata");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onloadeddataString, onloadeddataGetterFunction,
        onloadeddataSetterFunction);

    ESString* onloadedmetadataString = ESString::create("onloadedmetadata");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onloadedmetadataString,
        onloadedmetadataGetterFunction, onloadedmetadataSetterFunction);

    ESString* onloadstartString = ESString::create("onloadstart");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onloadstartString, onloadstartGetterFunction,
        onloadstartSetterFunction);

    ESString* onmouseoverString = ESString::create("onmouseover");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onmouseoverString, onmouseoverGetterFunction,
        onmouseoverSetterFunction);

    ESString* onpauseString = ESString::create("onpause");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onpauseString, onpauseGetterFunction,
        onpauseSetterFunction);

    ESString* onplayString = ESString::create("onplay");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onplayString, onplayGetterFunction,
        onplaySetterFunction);

    ESString* onplayingString = ESString::create("onplaying");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onplayingString, onplayingGetterFunction,
        onplayingSetterFunction);

    ESString* onprogressString = ESString::create("onprogress");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onprogressString, onprogressGetterFunction,
        onprogressSetterFunction);

    ESString* onratechangeString = ESString::create("onratechange");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onratechangeString, onratechangeGetterFunction,
        onratechangeSetterFunction);

    ESString* onseekedString = ESString::create("onseeked");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onseekedString, onseekedGetterFunction,
        onseekedSetterFunction);

    ESString* onseekingString = ESString::create("onseeking");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onseekingString, onseekingGetterFunction,
        onseekingSetterFunction);

    ESString* onstalledString = ESString::create("onstalled");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onstalledString, onstalledGetterFunction,
        onstalledSetterFunction);

    ESString* onsuspendString = ESString::create("onsuspend");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onsuspendString, onsuspendGetterFunction,
        onsuspendSetterFunction);

    ESString* ontimeupdateString = ESString::create("ontimeupdate");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, ontimeupdateString, ontimeupdateGetterFunction,
        ontimeupdateSetterFunction);

    ESString* onvolumechangeString = ESString::create("onvolumechange");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onvolumechangeString,
        onvolumechangeGetterFunction, onvolumechangeSetterFunction);

    ESString* onwaitingString = ESString::create("onwaiting");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, onwaitingString, onwaitingGetterFunction,
        onwaitingSetterFunction);

    ESString* childrenString = ESString::create("children");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, childrenString, childrenGetterFunction, nullptr);

    ESString* firstElementChildString = ESString::create("firstElementChild");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, firstElementChildString,
        firstElementChildGetterFunction, nullptr);

    ESString* lastElementChildString = ESString::create("lastElementChild");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, lastElementChildString,
        lastElementChildGetterFunction, nullptr);

    ESString* childElementCountString = ESString::create("childElementCount");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentPrototypeObj, childElementCountString,
        childElementCountGetterFunction, nullptr);

    // Bind for functions
    ESString* getElementsByTagNameString =
        ESString::create("getElementsByTagName");
    ESFunctionObject* getElementsByTagNameESFn =
        ESFunctionObject::create(nullptr, getElementsByTagNameFunction,
                                 getElementsByTagNameString, 1, false);
    DocumentPrototypeObj->defineDataProperty(
        getElementsByTagNameString, true, true, true, getElementsByTagNameESFn);

    ESString* getElementsByClassNameString =
        ESString::create("getElementsByClassName");
    ESFunctionObject* getElementsByClassNameESFn =
        ESFunctionObject::create(nullptr, getElementsByClassNameFunction,
                                 getElementsByClassNameString, 1, false);
    DocumentPrototypeObj->defineDataProperty(getElementsByClassNameString, true,
                                             true, true,
                                             getElementsByClassNameESFn);

    ESString* createElementString = ESString::create("createElement");
    ESFunctionObject* createElementESFn = ESFunctionObject::create(
        nullptr, createElementFunction, createElementString, 1, false);
    DocumentPrototypeObj->defineDataProperty(createElementString, true, true,
                                             true, createElementESFn);

    ESString* createDocumentFragmentString =
        ESString::create("createDocumentFragment");
    ESFunctionObject* createDocumentFragmentESFn =
        ESFunctionObject::create(nullptr, createDocumentFragmentFunction,
                                 createDocumentFragmentString, 0, false);
    DocumentPrototypeObj->defineDataProperty(createDocumentFragmentString, true,
                                             true, true,
                                             createDocumentFragmentESFn);

    ESString* createTextNodeString = ESString::create("createTextNode");
    ESFunctionObject* createTextNodeESFn = ESFunctionObject::create(
        nullptr, createTextNodeFunction, createTextNodeString, 1, false);
    DocumentPrototypeObj->defineDataProperty(createTextNodeString, true, true,
                                             true, createTextNodeESFn);

    ESString* createCommentString = ESString::create("createComment");
    ESFunctionObject* createCommentESFn = ESFunctionObject::create(
        nullptr, createCommentFunction, createCommentString, 1, false);
    DocumentPrototypeObj->defineDataProperty(createCommentString, true, true,
                                             true, createCommentESFn);

    ESString* createAttributeString = ESString::create("createAttribute");
    ESFunctionObject* createAttributeESFn = ESFunctionObject::create(
        nullptr, createAttributeFunction, createAttributeString, 1, false);
    DocumentPrototypeObj->defineDataProperty(createAttributeString, true, true,
                                             true, createAttributeESFn);

    ESString* elementFromPointString = ESString::create("elementFromPoint");
    ESFunctionObject* elementFromPointESFn = ESFunctionObject::create(
        nullptr, elementFromPointFunction, elementFromPointString, 2, false);
    DocumentPrototypeObj->defineDataProperty(elementFromPointString, true, true,
                                             true, elementFromPointESFn);

    ESString* getElementByIdString = ESString::create("getElementById");
    ESFunctionObject* getElementByIdESFn = ESFunctionObject::create(
        nullptr, getElementByIdFunction, getElementByIdString, 1, false);
    DocumentPrototypeObj->defineDataProperty(getElementByIdString, true, true,
                                             true, getElementByIdESFn);

    ESString* querySelectorString = ESString::create("querySelector");
    ESFunctionObject* querySelectorESFn = ESFunctionObject::create(
        nullptr, querySelectorFunction, querySelectorString, 1, false);
    DocumentPrototypeObj->defineDataProperty(querySelectorString, true, true,
                                             true, querySelectorESFn);

    ESString* querySelectorAllString = ESString::create("querySelectorAll");
    ESFunctionObject* querySelectorAllESFn = ESFunctionObject::create(
        nullptr, querySelectorAllFunction, querySelectorAllString, 1, false);
    DocumentPrototypeObj->defineDataProperty(querySelectorAllString, true, true,
                                             true, querySelectorAllESFn);

    return DocumentFunction;
}
}
