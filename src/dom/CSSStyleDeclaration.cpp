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
#include "dom/Document.h"
#include "style/CSSStyleLookupTrie.h"

namespace StarFish {

static ESValue readCallbackFunction(const ESValue& key, ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    CSSStyleDeclaration* self = (CSSStyleDeclaration*)obj->extraPointerData();
    STARFISH_ASSERT(self->isCSSStyleDeclaration());
    uint32_t idx = key.toIndex();
    if (idx < self->length()) {
        return ESString::create(self->item(idx)->utf8Data());
    }

    if (idx == ESValue::ESInvalidIndexValue) {
        const char* str = toBrowserString(key)->utf8Data();
        CSSStyleKind kind = lookupCSSStyleCamelCase(str, strlen(str));

        if (kind == CSSStyleKind::Unknown) {
            kind = lookupCSSStyle(str, strlen(str));
        }
        if (kind == CSSStyleKind::Unknown) {
            return ESValue(ESValue::ESDeletedValue);
        } else {
            if (false) {
            }
#define GET_ATTR(name, nameLower, nameCSSCase)   \
    else if (kind == CSSStyleKind::name)         \
    {                                            \
        return createScriptString(self->name()); \
    }
            FOR_EACH_STYLE_ATTRIBUTE_TOTAL(GET_ATTR)
#undef GET_ATTR
        }
    }

    return ESString::create("");
}

static bool writeCallbackFunction(const ESValue& key, const ESValue& val,
                                  ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    CSSStyleDeclaration* self = (CSSStyleDeclaration*)obj->extraPointerData();
    STARFISH_ASSERT(self->isCSSStyleDeclaration());
    const char* str = toBrowserString(key)->utf8Data();
    CSSStyleKind kind = lookupCSSStyleCamelCase(str, strlen(str));

    if (kind == CSSStyleKind::Unknown) {
        kind = lookupCSSStyle(str, strlen(str));
    }
    if (kind == CSSStyleKind::Unknown) {
        return false;
    } else {
        if (false) {
        }
#define SET_ATTR(name, nameLower, nameCSSCase)        \
    else if (kind == CSSStyleKind::name)              \
    {                                                 \
        self->set##name(toBrowserString(val), false); \
        return true;                                  \
    }
        FOR_EACH_STYLE_ATTRIBUTE_TOTAL(SET_ATTR)
#undef SET_ATTR
    }

    return false;
}

static ESValueVector enumerateCallbackFunction(ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    CSSStyleDeclaration* self = (CSSStyleDeclaration*)obj->extraPointerData();
    STARFISH_ASSERT(self->isCSSStyleDeclaration());
    size_t len = self->length();
    ESValueVector v(len);
    for (size_t i = 0; i < len; i++) {
        v[i] = ESValue(i);
    }

#define ENUM_ATTR(name, nameLower, nameCSSCase) \
    v.push_back(ESString::create(#nameLower));

    FOR_EACH_STYLE_ATTRIBUTE_TOTAL(ENUM_ATTR)
#undef ENUM_ATTR
    return v;
}

void CSSStyleDeclaration::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnCSSStyleDeclaration()->protoType());

    scriptObject()->setPropertyInterceptor(readCallbackFunction,
                                           writeCallbackFunction,
                                           enumerateCallbackFunction, true);
}

void CSSStyleDeclaration::addValuePair(CSSStyleValuePair p)
{
    for (size_t i = 0; i < m_cssValues.size(); i++) {
        CSSStyleValuePair v = m_cssValues[i];
        if (v.keyKind() == p.keyKind()) {
            m_cssValues[i] = p;
            return;
        }
    }
    m_cssValues.push_back(p);
}

void CSSStyleDeclaration::clear()
{
    m_cssValues.clear();
}

CSSStyleDeclaration* CSSStyleDeclaration::clone(Document* document,
                                                Element* element)
{
    CSSStyleDeclaration* newStyle = new CSSStyleDeclaration(document, element);
    newStyle->m_cssValues = m_cssValues;

    return newStyle;
}

#define DEFINE_ATTRIBUTE_GETTER(name, ...)                                    \
    String* CSSStyleDeclaration::name()                                       \
    {                                                                         \
        for (unsigned i = 0; i < m_cssValues.size(); i++) {                   \
            if (m_cssValues[i].keyKind() == CSSStyleValuePair::KeyKind::name) \
                return m_cssValues[i].toString();                             \
        }                                                                     \
        return String::emptyString;                                           \
    }
FOR_EACH_STYLE_ATTRIBUTE(DEFINE_ATTRIBUTE_GETTER)
#undef DEFINE_ATTRIBUTE_GETTER

#define DEFINE_ATTRIBUTE_SETTER(name, ...)                               \
    void CSSStyleDeclaration::set##name(String* value, bool isImportant) \
    {                                                                    \
        if (value->length() == 0) {                                      \
            removeCSSValuePair(CSSStyleValuePair::KeyKind::name);        \
            return;                                                      \
        }                                                                \
        GCVector<String*> tokens;                                        \
        if (CSSStyleValuePair::KeyKind::name ==                          \
            CSSStyleValuePair::KeyKind::Content) {                       \
            tokenizeCSSValue(&tokens, value, String::emptyString, true); \
        } else {                                                         \
            tokenizeCSSValue(&tokens, value, String::fromUTF8(","));     \
        }                                                                \
        CSSStyleValuePair ret;                                           \
        if (ret.updateValueCommon(&tokens) ||                            \
            ret.updateValue##name(&tokens)) {                            \
            ret.setFlagImportant(isImportant);                           \
            addCSSValuePair(CSSStyleValuePair::KeyKind::name, ret);      \
        }                                                                \
    }

FOR_EACH_STYLE_ATTRIBUTE(DEFINE_ATTRIBUTE_SETTER)
#undef DEFINE_ATTRIBUTE_SETTER

#define DEFINE_ATTRIBUTE_GETTER_FOURSIDE(PRE, ...)                        \
    String* CSSStyleDeclaration::PRE##__VA_ARGS__(bool* isCombined)       \
    {                                                                     \
        String* top = PRE##Top##__VA_ARGS__();                            \
        if (!top->equals(String::emptyString)) {                          \
            String* right = PRE##Right##__VA_ARGS__();                    \
            if (!right->equals(String::emptyString)) {                    \
                String* bottom = PRE##Bottom##__VA_ARGS__();              \
                if (!bottom->equals(String::emptyString)) {               \
                    String* left = PRE##Left##__VA_ARGS__();              \
                    if (!left->equals(String::emptyString)) {             \
                        return combineBoxString(top, right, bottom, left, \
                                                isCombined);              \
                    }                                                     \
                }                                                         \
            }                                                             \
        }                                                                 \
        return String::emptyString;                                       \
    }
DEFINE_ATTRIBUTE_GETTER_FOURSIDE(Margin);
DEFINE_ATTRIBUTE_GETTER_FOURSIDE(Padding);
DEFINE_ATTRIBUTE_GETTER_FOURSIDE(Border, Width);
DEFINE_ATTRIBUTE_GETTER_FOURSIDE(Border, Style);
DEFINE_ATTRIBUTE_GETTER_FOURSIDE(Border, Color);
#undef DEFINE_ATTRIBUTE_GETTER_FOURSIDE

String* CSSStyleDeclaration::combineBoxString(String* t, String* r, String* b,
                                              String* l, bool* isCombined)
{
    if (isCombined) {
        *isCombined = true;
    }
    // [NOTICE]
    // All initial --> return "initial"
    // Not all, but more than 1 initial --> return ""
    size_t initialCount = 0;
    initialCount += t->equals(String::initialString) ? 1 : 0;
    initialCount += r->equals(String::initialString) ? 1 : 0;
    initialCount += b->equals(String::initialString) ? 1 : 0;
    initialCount += l->equals(String::initialString) ? 1 : 0;
    if (initialCount > 0 && initialCount < 4) {
    }

    String* space = String::spaceString;
    if (!r->equals(l)) {
        return t->concat(space)
            ->concat(r)
            ->concat(space)
            ->concat(b)
            ->concat(space)
            ->concat(l);
    } else if (!t->equals(b)) {
        return t->concat(space)->concat(r)->concat(space)->concat(b);
    } else if (!t->equals(r)) {
        return t->concat(space)->concat(r);
    } else {
        if (isCombined) {
            *isCombined = false;
        }
        return t;
    }
}

unsigned long CSSStyleDeclaration::length() const
{
    return m_cssValues.size();
}

String* CSSStyleDeclaration::item(unsigned long index)
{
    if (index < m_cssValues.size()) {
        return m_cssValues[index].keyName();
    }
    return String::emptyString;
}

String* CSSStyleDeclaration::getPropertyValue(String* name)
{
    const char* c = name->utf8Data();
    CSSStyleKind kind = lookupCSSStyle(c, strlen(c));
    String* val = String::emptyString;
    switch (kind) {
#define MATCH_KEY(Name, ...) \
    case CSSStyleKind::Name: \
        val = Name();        \
        break;
        FOR_EACH_STYLE_ATTRIBUTE_TOTAL(MATCH_KEY)
#undef MATCH_KEY
    default:
        break;
    }
    return val;
}

void CSSStyleDeclaration::setProperty(String* name, String* value,
                                      String* prior)
{
    bool isImportant = false;
    String* lower = prior->toLower();
    if (lower->length() > 0) {
        if (lower->equals(String::fromUTF8("important"))) {
            isImportant = true;
        } else {
            return;
        }
    }

    const char* c = name->utf8Data();
    CSSStyleKind kind = lookupCSSStyle(c, strlen(c));
    if (kind == CSSStyleKind::Unknown) {
    } else {
        if (false) {
        }
#define SET_ATTR(name, nameLower, nameCSSCase) \
    else if (kind == CSSStyleKind::name)       \
    {                                          \
        set##name(value, isImportant);         \
    }
        FOR_EACH_STYLE_ATTRIBUTE_TOTAL(SET_ATTR)
#undef SET_ATTR
    }
}
}
