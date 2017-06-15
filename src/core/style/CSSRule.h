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

#ifndef __StarFishCSSRule__
#define __StarFishCSSRule__

#include "binding/ScriptWrappable.h"

namespace StarFish {

class CSSStyleSheet;
class CSSRuleList;

class CSSRule : public ScriptWrappable {
public:
    enum Type {
        STYLE_RULE = 1,
        CHARSET_RULE = 2,
        IMPORT_RULE = 3,
        MEDIA_RULE = 4,
        FONT_FACE_RULE = 5,
        PAGE_RULE = 6,
        MARGIN_RULE = 9,
        NAMESPACE_RULE = 10
    };

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCSSRule() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    virtual Type type() const = 0;
    virtual String* cssText() = 0;
    virtual CSSRuleList* cssRules()
    {
        return 0;
    }

    void setParentStyleSheet(CSSStyleSheet* styleSheet)
    {
        m_parentIsRule = false;
        m_parentStyleSheet = styleSheet;
    }

    void setParentRule(CSSRule* rule)
    {
        m_parentIsRule = true;
        m_parentRule = rule;
    }

    CSSStyleSheet* parentStyleSheet() const
    {
        if (m_parentIsRule) {
            return m_parentRule ? m_parentRule->parentStyleSheet() : nullptr;
        }
        return m_parentStyleSheet;
    }

    CSSRule* parentRule() const
    {
        return m_parentIsRule ? m_parentRule : nullptr;
    }

    // The CSSOM spec states that "setting the cssText attribute must do
    // nothing."
    void setCssText(String*)
    {
    }

protected:
    CSSRule(CSSStyleSheet* parent);

private:
    unsigned char m_parentIsRule : 1;

    // These should be Members, but no Members in unions.
    union {
        CSSRule* m_parentRule;
        CSSStyleSheet* m_parentStyleSheet;
    };
};
}

#endif
