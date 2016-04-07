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
