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

#ifndef __StarFishCSSRuleList__
#define __StarFishCSSRuleList__

#include "core/style/CSSStyleRule.h"

namespace StarFish {

class CSSRuleList : public ScriptWrappable {
public:
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCSSRuleList() const override;

    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
        // TODO child classes must override this function
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual unsigned length() const = 0;
    virtual CSSRule* item(unsigned index) const = 0;
    virtual CSSStyleSheet* styleSheet() const = 0;

protected:
    CSSRuleList()
        : ScriptWrappable(this)
    {
    }
};

class StaticCSSRuleList : public CSSRuleList {
public:
    StaticCSSRuleList()
    {
    }

    GCVector<CSSRule*>& rules()
    {
        return m_rules;
    }

    CSSStyleSheet* styleSheet() const override
    {
        return 0;
    }

private:
    unsigned length() const override
    {
        return m_rules.size();
    }

    CSSRule* item(unsigned index) const override
    {
        return index < m_rules.size() ? m_rules[index] : nullptr;
    }

    GCVector<CSSRule*> m_rules;
};

template <class Rule>
class LiveCSSRuleList : public CSSRuleList {
public:
    LiveCSSRuleList(Rule* rule)
        : m_rule(rule)
    {
    }

private:
    unsigned length() const override
    {
        return m_rule->length();
    }

    CSSRule* item(unsigned index) const override
    {
        return m_rule->item(index);
    }

    CSSStyleSheet* styleSheet() const override
    {
        return m_rule->parentStyleSheet();
    }

    Rule* m_rule;
};
}

#endif
