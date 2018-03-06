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

#ifndef __StarFishXMLDocument__
#define __StarFishXMLDocument__

#include "core/dom/Document.h"

namespace StarFish {

class URL;

class XMLDocument : public Document {
public:
    XMLDocument(Window* window, ScriptBindingInstance* scriptBindingInstance,
                ResourceURL* uri, String* charSet,
                bool doesParticipateInRendering)
        : Document(window, scriptBindingInstance, uri, charSet,
                   doesParticipateInRendering)
    {
    }

    XMLDocument(XMLDocument& doc)
        : Document(doc.m_window, doc.scriptBindingInstance(), doc.m_documentURI,
                   doc.m_characterSet, false)
    {
        m_contentType = String::createASCIIString("text/html");
    }

    Node* clone()
    {
        return new XMLDocument(*this);
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isXMLDocument() const override;
};
}

#endif
