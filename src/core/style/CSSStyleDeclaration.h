/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#ifndef __StarFishCSSStyleDeclaration__
#define __StarFishCSSStyleDeclaration__

#include "binding/ScriptWrappable.h"
#include "core/style/Style.h"

namespace StarFish {

class CSSRule;

class MutablePropertyValue : public gc {
public:
    MutablePropertyValue(String* name, String* value)
        : m_name(name)
        , m_value(value)
    {
    }

    String* name()
    {
        return m_name;
    }

    String* value()
    {
        return m_value;
    }

    void setName(String* name)
    {
        m_name = name;
    }

    void setValue(String* value)
    {
        m_value = value;
    }

private:
    String* m_name;
    String* m_value;
};

class CSSStyleDeclaration : public ScriptWrappable {
    friend class StyleResolver;
    friend class StyleRuleCSSStyleDeclaration;

public:
    enum Stage { resolveStyle, frameTreeBuild, layout };

    CSSStyleDeclaration(Element* element)
        : ScriptWrappable(this)
    {
        m_node = (Node*)element;
    }

    CSSStyleDeclaration(Document* document)
        : ScriptWrappable(this)
    {
        m_node = (Node*)document;
    }

    void addValuePair(CSSStyleValuePair p);

    void clear();

    CSSStyleDeclaration* clone(Element* element);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCSSStyleDeclaration() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    String* generateCSSText() const;

    void notifyNeedsStyleRecalc();

    static void tokenizeCSSValue(CSSTokenVector& tokens, const char* src,
                                 size_t len, const char* seperator = "",
                                 size_t seperatorCount = 0,
                                 bool isCaseSensitive = false,
                                 bool preserveContentWS = false);

    String* Border();
    String* BorderTop();
    String* BorderRight();
    String* BorderBottom();
    String* BorderLeft();
    String* BorderRadius();
    String* Background();
    String* BackgroundRepeat();
    String* BackgroundPosition();
    String* Font();
    String* FontFamily();
    String* Src();
    String* Transition();
    String* TransitionTransitionProperty();
    String* TransitionTransitionDuration();
    String* TransitionTransitionDelay();
    String* Overflow();
    String* FlexFlow();
    String* Flex();
    String* Outline();
    String* D();
    String* ListStyle();
    String* customProperty(String* key);
#define DECLARE_ATTRIBUTE_GETTER(name, ...) String* name();
    FOR_EACH_STYLE_ATTRIBUTE_BASIC(DECLARE_ATTRIBUTE_GETTER)
#undef DECLARE_ATTRIBUTE_GETTER

    void addCSSValuePair(CSSStyleValuePair::KeyKind name,
                         CSSStyleValuePair ret);

    void removeCSSValuePair(CSSStyleValuePair::KeyKind name);
    bool hasCSSValuePair(CSSStyleValuePair::KeyKind name);
    CSSStyleValuePair getCSSValuePair(CSSStyleValuePair::KeyKind name);

    void setBorder(const char* value, size_t len, bool isImportant);
    void setBorderTop(const char* value, size_t len, bool isImportant);
    void setBorderRight(const char* value, size_t len, bool isImportant);
    void setBorderBottom(const char* value, size_t len, bool isImportant);
    void setBorderLeft(const char* value, size_t len, bool isImportant);
    void setBorderRadius(const char* value, size_t len, bool isImportant);
    void setBackground(const char* value, size_t len, bool isImportant);
    void setBackgroundRepeat(const char* value, size_t len, bool isImportant);
    void setBackgroundPosition(const char* value, size_t len, bool isImportant);
    void setMargin(const char* value, size_t len, bool isImportant);
    void setPadding(const char* value, size_t len, bool isImportant);
    void setBorderWidth(const char* value, size_t len, bool isImportant);
    void setBorderStyle(const char* value, size_t len, bool isImportant);
    void setBorderColor(const char* value, size_t len, bool isImportant);
    void setFont(const char* value, size_t len, bool isImportant);
    void setFontFamily(const char* value, size_t len, bool isImportant);
    void setSrc(const char* value, size_t len, bool isImportant);
    void setTransition(const char* value, size_t len, bool isImportant);
    void setTransitionTransitionProperty(const char* value, size_t len,
                                         bool isImportant);
    void setTransitionTransitionDuration(const char* value, size_t len,
                                         bool isImportant);
    void setTransitionTransitionDelay(const char* value, size_t len,
                                      bool isImportant);
    void setOverflow(const char* value, size_t len, bool isImportant);
    void setFlexFlow(const char* value, size_t len, bool isImportant);
    void setFlex(const char* value, size_t len, bool isImportant);
    void setOutline(const char* value, size_t len, bool isImportant);
    void setD(const char* value, size_t len, bool isImportant);
    void setListStyle(const char* value, size_t len, bool isImportant);

    void setCustomProperty(String* key, String* value, size_t len);

#define DECLARE_ATTRIBUTE_SETTER(name, ...) \
    void set##name(const char* value, size_t len, bool isImportant);
    FOR_EACH_STYLE_ATTRIBUTE_BASIC(DECLARE_ATTRIBUTE_SETTER)
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
    // CSSStyleDeclaration has namedGetter, namedEnumerator, namedSetter
    Nullable<String*> defaultNamedGetter(String* name);
    bool defaultNamedSetter(String* name, Nullable<String*> value);
    void defaultNamedEnumerator(GCVector<String*>& enums);

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

    virtual bool isInlineStyle() const
    {
        return false;
    }

    virtual bool isComputedStyle() const
    {
        return false;
    }

    virtual void layoutIfNeeds()
    {
    }

    virtual void resolveStyleIfNeeds()
    {
    }

    virtual void buildFrameTreeIfNeeds()
    {
    }

    virtual void updateValue(CSSStyleValuePair::KeyKind keyKind)
    {
    }

    virtual Stage requiredStage(CSSStyleValuePair::KeyKind keyKind)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

protected:
    void rootPointerValueIfExists(CSSStyleValuePair v);

    GCAtomicVector<CSSStyleValuePair> m_cssValues;
    GCVector<MutablePropertyValue> m_cssCustomValues;
    GCUnorderedSet<void*> m_pointerRooter;
    Node* m_node;
};

class StyleRuleCSSStyleDeclaration : public CSSStyleDeclaration {
public:
    StyleRuleCSSStyleDeclaration(CSSStyleDeclaration* src, CSSRule* parentRule);

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
    InlineCSSStyleDeclaration(Element* element)
        : CSSStyleDeclaration(element)
    {
    }

    void setCssText(String* text) override;

    bool isInlineStyle() const override
    {
        return true;
    }
};

class ComputedStyleCSSStyleDeclaration : public CSSStyleDeclaration {
public:
    ComputedStyleCSSStyleDeclaration(Element* element)
        : CSSStyleDeclaration(element)
    {
    }

    void setCssText(String* text) override;

    bool isComputedStyle() const override
    {
        return true;
    }

    void layoutIfNeeds() override;
    void resolveStyleIfNeeds() override;
    void buildFrameTreeIfNeeds() override;

    void updateValue(CSSStyleValuePair::KeyKind keyKind) override;
    Stage requiredStage(CSSStyleValuePair::KeyKind keyKind) override;
};
}

#endif
