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

#ifndef __StarFishStyleRule__
#define __StarFishStyleRule__

namespace StarFish {

class CSSRule;
class CSSSelector;
class CSSStyleSheet;
class CSSStyleDeclaration;
class Document;
class MediaQuerySet;
class TextResource;
class StyleRule;
class StyleRuleMedia;
class StyleRuleImport;
class StyleRuleFontFace;
class StyleRuleSupports;

class StyleRuleBase : public gc {
public:
    enum RuleType {
        STYLE_RULE = 1,
        CHARSET_RULE = 2,
        IMPORT_RULE = 3,
        MEDIA_RULE = 4,
        FONT_FACE_RULE = 5,
        PAGE_RULE = 6,
        KEYFRAMES_RULE = 7,
        KEYFRAME_RULE = 8,
        MARGIN_RULE = 9,
        NAMESPACE_RULE = 10,
        SUPPORTS_RULE = 12
    };

    StyleRuleBase(RuleType ruleType)
        : m_ruleType(ruleType)
    {
    }

    StyleRuleBase(StyleRuleBase& o)
        : m_ruleType(o.type())
    {
    }

    virtual ~StyleRuleBase()
    {
    }

    RuleType type() const
    {
        return m_ruleType;
    }

    bool isStyleRule()
    {
        return type() == RuleType::STYLE_RULE;
    }

    bool isMediaRule()
    {
        return type() == RuleType::MEDIA_RULE;
    }

    bool isImportRule()
    {
        return type() == RuleType::IMPORT_RULE;
    }

    bool isCharsetRule()
    {
        return type() == RuleType::CHARSET_RULE;
    }

    bool isNamespaceRule()
    {
        return type() == RuleType::NAMESPACE_RULE;
    }

    bool isFontFaceRule()
    {
        return type() == RuleType::FONT_FACE_RULE;
    }

    bool isKeyframesRule()
    {
        return type() == RuleType::KEYFRAMES_RULE;
    }

    bool isKeyframeRule()
    {
        return type() == RuleType::KEYFRAME_RULE;
    }

    bool isSupportsRule()
    {
        return type() == RuleType::SUPPORTS_RULE;
    }

    inline StyleRule* asStyleRule();
    inline StyleRuleMedia* asStyleRuleMedia();
    inline StyleRuleImport* asStyleRuleImport();
    inline StyleRuleFontFace* asStyleRuleFontFace();
    inline StyleRuleSupports* asStyleRuleSupports();

    CSSRule* createCSSOMWrapper(CSSStyleSheet* parent_sheet = 0) const;
    CSSRule* createCSSOMWrapper(CSSRule* parent_rule) const;

protected:
    RuleType m_ruleType;
    CSSRule* createCSSOMWrapper(CSSStyleSheet* parentSheet,
                                CSSRule* parentRule) const;
};

class StyleRule : public StyleRuleBase {
    friend class StyleResolver;
    friend class CSSStyleSheet;
    friend class AncestorSelectorFilter;

public:
    static const unsigned maximumIdentifierCount = 10;
    StyleRule(CSSSelectorList&& selectorList, CSSStyleDeclaration* decl);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    CSSSelectorList& selectorList()
    {
        return m_selectorList;
    }

    CSSStyleDeclaration* styleDeclaration()
    {
        return m_styleDeclaration;
    }

    size_t order()
    {
        return m_order;
    }

    void setOrder(size_t o)
    {
        m_order = o;
    }

    bool isUARule()
    {
        return m_isUARule;
    }

    void setIsUARule(bool b)
    {
        m_isUARule = b;
    }

    bool isSimpleIDSelector()
    {
        return m_isSimpleIDSelector;
    }

    bool isSimpleClassSelector()
    {
        return m_isSimpleClassSelector;
    }

    bool isSimpleTagSelector()
    {
        return m_isSimpleTagSelector;
    }

    void initFlagsRelatedWithSelectorList();
    void wrapperTakeSelectorList(CSSSelectorList& selectors);

protected:
    CSSSelectorList m_selectorList;
    CSSStyleDeclaration* m_styleDeclaration;
    size_t m_order;
    bool m_isUARule : 1;
    bool m_isSimpleIDSelector : 1;
    bool m_isSimpleClassSelector : 1;
    bool m_isSimpleTagSelector : 1;

    unsigned m_identifierHashes[maximumIdentifierCount];
};

class StyleRuleGroup : public StyleRuleBase {
    friend class StyleResolver;

public:
    StyleRuleGroup(RuleType type, GCVector<StyleRuleBase*>& rules);
    StyleRuleGroup(StyleRuleGroup& o);

    GCVector<StyleRuleBase*>& childRules()
    {
        return m_childRules;
    }

    void wrapperInsertRule(unsigned index, StyleRuleBase* rule);
    void wrapperRemoveRule(unsigned index);

protected:
    GCVector<StyleRuleBase*> m_childRules;
};

class StyleRuleCondition : public StyleRuleGroup {
public:
    StyleRuleCondition(RuleType, String* condition_text,
                       GCVector<StyleRuleBase*>& rules);
    StyleRuleCondition(RuleType, GCVector<StyleRuleBase*>& rules);
    StyleRuleCondition(StyleRuleCondition&);

    String* conditionText() const
    {
        return m_conditionText;
    }

protected:
    String* m_conditionText;
};

class StyleRuleMedia : public StyleRuleCondition {
    friend class StyleResolver;

public:
    StyleRuleMedia(MediaQuerySet* media, GCVector<StyleRuleBase*>& rules);
    StyleRuleMedia(StyleRuleMedia& o);

    MediaQuerySet* mediaQuerySet() const
    {
        return m_mediaQuerySet;
    }

protected:
    MediaQuerySet* m_mediaQuerySet;
};

class StyleRuleImport : public StyleRuleBase {
    friend class StyleResolver;
    friend class ImportedStyleSheetDownloadClient;

public:
    StyleRuleImport(String* href, MediaQuerySet* media);

    MediaQuerySet* mediaQuerySet() const
    {
        return m_mediaQuerySet;
    }

    CSSStyleSheet* parentStyleSheet()
    {
        return m_parentStyleSheet;
    }

    void setParentStyleSheet(CSSStyleSheet* sheet)
    {
        m_parentStyleSheet = sheet;
    }

    void clearParentStyleSheet()
    {
        m_parentStyleSheet = nullptr;
    }

    String* href() const
    {
        return m_strHref;
    }

    CSSStyleSheet* styleSheet()
    {
        return m_generatedSheet;
    }

    void setStyleSheet(String* href, ResourceURL* baseURL, String* charset,
                       CSSStyleSheet* styleSheet);

    Document* document();
    void requestStyleSheet();
    bool isLoading() const;
    void unloadStyleSheetIfExists();

protected:
    void willStyleSheetLoad();
    void didStyleSheetLoadComplete();

    String* m_strHref;
    MediaQuerySet* m_mediaQuerySet;
    CSSStyleSheet* m_generatedSheet;
    TextResource* m_styleSheetTextResource;
    CSSStyleSheet* m_parentStyleSheet;
    bool m_loading;
};

class StyleRuleFontFace : public StyleRuleBase {
    friend class StyleResolver;

public:
    StyleRuleFontFace(CSSStyleDeclaration* decl);
    CSSStyleDeclaration* styleDeclaration()
    {
        return m_styleDeclaration;
    }

protected:
    CSSStyleDeclaration* m_styleDeclaration;
};

class StyleRuleSupports : public StyleRuleCondition {
    friend class StyleResolver;

public:
    StyleRuleSupports(String* conditionText, bool conditionIsSupported,
                      GCVector<StyleRuleBase*>& rules);
    StyleRuleSupports(StyleRuleSupports& o);
    bool conditionIsSupported() const
    {
        return m_conditionIsSupported;
    }

private:
    String* m_conditionText;
    bool m_conditionIsSupported;
};

inline StyleRule* StyleRuleBase::asStyleRule()
{
    STARFISH_ASSERT(isStyleRule());
    return (StyleRule*)this;
}

inline StyleRuleMedia* StyleRuleBase::asStyleRuleMedia()
{
    STARFISH_ASSERT(isMediaRule());
    return (StyleRuleMedia*)this;
}

inline StyleRuleImport* StyleRuleBase::asStyleRuleImport()
{
    STARFISH_ASSERT(isImportRule());
    return (StyleRuleImport*)this;
}

inline StyleRuleFontFace* StyleRuleBase::asStyleRuleFontFace()
{
    STARFISH_ASSERT(isFontFaceRule());
    return (StyleRuleFontFace*)this;
}

inline StyleRuleSupports* StyleRuleBase::asStyleRuleSupports()
{
    STARFISH_ASSERT(isSupportsRule());
    return (StyleRuleSupports*)this;
}
}

#endif
