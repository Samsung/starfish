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

#ifndef __StarFishCSSStyleSheet__
#define __StarFishCSSStyleSheet__

#include "core/style/StyleSheet.h"

namespace StarFish {

class CSSRule;
class CSSStyleRule;
class CSSStyleRuleImport;
class MediaQueryEvaluator;
class MediaQuerySet;
class Node;
class URL;

class CSSStyleSheet : public StyleSheet {
public:
    CSSStyleSheet(Node* origin, String* str, CSSImportRule* ownerRule = nullptr)
        : StyleSheet()
    {
        m_sourceString = str;
        m_origin = origin;
        m_ownerRule = ownerRule;
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCSSStyleSheet() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    void addRule(CSSStyleRule* rule);
    void addRule(CSSRule* rule);

    ResourceURL* url();
    Node* origin()
    {
        return m_origin;
    }

    void parseSheetIfneeds();

    GCVector<CSSStyleRule*>& rules()
    {
        STARFISH_ASSERT(m_sourceString == String::emptyString);
        return m_rules;
    }

    GCVector<CSSRule*>& allRules()
    {
        STARFISH_ASSERT(m_sourceString == String::emptyString);
        return m_allRules;
    }

    CSSImportRule* ownerRule()
    {
        return m_ownerRule;
    }

    void clearOwnerRule()
    {
        m_ownerRule = nullptr;
    }

    CSSStyleSheet* parentStyleSheet();

    void sortRulesBySpecificity();

    bool matchesMediaQueries(const MediaQueryEvaluator& evaluator,
                             MediaQuerySet* mediaQueres);
    void collectRulesForImportedSheet();
    void collectRulesForSheet(GCVector<CSSRule*>& rules);

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

protected:
    // m_stringString != String::emptyString means we need to parse style sheet
    // before access style rules.
    String* m_sourceString;
    GCVector<CSSStyleRule*> m_rules;
    GCVector<CSSRule*> m_allRules;
    Node* m_origin;
    GCVector<CSSImportRule*> m_importRules;
    CSSImportRule* m_ownerRule;
};

} /* namespace StarFish */

#endif /* __StarFishCSSStyleSheet__ */
