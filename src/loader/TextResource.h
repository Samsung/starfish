#ifndef __StarFishTextResource__
#define __StarFishTextResource__

#include "loader/Resource.h"

namespace StarFish {

class TextResource : public Resource {
    friend class ResourceLoader;
    TextResource(const URL& url, ResourceLoader* loader)
        : Resource(url, loader)
        , m_text(String::emptyString)
    {
    }
public:
    virtual bool isTextResource()
    {
        return true;
    }

    virtual void didDataReceived(const char* buffer, size_t length)
    {
        Resource::didDataReceived(buffer, length);
        m_text = m_text->concat(String::fromUTF8(buffer, length));
    }

    String* text()
    {
        return m_text;
    }

protected:
    // TextEncoding m_textEncoding;
    String* m_text;
};
}

#endif
