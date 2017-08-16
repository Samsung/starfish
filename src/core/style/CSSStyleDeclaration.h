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

#ifndef __StarFishCSSStyleDeclaration__
#define __StarFishCSSStyleDeclaration__

#include "binding/ScriptWrappable.h"
#include "core/style/Style.h"

namespace StarFish {

class CSSRule;
class CSSStyleDeclaration : public ScriptWrappable {
    friend class StyleResolver;
    friend class StyleRuleCSSStyleDeclaration;

public:
    enum StyleType {
        ExternalStyle,
        InternalStyle,
        InlineStyle,
    };

    CSSStyleDeclaration(Element* element = nullptr,
                        StyleType styleType = InternalStyle)
        : ScriptWrappable(this)
    {
        m_element = element;
        m_styleType = styleType;
    }

    void addValuePair(CSSStyleValuePair p);

    void clear();

    CSSStyleDeclaration* clone(Element* element);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCSSStyleDeclaration() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    StyleType styleType()
    {
        return m_styleType;
    }

    void setStyleType(StyleType styleType)
    {
        m_styleType = styleType;
    }

    String* generateCSSText() const;

    void notifyNeedsStyleRecalc();

    void tokenizeCSSValue(CSSTokenVector& tokens, const char* src, size_t len,
                          const char* seperator = "", size_t seperatorCount = 0,
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
    String* Overflow();
    String* FlexFlow();
    String* Flex();
#define DECLARE_ATTRIBUTE_GETTER(name, ...) String* name();
    FOR_EACH_STYLE_ATTRIBUTE(DECLARE_ATTRIBUTE_GETTER)
#undef DECLARE_ATTRIBUTE_GETTER

    void addCSSValuePair(CSSStyleValuePair::KeyKind name,
                         CSSStyleValuePair ret);

    void removeCSSValuePair(CSSStyleValuePair::KeyKind name);

    void setBorder(const char* value, size_t len, bool isImportant);
    void setBorderTop(const char* value, size_t len, bool isImportant);
    void setBorderRight(const char* value, size_t len, bool isImportant);
    void setBorderBottom(const char* value, size_t len, bool isImportant);
    void setBorderLeft(const char* value, size_t len, bool isImportant);
    void setBackground(const char* value, size_t len, bool isImportant);
    void setBackgroundRepeat(const char* value, size_t len, bool isImportant);
    void setBackgroundPosition(const char* value, size_t len, bool isImportant);
    void setMargin(const char* value, size_t len, bool isImportant);
    void setPadding(const char* value, size_t len, bool isImportant);
    void setBorderWidth(const char* value, size_t len, bool isImportant);
    void setBorderStyle(const char* value, size_t len, bool isImportant);
    void setBorderColor(const char* value, size_t len, bool isImportant);
    void setFont(const char* value, size_t len, bool isImportant);
    void setTransition(const char* value, size_t len, bool isImportant);
    void setOverflow(const char* value, size_t len, bool isImportant);
    void setFlexFlow(const char* value, size_t len, bool isImportant);
    void setFlex(const char* value, size_t len, bool isImportant);

#define DECLARE_ATTRIBUTE_SETTER(name, ...) \
    void set##name(const char* value, size_t len, bool isImportant);
    FOR_EACH_STYLE_ATTRIBUTE(DECLARE_ATTRIBUTE_SETTER)
#undef DECLARE_ATTRIBUTE_SETTER

#define DECLARE_ATTRIBUTE_GETTER_FOURSIDE(PRE, ...) \
    String* PRE##__VA_ARGS__(bool* isCombined = nullptr);

    DECLARE_ATTRIBUTE_GETTER_FOURSIDE(Margin);
    DECLARE_ATTRIBUTE_GETTER_FOURSIDE(Padding);
    DECLARE_ATTRIBUTE_GETTER_FOURSIDE(Border, Width);
    DECLARE_ATTRIBUTE_GETTER_FOURSIDE(Border, Style);
    DECLARE_ATTRIBUTE_GETTER_FOURSIDE(Border, Color);
#undef DECLARE_ATTRIBUTE_GETTER_FOURSIDE

    static String* combineBoxString(String* t, String* r, String* b, String* l,
                                    bool* isCombined = nullptr);

    uint32_t length() const;

    String* item(uint32_t index);

    String* cssText() const;
    virtual void setCssText(String* text);

    String* getPropertyValue(String* name);
    void setProperty(String* name, String* value, String* priority);

    // NOTE Based on IDL,
    // CSSStyleDeclaration has namedGetter, namedEnumerator, setter
    Nullable<String*> defaultNamedGetter(String* name);
    void defaultSetter(String* name, Nullable<String*> value);
    void defaultNamedEnumerator(std::vector<const char*>& enums);

    const GCAtomicVector<CSSStyleValuePair>& cssValues()
    {
        return m_cssValues;
    }

    virtual CSSRule* parentRule() const
    {
        return nullptr;
    }

    String* cssFloat()
    {
        return getPropertyValue(String::createASCIIString("float"));
    }

    void setCssFloat(String* value)
    {
        setProperty(String::createASCIIString("float"), value,
                    String::emptyString);
    }

protected:
    void rootPointerValueIfExists(CSSStyleValuePair v);

    GCAtomicVector<CSSStyleValuePair> m_cssValues;
    GCUnorderedSet<void*> m_pointerRooter;
    Element* m_element;
    StyleType m_styleType;
};

class StyleRuleCSSStyleDeclaration : public CSSStyleDeclaration {
public:
    StyleRuleCSSStyleDeclaration(CSSStyleDeclaration* src, CSSRule* parentRule)
        : CSSStyleDeclaration()
    {
        m_cssValues = src->m_cssValues;
        m_pointerRooter = src->m_pointerRooter;
        m_parentRule = parentRule;
    }
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    CSSRule* parentRule() const
    {
        return m_parentRule;
    }

    CSSStyleSheet* parentStyleSheet() const;
    void setCssText(String* text);

protected:
    CSSRule* m_parentRule;
};

class InlineCSSStyleDeclaration : public CSSStyleDeclaration {
public:
    InlineCSSStyleDeclaration(Element* element = nullptr,
                              StyleType styleType = InternalStyle)
        : CSSStyleDeclaration(element, styleType)
    {
    }

    void setCssText(String* text);
};

class ComputedStyleCSSStyleDeclaration : public CSSStyleDeclaration {
public:
    ComputedStyleCSSStyleDeclaration(Element* element = nullptr,
                                     StyleType styleType = InternalStyle)
        : CSSStyleDeclaration(element, styleType)
    {
    }

    void setCssText(String* text);
};
}

#endif
