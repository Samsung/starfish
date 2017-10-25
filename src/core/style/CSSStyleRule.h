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

#ifndef __StarFishCSSStyleRule__
#define __StarFishCSSStyleRule__

#include "core/style/CSSRule.h"

namespace StarFish {

class CSSRuleList;
class CSSStyleSheet;
class CSSStyleDeclaration;
class StyleRule;
class StyleRuleGroup;
class StyleRuleCondition;
class StyleRuleMedia;
class StyleRuleImport;
class StyleRuleFontFace;
class MediaQuerySet;
class MediaList;

class CSSStyleRule : public CSSRule {
public:
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCSSStyleRule() const override;

    CSSStyleRule(StyleRule* styleRule, CSSStyleSheet* parent);
    String* cssText() override;
    String* selectorText() const;
    void setSelectorText(String* selectorText);
    StyleRule* styleRule() const
    {
        return m_styleRule;
    }

    CSSStyleDeclaration* style();

private:
    CSSRule::Type type() const override
    {
        return CSSRule::Type::STYLE_RULE;
    }
    String* generateSelectorText() const;

    StyleRule* m_styleRule;
    CSSStyleDeclaration* m_propertiesWrapper;
};

class CSSGroupingRule : public CSSRule {
public:
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCSSGroupingRule() const override;

    CSSGroupingRule(StyleRuleGroup* group_rule, CSSStyleSheet* parent);

    CSSRuleList* cssRules() override;
    unsigned insertRule(String* rule, unsigned index);
    void deleteRule(unsigned index);

    // For CSSRuleList
    unsigned length() const;
    CSSRule* item(unsigned index);

protected:
    void appendCSSTextForItems(StringBuilder& result);

    StyleRuleGroup* m_groupRule;
    GCVector<CSSRule*> m_childRuleWrappers;
    CSSRuleList* m_ruleListWrapper;
};

class CSSConditionRule : public CSSGroupingRule {
public:
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCSSConditionRule() const override;

    CSSConditionRule(StyleRuleCondition* condition_rule, CSSStyleSheet* parent);

    virtual String* conditionText() const;
    virtual void setConditionText(String* text)
    {
        return;
    }
};

class CSSMediaRule : public CSSConditionRule {
public:
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCSSMediaRule() const override;

    CSSMediaRule(StyleRuleMedia*, CSSStyleSheet*);

    String* cssText() override;
    String* conditionText() const;
    MediaList* media();

private:
    CSSRule::Type type() const override
    {
        return CSSRule::Type::MEDIA_RULE;
    }
    MediaQuerySet* mediaQuerySet() const;
    MediaList* m_mediaWrapper;
};

class CSSImportRule : public CSSRule {
public:
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCSSImportRule() const override;

    CSSImportRule(StyleRuleImport*, CSSStyleSheet*);

    String* cssText() override;
    String* href() const;
    CSSStyleSheet* styleSheet();
    MediaList* media();

private:
    CSSRule::Type type() const override
    {
        return CSSRule::Type::IMPORT_RULE;
    }
    StyleRuleImport* m_importRule;
    CSSStyleSheet* m_styleSheetWrapper;
    MediaList* m_mediaWrapper;
};

class CSSFontFaceRule : public CSSRule {
public:
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCSSFontFaceRule() const override;

    CSSFontFaceRule(StyleRuleFontFace* styleRule, CSSStyleSheet* parent);
    String* cssText() override;
    StyleRuleFontFace* styleRule() const
    {
        return m_styleRule;
    }

    CSSStyleDeclaration* style();

private:
    CSSRule::Type type() const override
    {
        return CSSRule::Type::FONT_FACE_RULE;
    }

    StyleRuleFontFace* m_styleRule;
    CSSStyleDeclaration* m_propertiesWrapper;
};
}

#endif
