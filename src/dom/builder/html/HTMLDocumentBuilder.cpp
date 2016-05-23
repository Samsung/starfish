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
        , m_parser(nullptr)
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
        m_builder.m_parser = m_parser = new HTMLParser(document->window()->starFish(), document, m_htmlSource);
        m_parser->startParse();
        m_parser->parseStep();
    }

protected:
    HTMLDocumentBuilder& m_builder;
    HTMLParser* m_parser;
    String* m_htmlSource;
};

void HTMLDocumentBuilder::build(const URL& url)
{
    m_textResource = m_document->resourceLoader()->fetchText(url);
    m_textResource->addResourceClient(new HTMLResourceClient(m_textResource, *this));
    m_textResource->request();
}

void HTMLDocumentBuilder::resume()
{
    m_parser->parseStep();
}

}
