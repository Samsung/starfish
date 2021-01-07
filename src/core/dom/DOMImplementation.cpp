/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#include "StarfishConfig.h"
#include "binding/ScriptWrappable.h"
#include "binding/DocumentHoldable.h"
#include "core/dom/Document.h"
#include "core/dom/DocumentType.h"
#include "core/dom/DOMImplementation.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/HTMLHeadElement.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/dom/HTMLDocument.h"
#include "core/dom/DOMException.h"
#include "core/dom/XMLDocument.h"
#include "core/dom/Text.h"

namespace Starfish {

DOMImplementation::DOMImplementation(Document* document)
    : ScriptWrappable(this)
    , DocumentHoldable(document)
{
}

ScriptBindingInstance* DOMImplementation::scriptBindingInstance()
{
    return document()->scriptBindingInstance();
}

DocumentType* DOMImplementation::createDocumentType(String* qualifiedName,
                                                    String* publicId,
                                                    String* systemId)
{
    // Validate qualifiedName.
    if (!QualifiedName::validateQualifiedName(qualifiedName)) {
        throw new DOMException(document()->executionContext(),
                               DOMException::INVALID_CHARACTER_ERR);
    }

    // Return a new doctype, with qualifiedName as its name, publicId as its
    // public ID,
    // and systemId as its system ID, and with its node document
    // set to the associated document of the context object.
    return new DocumentType(document(), qualifiedName, publicId, systemId);
}

XMLDocument* DOMImplementation::createDocument(
    Nullable<String*> namespaceParameter, String* qualifiedName,
    DocumentType* doctype)
{
    // Let document be a new XMLDocument.
    XMLDocument* document = new XMLDocument(
        window(), scriptBindingInstance(), ResourceURL::aboutBlankURL(),
        String::createASCIIString("UTF-8"), false);
    // Let element be null.
    Element* element = nullptr;
    // If qualifiedName is not the empty string,
    // then set element to the result of running the internal createElementNS
    // steps,
    // given document, namespace, qualifiedName, and an empty dictionary.
    if (qualifiedName->length()) {
        element = document->createElementNS(namespaceParameter, qualifiedName);
    }
    // If doctype is non-null, append doctype to document.
    if (doctype) {
        document->appendChild(doctype);
        doctype->setDocument(document);
        doctype->setParentNode(document);
    }
    // If element is non-null, append element to document.
    if (element) {
        document->appendChild(element);
    }

    // document’s origin is context object’s associated document’s origin.
    document->setWebOrigin(m_document->webOrigin());

    // document’s content type is determined by namespace:
    if (namespaceParameter.hasValue()) {
        if (namespaceParameter.getValue()->equals(HTML_NAMESPACE)) {
            document->setContentType(
                String::createASCIIString("application/xhtml+xml"));
        } else if (namespaceParameter.getValue()->equals(SVG_NAMESPACE)) {
            document->setContentType(
                String::createASCIIString("image/svg+xml"));
        } else {
            // Any other namespace -> application/xml
            // document already have application/xml
        }
    }

    return document;
}

Document* DOMImplementation::createHTMLDocument(Nullable<String*> title)
{
    // Let doc be a new document that is an HTML document.
    // Set doc’s content type to "text/html".
    Document* doc = new HTMLDocument(window(), scriptBindingInstance(),
                                     ResourceURL::aboutBlankURL(),
                                     String::createASCIIString("UTF-8"), false);

    DocumentType* docType =
        new DocumentType(doc, String::createASCIIString("html"),
                         String::emptyString, String::emptyString);
    doc->appendChild(docType);

    HTMLHtmlElement* html =
        new HTMLHtmlElement(doc, doc->staticStrings()->m_htmlTagName);
    doc->appendChild(html);

    HTMLHeadElement* head =
        new HTMLHeadElement(doc, doc->staticStrings()->m_headTagName);
    html->appendChild(head);

    if (title.hasValue()) {
        Element* titleElement =
            doc->createElement(String::createASCIIString("title"));
        titleElement->appendChild(new Text(doc, title.getValue()));
        head->appendChild(titleElement);
    }

    html->appendChild(
        new HTMLBodyElement(doc, doc->staticStrings()->m_bodyTagName));

    // doc’s origin is context object’s associated document’s origin.
    doc->setWebOrigin(m_document->webOrigin());
    return doc;
}
} // namespace Starfish
