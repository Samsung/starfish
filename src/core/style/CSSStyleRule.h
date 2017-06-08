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

#ifndef __StarFishCSSStyleRule__
#define __StarFishCSSStyleRule__

#include "core/style/CSSRule.h"
#include "core/style/Style.h"

namespace StarFish {

class CSSStyleDeclaration;
class CSSStyleRule : public CSSRule {
    friend class StyleResolver;

public:
    CSSStyleRule(CSSSelector::Type type, AtomicString selectorText);

    CSSStyleRule(GCDeque<CSSSelector*>* selectorList,
                 CSSStyleDeclaration* decl);

    virtual void init(ScriptBindingInstance* instance) override;
    virtual bool isCSSStyleRule() const override;

    GCDeque<CSSSelector*>* selectorList() const
    {
        return m_selectorList;
    }

    CSSStyleDeclaration* styleDeclaration()
    {
        return m_styleDeclaration;
    }

    String* selectorText() const;
    void setSelectorText(String* selectorText)
    {
    }

    String* cssText() const override;

protected:
    GCDeque<CSSSelector*>* m_selectorList;
    CSSStyleDeclaration* m_styleDeclaration;
};

class CSSGroupingRule : public CSSRule {
    friend class StyleResolver;

public:
    CSSGroupingRule(RuleType type, GCVector<CSSRule*>& rules);
    CSSGroupingRule(CSSGroupingRule& o);

    virtual void init(ScriptBindingInstance* instance) override;
    virtual bool isCSSGroupingRule() const override;

    GCVector<CSSRule*>& childRules()
    {
        return m_childRules;
    }

    unsigned length() const;

protected:
    GCVector<CSSRule*> m_childRules;

    void appendCSSTextForItems(String* result) const;
};

class CSSConditionRule : public CSSGroupingRule {
public:
    CSSConditionRule(RuleType, String* condition_text,
                     GCVector<CSSRule*>& rules);
    CSSConditionRule(RuleType, GCVector<CSSRule*>& rules);
    CSSConditionRule(CSSConditionRule&);

    virtual void init(ScriptBindingInstance* instance) override;
    virtual bool isCSSConditionRule() const override;

    String* ConditionText() const
    {
        return m_conditionText;
    }

protected:
    String* m_conditionText;
};

class MediaQuerySet;
class CSSMediaRule : public CSSConditionRule {
    friend class StyleResolver;

public:
    CSSMediaRule(MediaQuerySet* media, GCVector<CSSRule*>& rules);
    CSSMediaRule(CSSMediaRule& o);

    virtual void init(ScriptBindingInstance* instance) override;
    virtual bool isCSSMediaRule() const override;

    MediaQuerySet* mediaQuerySet() const
    {
        return m_mediaQuerySet;
    }

    String* cssText() const override;

protected:
    MediaQuerySet* m_mediaQuerySet;
};

class CSSStyleSheet;
class Document;
class TextResource;
class CSSImportRule : public CSSRule {
    friend class StyleResolver;
    friend class ImportedStyleSheetDownloadClient;

public:
    CSSImportRule(String* href, MediaQuerySet* media);

    virtual void init(ScriptBindingInstance* instance) override;
    virtual bool isCSSImportRule() const override;

    MediaQuerySet* mediaQuerySet() const
    {
        return m_mediaQuerySet;
    }

    void setParentStyleSheet(CSSStyleSheet* sheet)
    {
        STARFISH_ASSERT(sheet);
        m_parentStyleSheet = sheet;
    }

    CSSStyleSheet* parentStyleSheet()
    {
        return m_parentStyleSheet;
    }

    Document* document();
    void requestStyleSheet();
    void unloadStyleSheetIfExists();

    String* cssText() const override;

protected:
    void willStyleSheetLoad();
    void didStyleSheetLoadComplete();

    String* m_strHref;
    MediaQuerySet* m_mediaQuerySet;
    CSSStyleSheet* m_parentStyleSheet;
    CSSStyleSheet* m_generatedSheet;
    TextResource* m_styleSheetTextResource;
};
}

#endif
