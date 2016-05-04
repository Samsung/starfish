#ifndef __StarFishHTMLDocumentBuilderElement__
#define __StarFishHTMLDocumentBuilderElement__

#include "dom/builder/DocumentBuilder.h"

namespace StarFish {

class Window;
class HTMLDocumentBuilder : public DocumentBuilder {
public:
    HTMLDocumentBuilder()
    {
    }

    virtual void build(Document* element, String* filePath);

protected:
};


}

#endif
