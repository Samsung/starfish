#ifndef __StarFishURL__
#define __StarFishURL__

#include "util/String.h"

namespace StarFish {

class URL {
public:
    STARFISH_MAKE_STACK_ALLOCATED();
    URL(String* url = String::emptyString)
        : m_string(url)
    {
    }

    String* string() const
    {
        return m_string;
    }

    String* path() const // directory name (with slash)
    {
        size_t idx = m_string->lastIndexOf('/');
        if (idx == SIZE_MAX)
            return String::emptyString;
        STARFISH_ASSERT(m_string->substring(0, idx + 1)->charAt(idx) == '/');
        return m_string->substring(0, idx + 1);
    }
protected:
    String* m_string;
    // String* m_protocol;
    // String* m_host;
    // String* m_path;
};
}

#endif
