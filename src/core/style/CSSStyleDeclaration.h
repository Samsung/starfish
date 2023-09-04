/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#ifndef __StarfishCSSStyleDeclaration__
#define __StarfishCSSStyleDeclaration__

#include "binding/ScriptWrappable.h"
#include "core/style/Style.h"

namespace Starfish {

class CSSRule;

CSSStyleValuePair::KeyKind lookupName(const char* buf, size_t len);

class CSSStyleDeclaration : public ScriptWrappable {
    friend class StyleResolver;
    friend class StyleRuleCSSStyleDeclaration;

public:
    static CSSStyleValuePair lengthToCSSStyleValue(Length len);

    CSSStyleDeclaration(Element* element);
    CSSStyleDeclaration(Document* document);

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

    void addCSSValuePair(CSSStyleValuePair::KeyKind name,
                         const CSSStyleValuePair& ret);

    void removeCSSValuePair(CSSStyleValuePair::KeyKind name);
    bool hasCSSValuePair(CSSStyleValuePair::KeyKind name);
    CSSStyleValuePair getCSSValuePair(CSSStyleValuePair::KeyKind name);

    static String* combineBoxString(String* t, String* r, String* b, String* l,
                                    bool* isCombined = nullptr);

    uint32_t length() const;

    String* item(uint32_t index);

    String* cssText() const;
    virtual void setCssText(String* text);

    String* getPropertyValue(String* name);
    String* getPropertyPriority(String* name);
    String* getPropertyValueInternal(CSSStyleValuePair::KeyKind keyKind);

    void setProperty(String* name, String* value, String* priority);

    String* removeProperty(String* name);

    // NOTE Based on IDL,
    // CSSStyleDeclaration has namedGetter, namedEnumerator, namedSetter
    Nullable<String*> defaultNamedGetter(String* name);
    bool defaultNamedSetter(String* name, Nullable<String*> value);
    void defaultNamedEnumerator(GCVector<String*>& enums);

    template <typename T>
    void setProperty(T name, T value, bool isImportant, bool allowSrcProperty)
    {
        struct Params {
            CSSStyleDeclaration* self;
            std::pair<CSSStyleValuePair::KeyKind, AtomicString> result;
            bool isImportant;
        } params;
        params.self = this;
        params.isImportant = isImportant;

        // Resolve name type and custom proerty name if needed.
        name->peekUTF8Buffer(
            [](const char* buf, size_t len, void* data) -> size_t {
                Params* params = reinterpret_cast<Params*>(data);
                params->result.first = lookupName(buf, len);

                if (params->result.first ==
                    CSSStyleValuePair::KeyKind::CustomProperty) {
                    params->result.second = AtomicString::createAtomicString(
                        ((DocumentHoldable*)params->self->m_node)->starfish(),
                        buf, len);
                }
#ifndef NDEBUG
                if (params->result.first ==
                    CSSStyleValuePair::KeyKind::Unknown) {
                    STARFISH_LOG_ERROR("Unsupported property: %s", buf);
                }
#endif
                return 0;
            },
            &params);

        if (params.result.first == CSSStyleValuePair::KeyKind::Src &&
            !allowSrcProperty) {
            return;
        }

        value->peekUTF8Buffer(
            [](const char* buf, size_t len, void* data) -> size_t {
                Params* params = reinterpret_cast<Params*>(data);
                CSSStyleDeclaration* self = params->self;
                CSSStyleValuePair::KeyKind kind = params->result.first;

                if (kind == CSSStyleValuePair::KeyKind::Unknown) {
                } else {
                    if (kind == CSSStyleValuePair::KeyKind::CustomProperty) {
                        self->setCustomProperty(
                            params->result.second,
                            String::createASCIIString(buf, len));
                    } else {
                        self->setPropertyInternal(kind, buf, len,
                                                  params->isImportant);
                    }
                }
                return 0;
            },
            &params);
    }

    bool setPropertyInternal(CSSStyleValuePair::KeyKind keyKind,
                             const char* value, size_t valueLength,
                             bool isImportant);

    const GCAtomicVector<CSSStyleValuePair>& cssValues()
    {
        return m_cssValues;
    }

    Nullable<MutablePropertyValueList*> cssCustomValues()
    {
        return m_cssCustomValues;
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

    virtual void updateValue(CSSStyleValuePair::KeyKind keyKind)
    {
        // Do nothing.
    }

    static bool parseUnitPositionShorthand(const CSSTokenVector& tokens,
                                           CSSStyleValuePair::KeyKind keykind,
                                           CSSStyleValuePair* retx,
                                           CSSStyleValuePair* rety,
                                           bool allowComma = true);
    static bool parseFontShorthand(const CSSTokenVector& tokens,
                                   CSSStyleValuePair* style,
                                   // UNSUPPORTED CSSStyleValuePair* variant,
                                   CSSStyleValuePair* weight,
                                   // UNSUPPORTED CSSStyleValuePair* stretch,
                                   CSSStyleValuePair* size,
                                   CSSStyleValuePair* lineHeight,
                                   CSSStyleValuePair* family);

protected:
    void rootPointerValueIfExists(const CSSStyleValuePair& v);
    void removeRootPointerValue(const CSSStyleValuePair& v);

    bool isShorthandProperty(CSSStyleValuePair::KeyKind keyKind);
    bool isStickyProperty(CSSStyleValuePair::KeyKind keyKind);

    enum class PropertyType {
        kLonghand,
        kShorthand,
        kSticky, // This term is not used in the web standard.
    };

    template <PropertyType type>
    String* getPropertyValueInternalFor(CSSStyleValuePair::KeyKind keyKind);

    template <PropertyType type>
    bool setPropertyInternalFor(CSSStyleValuePair::KeyKind keyKind,
                                const char* value, size_t valueLength,
                                bool isImportant);

    String* removePropertyInternal(CSSStyleValuePair::KeyKind keyKind);

    // Named property getter/setter/remover for shorthand properties.
    // These are typically implemented as a combination of long-handed
    // properties.
    String* Border();
    void setBorder(const char* value, size_t len, bool isImportant);
    void removeBorder();

    String* BorderTop();
    void setBorderTop(const char* value, size_t len, bool isImportant);
    void removeBorderTop();

    String* BorderRight();
    void setBorderRight(const char* value, size_t len, bool isImportant);
    void removeBorderRight();

    String* BorderBottom();
    void setBorderBottom(const char* value, size_t len, bool isImportant);
    void removeBorderBottom();

    String* BorderLeft();
    void setBorderLeft(const char* value, size_t len, bool isImportant);
    void removeBorderLeft();

    String* BorderImage();
    void setBorderImage(const char* value, size_t len, bool isImportant);
    void removeBorderImage();

    String* BorderStyle(bool* isCombined = nullptr);
    void setBorderStyle(const char* value, size_t len, bool isImportant);
    void removeBorderStyle();

    String* BorderWidth(bool* isCombined = nullptr);
    void setBorderWidth(const char* value, size_t len, bool isImportant);
    void removeBorderWidth();

    String* BorderColor(bool* isCombined = nullptr);
    void setBorderColor(const char* value, size_t len, bool isImportant);
    void removeBorderColor();

    String* BorderRadius();
    void setBorderRadius(const char* value, size_t len, bool isImportant);
    void removeBorderRadius();

    String* Background();
    void setBackground(const char* value, size_t len, bool isImportant);
    void removeBackground();

    String* BackgroundRepeat();
    void setBackgroundRepeat(const char* value, size_t len, bool isImportant);
    void removeBackgroundRepeat();

    String* BackgroundPosition();
    void setBackgroundPosition(const char* value, size_t len, bool isImportant);
    void removeBackgroundPosition();

    String* TextDecoration();
    void setTextDecoration(const char* value, size_t len, bool isImportant);
    void removeTextDecoration();

    String* Margin(bool* isCombined = nullptr);
    void setMargin(const char* value, size_t len, bool isImportant);
    void removeMargin();

    String* MarginInline();
    void setMarginInline(const char* value, size_t len, bool isImportant);
    void removeMarginInline();

    String* Padding(bool* isCombined = nullptr);
    void setPadding(const char* value, size_t len, bool isImportant);
    void removePadding();

    String* PaddingInline();
    void setPaddingInline(const char* value, size_t len, bool isImportant);
    void removePaddingInline();

    String* Font();
    void setFont(const char* value, size_t len, bool isImportant);
    void removeFont();

    String* Outline();
    void setOutline(const char* value, size_t len, bool isImportant);
    void removeOutline();

    String* Overflow();
    void setOverflow(const char* value, size_t len, bool isImportant);
    void removeOverflow();

    String* Transition();
    void setTransition(const char* value, size_t len, bool isImportant);
    void removeTransition();

    String* Animation();
    void setAnimation(const char* value, size_t len, bool isImportant);
    void removeAnimation();

    String* FlexFlow();
    void setFlexFlow(const char* value, size_t len, bool isImportant);
    void removeFlexFlow();

    String* Flex();
    void setFlex(const char* value, size_t len, bool isImportant);
    void removeFlex();

    String* ListStyle();
    void setListStyle(const char* value, size_t len, bool isImportant);
    void removeListStyle();

    String* Mask();
    void setMask(const char* value, size_t len, bool isImportant);
    void removeMask();

    String* MaskPosition();
    void setMaskPosition(const char* value, size_t len, bool isImportant);
    void removeMaskPosition();

    String* MaskRepeat();
    void setMaskRepeat(const char* value, size_t len, bool isImportant);
    void removeMaskRepeat();

    // Named property getter/setter/remover for custom properties.
    String* customProperty(String* key);
    void removeCustomProperty(String* key);
    void setCustomProperty(AtomicString key, String* value);

    // Named property setter for sticky longhand properties.
    // These implementations are not common.
    void setD(const char* value, size_t len, bool isImportant);
    void setFontFamily(const char* value, size_t len, bool isImportant);
    void setSrc(const char* value, size_t len, bool isImportant);
    void setTransitionDelay(const char* value, size_t len, bool isImportant);
    void setTransitionDuration(const char* value, size_t len, bool isImportant);
    void setTransitionProperty(const char* value, size_t len, bool isImportant);
    void setTransitionTimingFunction(const char* value, size_t len,
                                     bool isImportant);
    void setAnimationName(const char* value, size_t len, bool isImportant);
    void setAnimationDuration(const char* value, size_t len, bool isImportant);
    void setAnimationTimingFunction(const char* value, size_t len,
                                    bool isImportant);
    void setAnimationDelay(const char* value, size_t len, bool isImportant);
    void setAnimationIterationCount(const char* value, size_t len,
                                    bool isImportant);
    void setAnimationDirection(const char* value, size_t len, bool isImportant);
    void setAnimationPlayState(const char* value, size_t len, bool isImportant);
    void setAnimationFillMode(const char* value, size_t len, bool isImportant);

    void setFourSidedShorthandProperty(
        CSSStyleValuePair::KeyKind fourSidedShorthand,
        const CSSStyleValuePair::KeyKind sides[4], const char* value,
        size_t length, bool isImportant);

    void appendCSSText(StringBuilder& txtBuilder, const size_t pos,
                       const char* cssName, String* value,
                       bool isImportant) const;
    String* cssTextAffectedByAllProperty(const size_t& pos) const;

    String* UnitPosition(CSSStyleValuePair::KeyKind keyKind);
    void setUnitPosition(const char* value, size_t len, bool isImportant,
                         CSSStyleValuePair::KeyKind keyKind);
    void removeUnitPosition(CSSStyleValuePair::KeyKind keyKind);

    String* UnitRepeatStyle(CSSStyleValuePair::KeyKind keyKind);
    void setUnitRepeatStyle(const char* value, size_t len, bool isImportant,
                            CSSStyleValuePair::KeyKind keyKind);
    void removeUnitRepeatStyle(CSSStyleValuePair::KeyKind keyKind);

    GCAtomicVector<CSSStyleValuePair> m_cssValues;
    Nullable<MutablePropertyValueList*> m_cssCustomValues;
    GCVector<void*> m_pointerRooter;
    Node* m_node;
};

class StyleRuleCSSStyleDeclaration : public CSSStyleDeclaration {
public:
    StyleRuleCSSStyleDeclaration(CSSStyleDeclaration* src, CSSRule* parentRule);

    virtual ScriptBindingInstance* scriptBindingInstance() override;

    virtual CSSRule* parentRule() const override
    {
        return m_parentRule;
    }

    CSSStyleSheet* parentStyleSheet() const;
    void setCssText(String* text) override;

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

    void layoutIfNeeds();
    void resolveStyleIfNeeds();
    void buildFrameTreeIfNeeds();

    void updateValue(CSSStyleValuePair::KeyKind keyKind) override;

private:
    enum class RequreidStyleResolveStage {
        kStayleResolution,
        kFrameTreeBuild,
        kLayout,
    };

    void triggerResolveComputedStyleIfNeeds(CSSStyleValuePair::KeyKind keyKind);

    RequreidStyleResolveStage requiredStage(CSSStyleValuePair::KeyKind keyKind);
};
} // namespace Starfish

#endif
