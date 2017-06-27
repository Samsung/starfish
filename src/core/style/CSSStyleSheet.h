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

namespace StarFish {

// class CSSRule;
class CSSRuleList;
// class CSSStyleRule;
// class CSSStyleRuleImport;
class StyleRuleBase;
class StyleRule;
class StyleRuleImport;

class MediaQueryEvaluator;
class MediaQuerySet;
class Node;
class URL;

class CSSStyleSheet : public StyleSheet {
public:
    CSSStyleSheet(Node* origin, String* str,
                  StyleRuleImport* ownerRule = nullptr)
        : StyleSheet()
        , m_sourceString(str)
        , m_origin(origin)
        , m_ownerRule(ownerRule)
        , m_ruleList(nullptr)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCSSStyleSheet() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    void addStyleRule(std::pair<StyleRule*, ResourceURL*> rule);
    void addRule(StyleRuleBase* rule);

    ResourceURL* url();
    Node* origin()
    {
        return m_origin;
    }

    void parseSheetIfneeds();

    GCVector<std::pair<StyleRule*, ResourceURL*>>& rules()
    {
        return m_styleRules;
    }

    GCVector<StyleRuleBase*>& allRules()
    {
        return m_childRules;
    }

    StyleRuleImport* ownerRule()
    {
        return m_ownerRule;
    }

    void clearOwnerRule()
    {
        m_ownerRule = nullptr;
    }

    CSSStyleSheet* parentStyleSheet();

    void sortStyleRulesBySpecificity();

    bool matchesMediaQueries(const MediaQueryEvaluator& evaluator,
                             MediaQuerySet* mediaQueres);
    void collectRulesForImportedSheet();
    void collectStyleRules(GCVector<StyleRuleBase*>& rules, ResourceURL* url);

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

    CSSRuleList* cssRules();
    unsigned insertRule(String* ruleString, unsigned index);
    void deleteRule(unsigned index);

protected:
    // m_stringString != String::emptyString means we need to parse style sheet
    // before access style rules.
    String* m_sourceString;
    GCVector<std::pair<StyleRule*, ResourceURL*>> m_styleRules;
    GCVector<StyleRuleBase*> m_childRules;
    Node* m_origin;
    GCVector<StyleRuleImport*> m_importRules;
    StyleRuleImport* m_ownerRule;
    CSSRuleList* m_ruleList;

    GCVector<CSSRule*> m_childRuleWrappers;
};

} /* namespace StarFish */

#endif /* __StarFishCSSStyleSheet__ */
