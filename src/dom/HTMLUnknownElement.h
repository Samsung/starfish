#ifndef __StarFishHTMLUnknownElement__
#define __StarFishHTMLUnknownElement__

#include "dom/HTMLElement.h"

namespace StarFish {

class HTMLUnknownElement : public HTMLElement {
public:
    HTMLUnknownElement(Document* document, String* localName)
        : HTMLElement(document)
    {
        initScriptWrappable(this);
        m_localName = localName;
    }

    /* 4.4 Interface Node */

    virtual String* localName()
    {
        return m_localName;
    }

protected:
    String* m_localName;
};

}

#endif
