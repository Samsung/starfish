#ifndef __StarFishDocumentBuilderElement__
#define __StarFishDocumentBuilderElement__

#include "../Document.h"

namespace StarFish {

class DocumentBuilder : public gc {
public:
    DocumentBuilder(Document* document)
        : m_document(document)
    {
    }

    virtual ~DocumentBuilder()
    {

    }

    virtual void build(const URL& filePath) = 0;
    Document* document()
    {
        return m_document;
    }
protected:
    Document* m_document;
};


}

#endif
