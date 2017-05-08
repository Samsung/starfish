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

#include "dom/CSSStyleDeclaration.h"
#include "dom/HTMLElement.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue dirGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->dir();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue dirSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setDir(value0);
    return ESValue();
}

static ESValue offsetWidthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    // Declare native value (empty when type is void)
    int32_t result;
    result = originalObj->offsetWidth();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue offsetHeightGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    // Declare native value (empty when type is void)
    int32_t result;
    result = originalObj->offsetHeight();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue onabortGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnabort(value0);
    return ESValue();
}

static ESValue oncanplayGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOncanplay(value0);
    return ESValue();
}

static ESValue oncanplaythroughGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOncanplaythrough(value0);
    return ESValue();
}

static ESValue onclickGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnclick(value0);
    return ESValue();
}

static ESValue ondurationchangeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOndurationchange(value0);
    return ESValue();
}

static ESValue onemptiedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnemptied(value0);
    return ESValue();
}

static ESValue onendedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnended(value0);
    return ESValue();
}

static ESValue onerrorGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnerror(value0);
    return ESValue();
}

static ESValue onfocusGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnfocus(value0);
    return ESValue();
}

static ESValue onkeydownGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnkeydown(value0);
    return ESValue();
}

static ESValue onkeyupGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnkeyup(value0);
    return ESValue();
}

static ESValue onloadGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnload(value0);
    return ESValue();
}

static ESValue onloadeddataGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnloadeddata(value0);
    return ESValue();
}

static ESValue onloadedmetadataGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnloadedmetadata(value0);
    return ESValue();
}

static ESValue onloadstartGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnloadstart(value0);
    return ESValue();
}

static ESValue onmouseoverGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnmouseover(value0);
    return ESValue();
}

static ESValue onpauseGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnpause(value0);
    return ESValue();
}

static ESValue onplayGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnplay(value0);
    return ESValue();
}

static ESValue onplayingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnplaying(value0);
    return ESValue();
}

static ESValue onprogressGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnprogress(value0);
    return ESValue();
}

static ESValue onratechangeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnratechange(value0);
    return ESValue();
}

static ESValue onseekedGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnseeked(value0);
    return ESValue();
}

static ESValue onseekingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnseeking(value0);
    return ESValue();
}

static ESValue onstalledGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnstalled(value0);
    return ESValue();
}

static ESValue onsuspendGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnsuspend(value0);
    return ESValue();
}

static ESValue ontimeupdateGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOntimeupdate(value0);
    return ESValue();
}

static ESValue onvolumechangeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnvolumechange(value0);
    return ESValue();
}

static ESValue onwaitingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
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
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnwaiting(value0);
    return ESValue();
}

extern ESValue styleHTMLElementGetterFunction(ESVMInstance* instance);

extern ESValue styleHTMLElementSetterFunction(ESVMInstance* instance);

// Implement for functions
static ESValue clickFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    // Declare native value (empty when type is void)
    // Call native function (nargs: 0)
    originalObj->click();

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue focusFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLElement);
    // Declare native value (empty when type is void)
    // Call native function (nargs: 0)
    originalObj->focus();

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

ESFunctionObject* bindingHTMLElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* HTMLElementString = ESString::create("HTMLElement");
    ESFunctionObject* HTMLElementFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, HTMLElementString, 0, true, true);
    ESObject* HTMLElementPrototypeObj =
        HTMLElementFunction->protoType().asESPointer()->asESObject();
    HTMLElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    HTMLElementPrototypeObj->forceNonVectorHiddenClass(false);
    HTMLElementPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnElement()->protoType());
    HTMLElementFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnElement());

    // Bind for attributes
    ESString* dirString = ESString::create("dir");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, dirString, dirGetterFunction,
        dirSetterFunction);

    ESString* offsetWidthString = ESString::create("offsetWidth");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, offsetWidthString, offsetWidthGetterFunction,
        nullptr);

    ESString* offsetHeightString = ESString::create("offsetHeight");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, offsetHeightString, offsetHeightGetterFunction,
        nullptr);

    ESString* onabortString = ESString::create("onabort");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onabortString, onabortGetterFunction,
        onabortSetterFunction);

    ESString* oncanplayString = ESString::create("oncanplay");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, oncanplayString, oncanplayGetterFunction,
        oncanplaySetterFunction);

    ESString* oncanplaythroughString = ESString::create("oncanplaythrough");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, oncanplaythroughString,
        oncanplaythroughGetterFunction, oncanplaythroughSetterFunction);

    ESString* onclickString = ESString::create("onclick");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onclickString, onclickGetterFunction,
        onclickSetterFunction);

    ESString* ondurationchangeString = ESString::create("ondurationchange");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, ondurationchangeString,
        ondurationchangeGetterFunction, ondurationchangeSetterFunction);

    ESString* onemptiedString = ESString::create("onemptied");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onemptiedString, onemptiedGetterFunction,
        onemptiedSetterFunction);

    ESString* onendedString = ESString::create("onended");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onendedString, onendedGetterFunction,
        onendedSetterFunction);

    ESString* onerrorString = ESString::create("onerror");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onerrorString, onerrorGetterFunction,
        onerrorSetterFunction);

    ESString* onfocusString = ESString::create("onfocus");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onfocusString, onfocusGetterFunction,
        onfocusSetterFunction);

    ESString* onkeydownString = ESString::create("onkeydown");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onkeydownString, onkeydownGetterFunction,
        onkeydownSetterFunction);

    ESString* onkeyupString = ESString::create("onkeyup");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onkeyupString, onkeyupGetterFunction,
        onkeyupSetterFunction);

    ESString* onloadString = ESString::create("onload");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onloadString, onloadGetterFunction,
        onloadSetterFunction);

    ESString* onloadeddataString = ESString::create("onloadeddata");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onloadeddataString, onloadeddataGetterFunction,
        onloadeddataSetterFunction);

    ESString* onloadedmetadataString = ESString::create("onloadedmetadata");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onloadedmetadataString,
        onloadedmetadataGetterFunction, onloadedmetadataSetterFunction);

    ESString* onloadstartString = ESString::create("onloadstart");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onloadstartString, onloadstartGetterFunction,
        onloadstartSetterFunction);

    ESString* onmouseoverString = ESString::create("onmouseover");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onmouseoverString, onmouseoverGetterFunction,
        onmouseoverSetterFunction);

    ESString* onpauseString = ESString::create("onpause");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onpauseString, onpauseGetterFunction,
        onpauseSetterFunction);

    ESString* onplayString = ESString::create("onplay");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onplayString, onplayGetterFunction,
        onplaySetterFunction);

    ESString* onplayingString = ESString::create("onplaying");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onplayingString, onplayingGetterFunction,
        onplayingSetterFunction);

    ESString* onprogressString = ESString::create("onprogress");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onprogressString, onprogressGetterFunction,
        onprogressSetterFunction);

    ESString* onratechangeString = ESString::create("onratechange");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onratechangeString, onratechangeGetterFunction,
        onratechangeSetterFunction);

    ESString* onseekedString = ESString::create("onseeked");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onseekedString, onseekedGetterFunction,
        onseekedSetterFunction);

    ESString* onseekingString = ESString::create("onseeking");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onseekingString, onseekingGetterFunction,
        onseekingSetterFunction);

    ESString* onstalledString = ESString::create("onstalled");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onstalledString, onstalledGetterFunction,
        onstalledSetterFunction);

    ESString* onsuspendString = ESString::create("onsuspend");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onsuspendString, onsuspendGetterFunction,
        onsuspendSetterFunction);

    ESString* ontimeupdateString = ESString::create("ontimeupdate");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, ontimeupdateString, ontimeupdateGetterFunction,
        ontimeupdateSetterFunction);

    ESString* onvolumechangeString = ESString::create("onvolumechange");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onvolumechangeString,
        onvolumechangeGetterFunction, onvolumechangeSetterFunction);

    ESString* onwaitingString = ESString::create("onwaiting");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, onwaitingString, onwaitingGetterFunction,
        onwaitingSetterFunction);

    ESString* styleString = ESString::create("style");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementPrototypeObj, styleString, styleHTMLElementGetterFunction,
        styleHTMLElementSetterFunction);

    // Bind for functions
    ESString* clickString = ESString::create("click");
    ESFunctionObject* clickESFn =
        ESFunctionObject::create(nullptr, clickFunction, clickString, 0, false);
    HTMLElementPrototypeObj->defineDataProperty(clickString, true, true, true,
                                                clickESFn);

    ESString* focusString = ESString::create("focus");
    ESFunctionObject* focusESFn =
        ESFunctionObject::create(nullptr, focusFunction, focusString, 0, false);
    HTMLElementPrototypeObj->defineDataProperty(focusString, true, true, true,
                                                focusESFn);

    return HTMLElementFunction;
}
}
