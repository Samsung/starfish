#ifndef __StarFishElementResourceClient__
#define __StarFishElementResourceClient__

#include "loader/ResourceClient.h"

namespace StarFish {

class Element;
class ElementResourceClient : public ResourceClient {
public:
    ElementResourceClient(Element* element, Resource* resource, bool needsSyncEventDispatch = false)
        : ResourceClient(resource)
        , m_needsSyncEventDispatch(needsSyncEventDispatch)
        , m_element(element)
    {
    }
    virtual void didLoadFinished();
    virtual void didLoadFailed();
protected:
    bool m_needsSyncEventDispatch;
    Element* m_element;
};
}

#endif
