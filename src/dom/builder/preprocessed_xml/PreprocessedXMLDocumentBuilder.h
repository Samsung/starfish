#ifndef __StarFishPreprocessedXMLDocumentBuilderElement__
#define __StarFishPreprocessedXMLDocumentBuilderElement__

#include "dom/builder/DocumentBuilder.h"

namespace StarFish {

class Window;
class PreprocessedXMLDocumentBuilder : public DocumentBuilder {
public:
    PreprocessedXMLDocumentBuilder()
    {
    }

    virtual void build(Document* element, String* filePath);

protected:
};


}

#endif
