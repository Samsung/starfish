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

#ifndef __StarFishDOMImplementation__
#define __StarFishDOMImplementation__

#include "binding/ScriptWrappable.h"
#include "binding/DocumentHoldable.h"

namespace StarFish {

class DocumentType;
class XMLDocument;
class Window;

class DOMImplementation : public ScriptWrappable, public DocumentHoldable {
public:
    DOMImplementation(Document* document, ScriptBindingInstance* instance)
        : ScriptWrappable(this)
        , DocumentHoldable(document)
        , m_instance(instance)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isDOMImplementation() const override;
    virtual ScriptBindingInstance* scriptBindingInstance()
    {
        return DocumentHoldable::scriptBindingInstance();
    }

    DocumentType* createDocumentType(String* qualifiedName, String* publicId,
                                     String* systemId);
    XMLDocument* createDocument(Nullable<String*> namespaceParameter,
                                String* qualifiedName, DocumentType* doctype);
    Document* createHTMLDocument(Nullable<String*> title = Nullable<String*>());

    // useless; always returns true
    bool hasFeature()
    {
        return true;
    }

private:
    ScriptBindingInstance* m_instance;
};
}

#endif
