/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2006, 2007, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Andreas Kling (kling@webkit.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
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
        // UNKNOWN_RULE = 0 // Obsolete
        STYLE_RULE = 1,
        CHARSET_RULE = 2, // Obsolete: removed in most browsers
        IMPORT_RULE = 3,
        MEDIA_RULE = 4,
        FONT_FACE_RULE = 5,
        PAGE_RULE = 6,
        KEYFRAMES_RULE = 7,
        KEYFRAME_RULE = 8,
        MARGIN_RULE = 9,
        NAMESPACE_RULE = 10,
        COUNTER_STYLE_RULE = 11,
        SUPPORTS_RULE = 12,
        DOCUMENT_RULE = 13,
        FONT_FEATURE_VALUES_RULE = 14,
        VIEWPORT_RULE = 15,
        REGION_STYLE_RULE = 16,
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
