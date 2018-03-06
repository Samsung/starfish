/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishHTMLStyleElement__
#define __StarFishHTMLStyleElement__

#include "core/dom/HTMLElement.h"

namespace StarFish {

class HTMLStyleElement : public HTMLElement {
public:
    HTMLStyleElement(Document* document)
        : HTMLElement(document)
        , m_generatedSheet(nullptr)
        , m_loaded(false)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLStyleElement() const override;

    /* 4.4 Interface Node */
    virtual QualifiedName name();

    /* Other methods (not in DOM API) */

    String* type();
    void setType(String* type);

    String* media();
    void setMedia(String* media);

    StyleSheet* sheet();

    virtual void didCharacterDataModified(String* before, String* after);
    virtual void didNodeInsertedToDocumentTree();
    virtual void didNodeRemovedFromDocumentTree();
    virtual void didNodeInserted(Node* parent, Node* newChild);
    virtual void didNodeRemoved(Node* parent, Node* oldChild);
    virtual void finishParsing()
    {
        HTMLElement::finishParsing();
        if (isInDocumentScopeAndDocumentParticipateInRendering()) {
            generateStyleSheet();
        }
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
