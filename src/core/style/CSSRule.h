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

class CSSStyleRule;
class CSSMediaRule;
class CSSImportRule;

class CSSRule : public ScriptWrappable {
public:
    enum RuleType {
        STYLE_RULE = 1,
        CHARSET_RULE = 2,
        IMPORT_RULE = 3,
        MEDIA_RULE = 4,
        FONT_FACE_RULE = 5,
        PAGE_RULE = 6,
        MARGIN_RULE = 9,
        NAMESPACE_RULE = 10
    };

    CSSRule(RuleType ruleType)
        : ScriptWrappable(this)
        , m_ruleType(ruleType)
    {
    }

    CSSRule(CSSRule& o)
        : ScriptWrappable(this)
        , m_ruleType(o.type())
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCSSRule() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
        // TODO child classes must override this function
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    RuleType type()
    {
        return m_ruleType;
    }

    virtual bool isStyleRule()
    {
        return type() == RuleType::STYLE_RULE;
    }

    virtual bool isMediaRule()
    {
        return type() == RuleType::MEDIA_RULE;
    }

    virtual bool isImportRule()
    {
        return type() == RuleType::IMPORT_RULE;
    }

    virtual bool isCharsetRule()
    {
        return type() == RuleType::CHARSET_RULE;
    }

    virtual bool isNamespaceRule()
    {
        return type() == RuleType::NAMESPACE_RULE;
    }

    CSSStyleRule* asCSSStyleRule()
    {
        STARFISH_ASSERT(isStyleRule());
        return (CSSStyleRule*)this;
    }

    CSSMediaRule* asCSSStyleRuleMedia()
    {
        STARFISH_ASSERT(isMediaRule());
        return (CSSMediaRule*)this;
    }

    CSSImportRule* asCSSStyleRuleImport()
    {
        STARFISH_ASSERT(isImportRule());
        return (CSSImportRule*)this;
    }

    virtual String* cssText() const = 0;
    void setCssText(String*)
    {
    }

private:
    RuleType m_ruleType;
};
}

#endif
