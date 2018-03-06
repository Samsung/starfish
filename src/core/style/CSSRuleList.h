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

#ifndef __StarFishCSSRuleList__
#define __StarFishCSSRuleList__

#include "binding/ScriptWrappable.h"
#include "core/dom/Node.h"

namespace StarFish {

class CSSRule;
class CSSStyleSheet;
class CSSGroupingRule;

class CSSRuleList : public ScriptWrappable {
public:
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCSSRuleList() const override;

    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
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

template <class Rule>
class LiveCSSRuleList : public CSSRuleList {
public:
    LiveCSSRuleList(Rule* rule)
        : m_rule(rule)
    {
    }

    ScriptBindingInstance* scriptBindingInstance() override
    {
        return styleSheet()->origin()->scriptBindingInstance();
    }

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

private:
    Rule* m_rule;
};
}

#endif
