/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "core/style/CSSStyleDeclaration.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/style/CSSStyleLookupTrie.h"

namespace StarFish {

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

ScriptBindingInstance* CSSStyleDeclaration::scriptBindingInstance()
{
    STARFISH_ASSERT(m_element);
    return m_element->scriptBindingInstance();
}

CSSStyleDeclaration* CSSStyleDeclaration::clone(Element* element)
{
    CSSStyleDeclaration* newStyle = new CSSStyleDeclaration(element);
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

#define DEFINE_ATTRIBUTE_SETTER(name, ...)                                    \
    void CSSStyleDeclaration::set##name(const char* value, size_t len,        \
                                        bool isImportant)                     \
    {                                                                         \
        if (len == 0) {                                                       \
            removeCSSValuePair(CSSStyleValuePair::KeyKind::name);             \
            return;                                                           \
        }                                                                     \
        CSSTokenVector tokens;                                                \
        if (UNLIKELY(CSSStyleValuePair::KeyKind::name ==                      \
                     CSSStyleValuePair::KeyKind::Content)) {                  \
            tokenizeCSSValue(tokens, value, len, "", 0, true);                \
        } else {                                                              \
            tokenizeCSSValue(tokens, value, len, ",", 1);                     \
        }                                                                     \
        CSSStyleValuePair ret;                                                \
        if (ret.updateValueCommon(tokens) || ret.updateValue##name(tokens)) { \
            ret.setFlagImportant(isImportant);                                \
            addCSSValuePair(CSSStyleValuePair::KeyKind::name, ret);           \
        }                                                                     \
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

uint32_t CSSStyleDeclaration::length() const
{
    return m_cssValues.size();
}

String* CSSStyleDeclaration::item(uint32_t index)
{
    if (index < m_cssValues.size()) {
        return m_cssValues[index].keyName();
    }
    return String::emptyString;
}

String* CSSStyleDeclaration::getPropertyValue(String* name)
{
    auto str = name->toNullableUTF8String();
    CSSStyleKind kind = lookupCSSStyle(str.m_buffer, str.m_bufferSize);
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
    if (prior->length() > 0) {
        if (prior->equalsWithoutCase("important")) {
            isImportant = true;
        } else {
            return;
        }
    }

    auto str = name->toNullableUTF8String();
    CSSStyleKind kind = lookupCSSStyle(str.m_buffer, str.m_bufferSize);

    struct Sender {
        CSSStyleDeclaration* self;
        CSSStyleKind kind;
        bool isImportant;
    } sender;
    sender.self = this;
    sender.kind = kind;
    sender.isImportant = isImportant;
    value->peekUTF8Buffer(
        [](const char* buf, size_t len, void* data) -> size_t {
            CSSStyleKind kind = ((Sender*)data)->kind;
            CSSStyleDeclaration* self = ((Sender*)data)->self;
            bool isImportant = ((Sender*)data)->isImportant;
            if (kind == CSSStyleKind::Unknown) {
            } else {
                if (false) {
                }
#define SET_ATTR(name, nameLower, nameCSSCase)  \
    else if (kind == CSSStyleKind::name)        \
    {                                           \
        self->set##name(buf, len, isImportant); \
    }
                FOR_EACH_STYLE_ATTRIBUTE_TOTAL(SET_ATTR)
#undef SET_ATTR
            }
            return 0;
        },
        &sender);
}

String* CSSStyleDeclaration::cssText() const
{
    return generateCSSText();
}

void CSSStyleDeclaration::setCssText(String* text)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

Nullable<String*> CSSStyleDeclaration::defaultNamedGetter(String* name)
{
    auto str = name->toNullableUTF8String();
    CSSStyleKind kind = lookupCSSStyleCamelCase(str.m_buffer, str.m_bufferSize);

    if (kind == CSSStyleKind::Unknown) {
        kind = lookupCSSStyle(str.m_buffer, str.m_bufferSize);
    }
    if (kind == CSSStyleKind::Unknown) {
        return Nullable<String*>();
    }
    if (false) {
    }
#define GET_ATTR(name, ...)               \
    else if (kind == CSSStyleKind::name)  \
    {                                     \
        return Nullable<String*>(name()); \
    }
    FOR_EACH_STYLE_ATTRIBUTE_TOTAL(GET_ATTR)
#undef GET_ATTR
    return Nullable<String*>();
}

void CSSStyleDeclaration::defaultNamedEnumerator(
    std::vector<const char*>& enums)
{
#define ENUM_ATTR(name, nameLower, ...) enums.push_back(#nameLower);
    FOR_EACH_STYLE_ATTRIBUTE_TOTAL(ENUM_ATTR)
#undef ENUM_ATTR
}

void CSSStyleDeclaration::defaultSetter(String* name, Nullable<String*> value)
{
    auto str = name->toNullableUTF8String();
    CSSStyleKind kind = lookupCSSStyleCamelCase(str.m_buffer, str.m_bufferSize);

    if (kind == CSSStyleKind::Unknown) {
        kind = lookupCSSStyle(str.m_buffer, str.m_bufferSize);
    }
    if (kind == CSSStyleKind::Unknown) {
        return;
    }
    // Empty string let setter remove its value
    String* valueTo = String::emptyString;
    if (value.hasValue()) {
        valueTo = value.getValue();
    }

    struct Sender {
        CSSStyleDeclaration* self;
        CSSStyleKind kind;
    } sender;
    sender.self = this;
    sender.kind = kind;

    valueTo->peekUTF8Buffer(
        [](const char* buf, size_t len, void* data) -> size_t {
            CSSStyleKind kind = ((Sender*)data)->kind;
            CSSStyleDeclaration* self = ((Sender*)data)->self;
            if (false) {
            }
#define SET_ATTR(name, ...)               \
    else if (kind == CSSStyleKind::name)  \
    {                                     \
        self->set##name(buf, len, false); \
    }
            FOR_EACH_STYLE_ATTRIBUTE_TOTAL(SET_ATTR)
#undef SET_ATTR
            return 0;
        },
        &sender);
}
}
