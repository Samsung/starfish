/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
