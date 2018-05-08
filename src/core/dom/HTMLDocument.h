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

#ifndef __StarFishHTMLDocument__
#define __StarFishHTMLDocument__

#include "core/dom/Document.h"

namespace StarFish {

class Window;

class HTMLDocument : public Document {
public:
    HTMLDocument(Window* window, ScriptBindingInstance* scriptBindingInstance,
                 ResourceURL* url, String* charSet,
                 bool doesParticipateInRendering)
        : Document(window, scriptBindingInstance, url, charSet,
                   doesParticipateInRendering)
    {
        m_contentType = String::createASCIIString("text/html");
    }

    HTMLDocument(HTMLDocument& doc)
        : Document(doc.m_window, doc.scriptBindingInstance(), doc.m_documentURI,
                   doc.m_characterSet, false)
    {
        m_contentType = String::createASCIIString("text/html");
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    Node* clone() override
    {
        return new HTMLDocument(*this);
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLDocument() const override;

    static Element* createHTMLElement(Document* document,
                                      AtomicString localName);

    static bool isCaseSensitiveAttribute(Document* document,
                                         const QualifiedName& attributeName);
};
}

#endif
