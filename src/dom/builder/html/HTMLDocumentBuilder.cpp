#include "StarFishConfig.h"
#include "HTMLDocumentBuilder.h"

#include "dom/DOM.h"
#include "dom/binding/ScriptBindingInstance.h"
#include "dom/parser/HTMLParser.h"

namespace StarFish {

class HTMLResourceClient : public ResourceClient {
public:
    HTMLResourceClient(Resource* res, HTMLDocumentBuilder& builder)
        : ResourceClient(res)
        , m_builder(builder)
        , m_htmlSource(String::emptyString)
    {
    }

    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        m_htmlSource = String::createASCIIString("Cannot open page ");
        m_htmlSource = m_htmlSource->concat(m_resource->url().urlString());
        load();
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        m_htmlSource = m_resource->asTextResource()->text();
        load();
    }

    void load()
    {
        Document* document = m_builder.document();
        HTMLParser parser(document->window()->starFish(), document, m_htmlSource);
        parser.parse();
        String* eventType = document->window()->starFish()->staticStrings()->m_DOMContentLoaded.localName();
        Event* e = new Event(eventType, EventInit(true, true));
        document->EventTarget::dispatchEvent(e);
    }

protected:
    HTMLDocumentBuilder& m_builder;
    String* m_htmlSource;
};

void HTMLDocumentBuilder::build(const URL& url)
{
    TextResource* res = m_document->resourceLoader()->fetchText(url);
    res->addResourceClient(new HTMLResourceClient(res, *this));
    res->request(true);
}

}
