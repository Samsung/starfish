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

#include "core/style/CSSRule.h"

namespace StarFish {

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
class StyleRuleCounterStyle;
class StyleRuleNamespace;
class StyleRuleKeyframes;

class StyleRuleBase : public gc {
public:
    StyleRuleBase(CSSRule::Type ruleType)
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

    CSSRule::Type type() const
    {
        return m_ruleType;
    }

    bool isStyleRule()
    {
        return type() == CSSRule::Type::STYLE_RULE;
    }

    bool isMediaRule()
    {
        return type() == CSSRule::Type::MEDIA_RULE;
    }

    bool isImportRule()
    {
        return type() == CSSRule::Type::IMPORT_RULE;
    }

    bool isCharsetRule()
    {
        return type() == CSSRule::Type::CHARSET_RULE;
    }

    bool isNamespaceRule()
    {
        return type() == CSSRule::Type::NAMESPACE_RULE;
    }

    bool isFontFaceRule()
    {
        return type() == CSSRule::Type::FONT_FACE_RULE;
    }

    bool isKeyframesRule()
    {
        return type() == CSSRule::Type::KEYFRAMES_RULE;
    }

    bool isKeyframeRule()
    {
        return type() == CSSRule::Type::KEYFRAME_RULE;
    }

    bool isCounterStyleRule()
    {
        return type() == CSSRule::Type::COUNTER_STYLE_RULE;
    }

    bool isSupportsRule()
    {
        return type() == CSSRule::Type::SUPPORTS_RULE;
    }

    bool isDocumentRule()
    {
        return type() == CSSRule::Type::DOCUMENT_RULE;
    }

    bool isFontFeatureValuesRule()
    {
        return type() == CSSRule::Type::FONT_FEATURE_VALUES_RULE;
    }

    bool isViewportRule()
    {
        return type() == CSSRule::Type::VIEWPORT_RULE;
    }

    bool isRegionStyleRule()
    {
        return type() == CSSRule::Type::REGION_STYLE_RULE;
    }

    inline StyleRule* asStyleRule();
    inline StyleRuleMedia* asStyleRuleMedia();
    inline StyleRuleImport* asStyleRuleImport();
    inline StyleRuleFontFace* asStyleRuleFontFace();
    inline StyleRuleSupports* asStyleRuleSupports();
    inline StyleRuleCounterStyle* asStyleRuleCounterStyle();
    inline StyleRuleNamespace* asStyleRuleNamespace();
    inline StyleRuleKeyframes* asStyleRuleKeyframes();

    CSSRule* createCSSOMWrapper(CSSStyleSheet* parent_sheet = 0) const;
    CSSRule* createCSSOMWrapper(CSSRule* parent_rule) const;

protected:
    CSSRule::Type m_ruleType;
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
    StyleRuleGroup(CSSRule::Type type, GCVector<StyleRuleBase*>& rules);
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
    StyleRuleCondition(CSSRule::Type, String* condition_text,
                       GCVector<StyleRuleBase*>& rules);
    StyleRuleCondition(CSSRule::Type, GCVector<StyleRuleBase*>& rules);
    StyleRuleCondition(StyleRuleCondition&);

    virtual String* conditionText() const
    {
        return m_conditionText;
    }

    virtual void setConditionText(String* conditionText)
    {
        m_conditionText = conditionText;
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
    StyleRuleSupports(String* conditionText, bool isSupported,
                      GCVector<StyleRuleBase*>& rules);
    StyleRuleSupports(StyleRuleSupports& o);

    bool isSupported()
    {
        return m_isSupported;
    }

    void setSupported(bool isSupported)
    {
        m_isSupported = isSupported;
    }

    bool eval(Document* doc, String* conditionText);

private:
    bool m_isSupported;
};

class StyleRuleCounterStyle : public StyleRuleBase {
    friend class StyleResolver;

public:
    StyleRuleCounterStyle(CSSStyleDeclaration* decl);
};

class StyleRuleNamespace : public StyleRuleBase {
public:
    StyleRuleNamespace(String* namespaceURI, String* prefix);

    String* namespaceURI() const
    {
        return m_namespaceURI;
    }

    String* prefix() const
    {
        return m_prefix;
    }

private:
    String* m_namespaceURI;
    String* m_prefix;
};

class StyleRuleKeyframes : public StyleRuleBase {
public:
    // TODO: Consider <keyframe-block-list>
    StyleRuleKeyframes(String* name);

    String* name() const
    {
        return m_name;
    }

    void setName(String* name)
    {
        m_name = name;
    }

private:
    String* m_name;
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

inline StyleRuleCounterStyle* StyleRuleBase::asStyleRuleCounterStyle()
{
    STARFISH_ASSERT(isCounterStyleRule());
    return (StyleRuleCounterStyle*)this;
}

inline StyleRuleNamespace* StyleRuleBase::asStyleRuleNamespace()
{
    STARFISH_ASSERT(isNamespaceRule());
    return (StyleRuleNamespace*)this;
}

inline StyleRuleKeyframes* StyleRuleBase::asStyleRuleKeyframes()
{
    STARFISH_ASSERT(isKeyframesRule());
    return (StyleRuleKeyframes*)this;
}
}

#endif
