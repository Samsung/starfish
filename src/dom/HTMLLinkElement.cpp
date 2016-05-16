#include "StarFishConfig.h"
#include "dom/Document.h"
#include "HTMLLinkElement.h"

#include "loader/ElementResourceClient.h"
#include "platform/message_loop/MessageLoop.h"
#include "style/CSSParser.h"
#include "platform/file_io/FileIO.h"

namespace StarFish {

URL HTMLLinkElement::href()
{
    size_t href = hasAttribute(document()->window()->starFish()->staticStrings()->m_href);
    if (href != SIZE_MAX) {
        String* url = getAttribute(href);
        return URL(document()->documentURI().baseURI(), url);
    }
    return URL(String::emptyString, String::emptyString);
}

void HTMLLinkElement::didNodeInsertedToDocumenTree()
{
    HTMLElement::didNodeInsertedToDocumenTree();
    checkLoadStyleSheet();
}

void HTMLLinkElement::didNodeRemovedFromDocumenTree()
{
    HTMLElement::didNodeRemovedFromDocumenTree();
    unloadStyleSheetIfExists();
}

void HTMLLinkElement::checkLoadStyleSheet()
{
    if (!isInDocumentScope()) {
        unloadStyleSheetIfExists();
        return;
    }

    size_t type = hasAttribute(document()->window()->starFish()->staticStrings()->m_type);
    size_t href = hasAttribute(document()->window()->starFish()->staticStrings()->m_href);
    size_t rel = hasAttribute(document()->window()->starFish()->staticStrings()->m_rel);

    if (((type != SIZE_MAX && getAttribute(type)->toLower()->equals("text/css")) || type == SIZE_MAX)
        && href != SIZE_MAX
        && rel != SIZE_MAX && getAttribute(rel)->toLower()->equals("stylesheet")) {
        loadStyleSheet();
    } else {
        unloadStyleSheetIfExists();
    }
}

class StyleSheetDownloadClient : public ResourceClient {
public:
    StyleSheetDownloadClient(HTMLLinkElement* element, Resource* res)
        : ResourceClient(res)
        , m_element(element)
    {
    }

    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        m_element->m_styleSheetTextResource = nullptr;
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        String* text = m_resource->asTextResource()->text();

        CSSParser parser(m_element->document());
        CSSStyleSheet* sheet = parser.parseStyleSheet(text, m_element);
        if (sheet) {
            m_element->m_generatedSheet = sheet;
            m_element->document()->styleResolver()->addSheet(sheet);
            m_element->document()->window()->setWholeDocumentNeedsStyleRecalc();
        }
    }
protected:
    HTMLLinkElement* m_element;
};

void HTMLLinkElement::loadStyleSheet()
{
    unloadStyleSheetIfExists();
    size_t href = hasAttribute(document()->window()->starFish()->staticStrings()->m_href);

    String* urlString = getAttribute(href);
    URL url = URL(document()->documentURI().baseURI(), urlString);

    if (url) {
        m_styleSheetTextResource = document()->resourceLoader()->fetchText(url);
        m_styleSheetTextResource->addResourceClient(new StyleSheetDownloadClient(this, m_styleSheetTextResource));
        m_styleSheetTextResource->addResourceClient(new ElementResourceClient(this, m_styleSheetTextResource));
        m_styleSheetTextResource->request();
    }
}

void HTMLLinkElement::unloadStyleSheetIfExists()
{
    if (m_styleSheetTextResource) {
        m_styleSheetTextResource->cancel();
        m_styleSheetTextResource = nullptr;
    }
    if (m_generatedSheet) {
        document()->styleResolver()->removeSheet(m_generatedSheet);
        document()->window()->setWholeDocumentNeedsStyleRecalc();
        m_generatedSheet = nullptr;
    }
}

void HTMLLinkElement::didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated, attributeRemoved);
    if (name == document()->window()->starFish()->staticStrings()->m_href) {
        if (!old->equals(value))
            checkLoadStyleSheet();
    } else if (name == document()->window()->starFish()->staticStrings()->m_type) {
        checkLoadStyleSheet();
    } else if (name == document()->window()->starFish()->staticStrings()->m_rel) {
        checkLoadStyleSheet();
    }
}

}
