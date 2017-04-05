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

#ifndef __StarFishCSSStyleDeclaration__
#define __StarFishCSSStyleDeclaration__

#include "style/Style.h"

namespace StarFish {

class CSSStyleDeclaration : public ScriptWrappable {
    friend class StyleResolver;

public:
    enum StyleType {
        ExternalStyle,
        InternalStyle,
        InlineStyle,
    };

    CSSStyleDeclaration(Document* document, Element* element = NULL,
                        StyleType styleType = InternalStyle)
        : ScriptWrappable(this)
        , m_document(document)
    {
        m_element = element;
        m_styleType = styleType;
    }

    void addValuePair(CSSStyleValuePair p)
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

    void clear()
    {
        m_cssValues.clear();
    }

    Document* document()
    {
        return m_document;
    }

    CSSStyleDeclaration* clone(Document* document, Element* element)
    {
        CSSStyleDeclaration* newStyle =
            new CSSStyleDeclaration(document, element);
        newStyle->m_cssValues = m_cssValues;

        return newStyle;
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual bool isCSSStyleDeclaration() const
    {
        return true;
    }

    StyleType styleType()
    {
        return m_styleType;
    }

    void setStyleType(StyleType styleType)
    {
        m_styleType = styleType;
    }

    String* generateCSSText();

    void notifyNeedsStyleRecalc();

    void tokenizeCSSValue(GCVector<String*>* tokens, String* src,
                          String* seperator = String::emptyString,
                          bool isCaseSensitive = false);

    String* Border();
    String* BorderTop();
    String* BorderRight();
    String* BorderBottom();
    String* BorderLeft();
    String* Background();
    String* BackgroundRepeat();
    String* BackgroundPosition();
    String* Font();
    String* Transition();
#define ATTRIBUTE_GETTER(name, ...)                                           \
    String* name()                                                            \
    {                                                                         \
        for (unsigned i = 0; i < m_cssValues.size(); i++) {                   \
            if (m_cssValues[i].keyKind() == CSSStyleValuePair::KeyKind::name) \
                return m_cssValues[i].toString();                             \
        }                                                                     \
        return String::emptyString;                                           \
    }

    FOR_EACH_STYLE_ATTRIBUTE(ATTRIBUTE_GETTER)
#undef ATTRIBUTE_GETTER

    void addCSSValuePair(CSSStyleValuePair::KeyKind name, CSSStyleValuePair ret)
    {
        for (unsigned i = 0; i < m_cssValues.size(); i++) {
            if (m_cssValues[i].keyKind() == name) {
                if (styleType() == StyleType::InlineStyle ||
                    ret.flagImportant() == true ||
                    (ret.flagImportant() == false &&
                     m_cssValues[i].flagImportant() == false)) {
                    m_cssValues[i].setValueKind(ret.valueKind());
                    m_cssValues[i].setValue(ret.value());
                    m_cssValues[i].setFlagImportant(ret.flagImportant());
                    notifyNeedsStyleRecalc();
                }

                return;
            }
        }
        ret.setKeyKind(name);
        m_cssValues.push_back(ret);
        notifyNeedsStyleRecalc();
    }

    void removeCSSValuePair(CSSStyleValuePair::KeyKind name)
    {
        unsigned len = m_cssValues.size();
        for (unsigned i = 0; i < len; i++) {
            if (m_cssValues[i].keyKind() == name) {
                m_cssValues.erase(m_cssValues.begin() + i);
                notifyNeedsStyleRecalc();
                return;
            }
        }
    }

    void setBorder(String* value, bool isImportant);
    void setBorderTop(String* value, bool isImportant);
    void setBorderRight(String* value, bool isImportant);
    void setBorderBottom(String* value, bool isImportant);
    void setBorderLeft(String* value, bool isImportant);
    void setBackground(String* value, bool isImportant);
    void setBackgroundRepeat(String* value, bool isImportant);
    void setBackgroundPosition(String* value, bool isImportant);
    void setMargin(String* value, bool isImportant);
    void setPadding(String* value, bool isImportant);
    void setBorderWidth(String* value, bool isImportant);
    void setBorderStyle(String* value, bool isImportant);
    void setBorderColor(String* value, bool isImportant);
    void setFont(String* value, bool isImportant);
    void setTransition(String* value, bool isImportant);

#define ATTRIBUTE_SETTER(name, ...)                                      \
    void set##name(String* value, bool isImportant)                      \
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

    FOR_EACH_STYLE_ATTRIBUTE(ATTRIBUTE_SETTER)
#undef ATTRIBUTE_SETTER

#define ATTRIBUTE_GETTER_FOURSIDE(PRE, ...)                               \
    String* PRE##__VA_ARGS__(bool* isCombined = nullptr)                  \
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
    ATTRIBUTE_GETTER_FOURSIDE(Margin);
    ATTRIBUTE_GETTER_FOURSIDE(Padding);
    ATTRIBUTE_GETTER_FOURSIDE(Border, Width);
    ATTRIBUTE_GETTER_FOURSIDE(Border, Style);
    ATTRIBUTE_GETTER_FOURSIDE(Border, Color);
#undef ATTRIBUTE_GETTER_FOURSIDE

    static String* combineBoxString(String* t, String* r, String* b, String* l,
                                    bool* isCombined = nullptr)
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

    unsigned long length() const
    {
        return m_cssValues.size();
    }

    String* item(unsigned long index)
    {
        if (index < m_cssValues.size()) {
            return m_cssValues[index].keyName();
        }
        return String::emptyString;
    }

protected:
    GCVector<CSSStyleValuePair> m_cssValues;
    Document* m_document;
    Element* m_element;
    StyleType m_styleType;
};
}

#endif
