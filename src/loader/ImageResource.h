#ifndef __StarFishImageResource__
#define __StarFishImageResource__

#include "loader/Resource.h"

namespace StarFish {

class ImageData;
class ImageResource : public Resource {
    friend class ResourceLoader;
    ImageResource(const URL& url, ResourceLoader* loader)
        : Resource(url, loader)
    {
        m_imageData = nullptr;
    }
public:
    virtual bool isImageResource()
    {
        return true;
    }

    ImageData* imageData()
    {
        return m_imageData;
    }

    virtual void request(bool needsSyncRequest = false);
protected:
    ImageData* m_imageData;
};
}

#endif
