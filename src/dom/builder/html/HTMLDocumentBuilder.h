#ifndef __StarFishHTMLDocumentBuilderElement__
#define __StarFishHTMLDocumentBuilderElement__

#include "dom/builder/DocumentBuilder.h"

namespace StarFish {

class Window;
class HTMLDocumentBuilder : public DocumentBuilder {
public:
    HTMLDocumentBuilder(Document* document)
        : DocumentBuilder(document)
    {
    }

    virtual void build(const URL& url);

protected:
};


}

#endif
