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

#ifndef __StarFishCSSStyleSheet__
#define __StarFishCSSStyleSheet__

#include "core/style/StyleSheet.h"
#include "core/style/MediaQueryEvaluator.h"

namespace StarFish {

class CSSImportRule;
class CSSRule;
class CSSRuleList;
class MediaList;
class MediaQueryEvaluator;
class MediaQuerySet;
class Node;
class StyleRule;
class StyleRuleBase;
class StyleRuleImport;
class URL;

class RuleSet : public gc {
public:
    GCUnorderedMultiMap<AtomicString, std::pair<StyleRule*, ResourceURL*>>&
    idRules()
    {
        return m_idRules;
    }

    GCUnorderedMultiMap<AtomicString, std::pair<StyleRule*, ResourceURL*>>&
    classRules()
    {
        return m_classRules;
    }

    GCUnorderedMultiMap<AtomicString, std::pair<StyleRule*, ResourceURL*>>&
    tagRules()
    {
        return m_tagRules;
    }

    GCUnorderedMultiMap<AtomicString, std::pair<StyleRule*, ResourceURL*>>&
    universalRules()
    {
        return m_universalRules;
    }

    void clear()
    {
        m_idRules.clear();
        m_classRules.clear();
        m_tagRules.clear();
        m_universalRules.clear();
    }

private:
    GCUnorderedMultiMap<AtomicString, std::pair<StyleRule*, ResourceURL*>>
        m_idRules;
    GCUnorderedMultiMap<AtomicString, std::pair<StyleRule*, ResourceURL*>>
        m_classRules;
    GCUnorderedMultiMap<AtomicString, std::pair<StyleRule*, ResourceURL*>>
        m_tagRules;
    GCUnorderedMultiMap<AtomicString, std::pair<StyleRule*, ResourceURL*>>
        m_universalRules;
};

class CSSStyleSheet : public StyleSheet {
public:
    CSSStyleSheet(Node* origin, String* str);
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCSSStyleSheet() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    void addRule(StyleRuleBase* rule);

    ResourceURL* url();
    Node* origin()
    {
        return m_origin;
    }

    void parseSheetIfneeds();

    GCVector<std::pair<StyleRule*, ResourceURL*>>& styleRules()
    {
        return m_styleRules;
    }

    GCVector<StyleRuleImport*>& importRules()
    {
        return m_importRules;
    }

    GCVector<StyleRuleBase*>& childRules()
    {
        return m_childRules;
    }

    CSSRule* ownerRule()
    {
        return m_ownerRule;
    }

    void clearOwnerRule()
    {
        m_ownerRule = nullptr;
    }

    void clearStyleRules()
    {
        m_styleRules.clear();
        m_styleRules.shrink_to_fit();
    }

    void setOwnerRule(CSSRule* ownerRule);
    CSSStyleSheet* parentStyleSheet() const;
    void sortStyleRulesBySpecificity();
    bool matchesMediaQueries(const MediaQueryEvaluator& evaluator,
                             MediaQuerySet* mediaQueres,
                             MediaQueryResultList* viewportDependentResult,
                             MediaQueryResultList* deviceDependentResult);
    void collectRulesFromImportedSheet(
        GCVector<StyleRuleImport*>& rules,
        std::vector<CSSStyleDeclaration*>& webFonts,
        MediaQueryResultList* viewportDependentResult,
        MediaQueryResultList* deviceDependentResult);
    void collectStyleRules(
        GCVector<StyleRuleBase*>& rules,
        std::vector<CSSStyleDeclaration*>& webFonts, ResourceURL* url,
        MediaQueryResultList* viewportDependentResult = nullptr,
        MediaQueryResultList* deviceDependentResult = nullptr);

    /* DOM APIs */
    String* type() const override
    {
        return String::fromUTF8("text/css");
    }

    String* href() const override;

    Node* ownerNode() const override
    {
        return m_origin;
    }

    unsigned length() const;
    CSSRule* item(unsigned index);
    StyleRuleBase* ruleAt(unsigned index) const;

    bool wrapperInsertRule(StyleRuleBase* rule, unsigned index);
    bool wrapperDeleteRule(unsigned index);

    MediaQuerySet* mediaQuerySet()
    {
        return m_mediaQuerySet;
    }

    void setMediaQuerySet(MediaQuerySet* mediaQuerySet);
    MediaList* media();

    CSSRuleList* cssRules();
    CSSRuleList* rules()
    {
        return cssRules();
    }
    unsigned insertRule(String* ruleString, unsigned index);
    void deleteRule(unsigned index);

protected:
    // m_stringString != String::emptyString means we need to parse style sheet
    // before access style rules.
    String* m_sourceString;
    Node* m_origin;
    CSSRule* m_ownerRule;
    CSSRuleList* m_ruleList;
    MediaQuerySet* m_mediaQuerySet;
    MediaList* m_mediaWrapper;

    GCVector<StyleRuleBase*> m_childRules;
    GCVector<StyleRuleImport*> m_importRules;
    GCVector<std::pair<StyleRule*, ResourceURL*>> m_styleRules;
    GCVector<CSSRule*> m_childRuleWrappers;
};

} /* namespace StarFish */

#endif /* __StarFishCSSStyleSheet__ */
