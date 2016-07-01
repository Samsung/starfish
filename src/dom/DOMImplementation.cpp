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

#ifdef STARFISH_EXP

#include "StarFishConfig.h"
#include "DOMImplementation.h"
#include "dom/DocumentType.h"

#include "dom/Document.h"
#include "dom/HTMLHtmlElement.h"
#include "dom/HTMLHeadElement.h"
#include "dom/HTMLBodyElement.h"

namespace StarFish {

DocumentType* DOMImplementation::createDocumentType(String* qualifiedName, String* publicId, String* systemId)
{
    return new DocumentType(nullptr, qualifiedName, publicId, systemId);
}

Document* DOMImplementation::createHTMLDocument(String* title)
{
    Document* doc = new Document(m_window, m_instance);
    DocumentType* docType = new DocumentType(doc);
    doc->appendChild(docType);

    HTMLHtmlElement* html = new HTMLHtmlElement(doc);
    doc->appendChild(html);

    // FIXME: not setting up title
    HTMLHeadElement* head = new HTMLHeadElement(doc);
    html->appendChild(head);

    html->appendChild(new HTMLBodyElement(doc));
    return doc;
}

}

#endif
