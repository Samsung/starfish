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
