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

#ifndef __StarFishHTMLStyleElement__
#define __StarFishHTMLStyleElement__

#include "dom/HTMLElement.h"

namespace StarFish {

class HTMLStyleElement : public HTMLElement {
public:
    HTMLStyleElement(Document* document)
        : HTMLElement(document)
        , m_generatedSheet(nullptr)
        , m_loaded(false)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */

    virtual String* localName()
    {
        return document()->window()->starFish()->staticStrings()->m_styleTagName.localName();
    }

    virtual QualifiedName name()
    {
        return document()->window()->starFish()->staticStrings()->m_styleTagName;
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLStyleElement() const
    {
        return true;
    }

    virtual void didCharacterDataModified(String* before, String* after);
    virtual void didNodeInsertedToDocumenTree();
    virtual void didNodeRemovedFromDocumenTree();
    virtual void didNodeInserted(Node* parent, Node* newChild);
    virtual void didNodeRemoved(Node* parent, Node* oldChild);
    virtual void finishParsing()
    {
        HTMLElement::finishParsing();
        if (isInDocumentScopeAndDocumentParticipateInRendering())
            generateStyleSheet();
    }

    void generateStyleSheet();
    void removeStyleSheet();
    CSSStyleSheet* generatedSheet()
    {
        return m_generatedSheet;
    }

    bool hasLoaded()
    {
        return m_loaded;
    }

    void setLoaded()
    {
        m_loaded = true;
    }

private:
    void dispatchLoadEvent();

protected:
    CSSStyleSheet* m_generatedSheet;
    bool m_loaded;
};

}

#endif
