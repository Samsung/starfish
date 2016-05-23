#ifndef __StarFishHTMLDocumentBuilderElement__
#define __StarFishHTMLDocumentBuilderElement__

#include "dom/builder/DocumentBuilder.h"

namespace StarFish {

class Window;
class TextResource;
class HTMLParser;

class HTMLDocumentBuilder : public DocumentBuilder {
    friend class HTMLResourceClient;
public:
    HTMLDocumentBuilder(Document* document)
        : DocumentBuilder(document)
        , m_parser(nullptr)
        , m_textResource(nullptr)
    {
    }

    virtual void build(const URL& url);
    virtual void resume();
    HTMLParser* parser()
    {
        return m_parser;
    }
protected:
    HTMLParser* m_parser;
    TextResource* m_textResource;
};


}

#endif
