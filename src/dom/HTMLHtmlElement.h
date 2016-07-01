/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#ifndef __StarFishHTMLHtmlElement__
#define __StarFishHTMLHtmlElement__

#include "dom/HTMLElement.h"

namespace StarFish {

class HTMLHtmlElement : public HTMLElement {
public:
    HTMLHtmlElement(Document* document)
        : HTMLElement(document)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */

    virtual String* localName()
    {
        return document()->window()->starFish()->staticStrings()->m_htmlTagName.localName();
    }

    virtual QualifiedName name()
    {
        return document()->window()->starFish()->staticStrings()->m_htmlTagName;
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLHtmlElement() const
    {
        return true;
    }

    virtual void didComputedStyleChanged(ComputedStyle* oldStyle, ComputedStyle* newStyle)
    {
        HTMLElement::didComputedStyleChanged(oldStyle, newStyle);
        if (!newStyle->backgroundColor().isTransparent() || newStyle->backgroundImageData()) {
            document()->window()->m_hasRootElementBackground = true;
        } else {
            document()->window()->m_hasRootElementBackground = false;
        }

        if (oldStyle && oldStyle->overflow() != newStyle->overflow()) {
            document()->setNeedsFrameTreeBuild();
        }
    }

    HTMLBodyElement* body()
    {
        // root element of html document is HTMLHtmlElement
        // https://www.w3.org/TR/html-markup/html.html
        Node* n = firstChild();
        while (n) {
            if (n->isElement() && n->asElement() && n->asElement()->isHTMLElement() && n->asElement()->asHTMLElement()->isHTMLBodyElement()) {
                return n->asElement()->asHTMLElement()->asHTMLBodyElement();
            }
            n = n->nextSibling();
        }

        return nullptr;
    }

protected:
};

}

#endif
