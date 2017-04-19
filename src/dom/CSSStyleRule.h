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

#ifndef __StarFishCSSStyleRule__
#define __StarFishCSSStyleRule__

#include "style/Style.h"

namespace StarFish {

class CSSStyleDeclaration;
class CSSStyleRule : public ScriptWrappable {
    friend class StyleResolver;

public:
    CSSStyleRule(CSSSelector::Type type, String* selectorText,
                 Document* document);

    CSSStyleRule(GCDeque<CSSSelector*>* selectorList, Document* document,
                 CSSStyleDeclaration* decl);

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual bool isCSSStyleRule() const override
    {
        return true;
    }

    GCDeque<CSSSelector*>* selectorList()
    {
        return m_selectorList;
    }

    CSSStyleDeclaration* styleDeclaration()
    {
        return m_styleDeclaration;
    }

    Document* document()
    {
        return m_document;
    }

protected:
    GCDeque<CSSSelector*>* m_selectorList;
    CSSStyleDeclaration* m_styleDeclaration;
    Document* m_document;
};
}

#endif
