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

#include "dom/DOM.h"
#include "dom/binding/escargot/ScriptBindingInstanceDataEscargot.h"
#include "style/CSSStyleLookupTrie.h"

namespace StarFish {

using namespace escargot;

static ESValue lengthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(
        ScriptWrappable::Type::CSSStyleDeclarationObject, CSSStyleDeclaration);
    uint32_t len = originalObj->length();
    return ESValue(len);
}

#ifdef STARFISH_ENABLE_TEST
static ESValue getPropertyValueFunction(ESVMInstance* instance)
{
    try {
        ESValue thisValue =
            instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue,
                     ScriptWrappable::Type::CSSStyleDeclarationObject);
        CSSStyleDeclaration* decl =
            (CSSStyleDeclaration*)thisValue.asESPointer()
                ->asESObject()
                ->extraPointerData();
        ESValue prop = instance->currentExecutionContext()->readArgument(0);

        if (prop.isESString()) {
            String* name = toBrowserString(prop);
            const char* c = name->utf8Data();
            CSSStyleKind kind = lookupCSSStyle(c, strlen(c));
            String* val = String::emptyString;
            switch (kind) {
#define MATCH_KEY(Name, ...) \
    case CSSStyleKind::Name: \
        val = decl->Name();  \
        break;
                FOR_EACH_STYLE_ATTRIBUTE_TOTAL(MATCH_KEY)
#undef MATCH_KEY
            default:
                break;
            }
            return toJSString(val);
        } else {
            return ESString::create("");
        }
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue setPropertyFunction(ESVMInstance* instance)
{
    try {
        ESValue thisValue =
            instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue,
                     ScriptWrappable::Type::CSSStyleDeclarationObject);
        CSSStyleDeclaration* decl =
            (CSSStyleDeclaration*)thisValue.asESPointer()
                ->asESObject()
                ->extraPointerData();
        ESValue prop = instance->currentExecutionContext()->readArgument(0);
        ESValue val = instance->currentExecutionContext()->readArgument(1);

        String* name = toBrowserString(prop.toString())->toLower();
        const char* c = name->utf8Data();
        CSSStyleKind kind = lookupCSSStyle(c, strlen(c));

        bool isImportant = false;
        if (instance->currentExecutionContext()->argumentCount() == 3) {
            String* pri = toBrowserString(instance->currentExecutionContext()
                                              ->readArgument(2)
                                              .toString())
                              ->toLower();
            if (!pri->equals(String::emptyString)) {
                if (pri->equals(String::fromUTF8("important"))) {
                    isImportant = true;
                } else {
                    return ESValue();
                }
            }

            if (kind == CSSStyleKind::Unknown) {
            } else {
                if (false) {
                }
#define SET_ATTR(name, nameLower, nameCSSCase)              \
    else if (kind == CSSStyleKind::name)                    \
    {                                                       \
        decl->set##name(toBrowserString(val), isImportant); \
    }
                FOR_EACH_STYLE_ATTRIBUTE_TOTAL(SET_ATTR)
            }
        }
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    return ESValue();
}
#endif

ESFunctionObject* bindingCSSStyleDeclaration(
    ScriptBindingInstance* scriptBindingInstance)
{
    /* style-related getter/setter start here */
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(CSSStyleDeclaration,
                                    fetchData(scriptBindingInstance)
                                        ->m_instance->globalObject()
                                        ->objectPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CSSStyleDeclarationFunction->protoType().asESPointer()->asESObject(),
        ESString::create("length"), lengthGetterFunction, nullptr);

#ifdef STARFISH_ENABLE_TEST
    CSSStyleDeclarationFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("getPropertyValue"), true, true, true,
            ESFunctionObject::create(nullptr, getPropertyValueFunction,
                                     ESString::create("getPropertyValue"), 2,
                                     false));

    CSSStyleDeclarationFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("setProperty"), true, true, true,
                             ESFunctionObject::create(
                                 nullptr, setPropertyFunction,
                                 ESString::create("setProperty"), 3, false));
#endif

    return CSSStyleDeclarationFunction;
}
}
