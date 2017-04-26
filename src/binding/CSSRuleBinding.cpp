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

#include "dom/CSSRule.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue typeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(CSSRule);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->type();
    // Return ESValue from native value
    return ESValue(result);
}

ESFunctionObject* bindingCSSRule(ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* CSSRuleString = ESString::create("CSSRule");
    ESFunctionObject* CSSRuleFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, CSSRuleString, 0, true, true);
    CSSRuleFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    CSSRuleFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    CSSRuleFunction->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)
            ->m_instance->globalObject()
            ->objectPrototype());
    ESObject* CSSRulePrototypeObj =
        CSSRuleFunction->protoType().asESPointer()->asESObject();

    // Bind for constants
    ESString* STYLE_RULEString = ESString::create("STYLE_RULE");
    ESValue STYLE_RULEValue = ESValue(1);
    CSSRulePrototypeObj->defineDataProperty(STYLE_RULEString, false, true,
                                            false, STYLE_RULEValue);

    CSSRuleFunction->defineDataProperty(STYLE_RULEString, false, true, false,
                                        STYLE_RULEValue);

    ESString* CHARSET_RULEString = ESString::create("CHARSET_RULE");
    ESValue CHARSET_RULEValue = ESValue(2);
    CSSRulePrototypeObj->defineDataProperty(CHARSET_RULEString, false, true,
                                            false, CHARSET_RULEValue);

    CSSRuleFunction->defineDataProperty(CHARSET_RULEString, false, true, false,
                                        CHARSET_RULEValue);

    ESString* IMPORT_RULEString = ESString::create("IMPORT_RULE");
    ESValue IMPORT_RULEValue = ESValue(3);
    CSSRulePrototypeObj->defineDataProperty(IMPORT_RULEString, false, true,
                                            false, IMPORT_RULEValue);

    CSSRuleFunction->defineDataProperty(IMPORT_RULEString, false, true, false,
                                        IMPORT_RULEValue);

    ESString* MEDIA_RULEString = ESString::create("MEDIA_RULE");
    ESValue MEDIA_RULEValue = ESValue(4);
    CSSRulePrototypeObj->defineDataProperty(MEDIA_RULEString, false, true,
                                            false, MEDIA_RULEValue);

    CSSRuleFunction->defineDataProperty(MEDIA_RULEString, false, true, false,
                                        MEDIA_RULEValue);

    ESString* FONT_FACE_RULEString = ESString::create("FONT_FACE_RULE");
    ESValue FONT_FACE_RULEValue = ESValue(5);
    CSSRulePrototypeObj->defineDataProperty(FONT_FACE_RULEString, false, true,
                                            false, FONT_FACE_RULEValue);

    CSSRuleFunction->defineDataProperty(FONT_FACE_RULEString, false, true,
                                        false, FONT_FACE_RULEValue);

    ESString* PAGE_RULEString = ESString::create("PAGE_RULE");
    ESValue PAGE_RULEValue = ESValue(6);
    CSSRulePrototypeObj->defineDataProperty(PAGE_RULEString, false, true, false,
                                            PAGE_RULEValue);

    CSSRuleFunction->defineDataProperty(PAGE_RULEString, false, true, false,
                                        PAGE_RULEValue);

    ESString* MARGIN_RULEString = ESString::create("MARGIN_RULE");
    ESValue MARGIN_RULEValue = ESValue(9);
    CSSRulePrototypeObj->defineDataProperty(MARGIN_RULEString, false, true,
                                            false, MARGIN_RULEValue);

    CSSRuleFunction->defineDataProperty(MARGIN_RULEString, false, true, false,
                                        MARGIN_RULEValue);

    ESString* NAMESPACE_RULEString = ESString::create("NAMESPACE_RULE");
    ESValue NAMESPACE_RULEValue = ESValue(10);
    CSSRulePrototypeObj->defineDataProperty(NAMESPACE_RULEString, false, true,
                                            false, NAMESPACE_RULEValue);

    CSSRuleFunction->defineDataProperty(NAMESPACE_RULEString, false, true,
                                        false, NAMESPACE_RULEValue);

    // Bind for attributes
    ESString* typeString = ESString::create("type");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CSSRulePrototypeObj, typeString, typeGetterFunction, nullptr);

    return CSSRuleFunction;
}
}
