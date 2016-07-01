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
        m_htmlSource = String::createASCIIString("<div style='text-align:center;margin-top:50px;'>Cannot open page ");
        m_htmlSource = m_htmlSource->concat(m_resource->url().urlString());
        m_htmlSource = m_htmlSource->concat(String::createASCIIString("</div>"));
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
