#ifndef __StarFishURL__
#define __StarFishURL__

#include "util/String.h"

namespace StarFish {

class URL {
    STARFISH_MAKE_STACK_ALLOCATED();
public:
    URL(String* baseURL, String* url);
    String* baseURI() const;
    bool isFileURL() const
    {
        return m_urlString->startsWith("file://");
    }

    bool isDataURL() const
    {
        return m_urlString->startsWith("data:");
    }

    String* string() const
    {
        return m_string;
    }

    String* urlString() const
    {
        return m_urlString;
    }

    // http://foo.com/asdf?asdf=1 -> http://foo.com/asdf
    String* urlStringWithoutSearchPart() const;

    operator bool()
    {
        return m_urlString->length();
    }
protected:
    String* m_string;
    String* m_urlString;
};
}

#endif
