#include "StarFishConfig.h"
#include "DOMImplementation.h"
#include "dom/DocumentType.h"

#include "dom/Document.h"

namespace StarFish {

DocumentType* DOMImplementation::createDocumentType(String* qualifiedName, String* publicId, String* systemId)
{
    return nullptr;
    // return new DocumentType(nullptr, qualifiedName, publicId, systemId);
}

Document* DOMImplementation::createHTMLDocument(String* title)
{
    Document* doc = nullptr; //new Document(m_window, m_instance);
    return doc;
}

}
