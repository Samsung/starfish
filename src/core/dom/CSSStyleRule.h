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

#include "core/dom/CSSRule.h"
#include "core/style/Style.h"

namespace StarFish {

class CSSStyleDeclaration;
class CSSStyleRule : public CSSRule {
    friend class StyleResolver;

public:
    CSSStyleRule(CSSSelector::Type type, String* selectorText);

    CSSStyleRule(GCDeque<CSSSelector*>* selectorList,
                 CSSStyleDeclaration* decl);

    virtual void init(ScriptBindingInstance* instance) override;
    virtual bool isCSSStyleRule() const override;

    GCDeque<CSSSelector*>* selectorList()
    {
        return m_selectorList;
    }

    CSSStyleDeclaration* styleDeclaration()
    {
        return m_styleDeclaration;
    }

protected:
    GCDeque<CSSSelector*>* m_selectorList;
    CSSStyleDeclaration* m_styleDeclaration;
};

class CSSStyleRuleGroup : public CSSRule {
    friend class StyleResolver;

public:
    CSSStyleRuleGroup(RuleType type, GCVector<CSSRule*>& rules);
    CSSStyleRuleGroup(CSSStyleRuleGroup& o);

    GCVector<CSSRule*>& childRules()
    {
        return m_childRules;
    }

protected:
    GCVector<CSSRule*> m_childRules;
};

class MediaQuerySet;
class CSSStyleRuleMedia : public CSSStyleRuleGroup {
    friend class StyleResolver;

public:
    CSSStyleRuleMedia(MediaQuerySet* media, GCVector<CSSRule*>& rules);
    CSSStyleRuleMedia(CSSStyleRuleMedia& o);

    MediaQuerySet* mediaQuerySet()
    {
        return m_mediaQuerySet;
    }

protected:
    MediaQuerySet* m_mediaQuerySet;
};

class CSSStyleSheet;
class Document;
class TextResource;
class CSSStyleRuleImport : public CSSRule {
    friend class StyleResolver;
    friend class ImportedStyleSheetDownloadClient;

public:
    CSSStyleRuleImport(String* href, MediaQuerySet* media);

    MediaQuerySet* mediaQuerySet()
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
