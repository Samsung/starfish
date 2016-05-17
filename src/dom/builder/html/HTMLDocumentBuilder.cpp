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

    {
    }

    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        load();
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        load();
    }

    void load()
    {
        Document* document = m_builder.document();
        HTMLParser parser(document->window()->starFish(), document, resource()->asTextResource()->text());
        parser.parse();

        String* eventType = document->window()->starFish()->staticStrings()->m_DOMContentLoaded.localName();
        Event* e = new Event(eventType, EventInit(true, true));
        document->EventTarget::dispatchEvent(e);
    }

protected:
    HTMLDocumentBuilder& m_builder;
};

void HTMLDocumentBuilder::build(const URL& url)
{
    TextResource* res = m_document->resourceLoader()->fetchText(url);
    res->addResourceClient(new HTMLResourceClient(res, *this));
    res->request(true);
}

}
