#include "StarFishConfig.h"
#include "URL.h"

namespace StarFish {

URL::URL(String* baseURL, String* url)
    : m_string(url)
{
    if (url->startsWith("data:")) {
        m_urlString = m_string = url;
        return;
    }

    bool isAbsolute = url->contains("://");

    if (url->startsWith("//")) {
        isAbsolute = true;
        STARFISH_ASSERT(baseURL->length());
        size_t idx = baseURL->indexOf(':');
        url = baseURL->substring(0, idx + 1)->concat(url);
    }

    if (url->startsWith("/")) {
        isAbsolute = true;
        if (baseURL->startsWith("file://")) {
            url = String::createASCIIString("file://")->concat(url);
        } else {
            size_t pos = baseURL->find("://");
            STARFISH_ASSERT(pos != SIZE_MAX);
            size_t pos2 = baseURL->find("/", pos + 3);
            if (pos2 != SIZE_MAX) {
                baseURL = baseURL->substring(0, pos2);
                url = baseURL->concat(url);
            } else {
                url = baseURL->concat(url);
            }
        }
    }

    if (baseURL->equals(String::emptyString)) {
        STARFISH_ASSERT(isAbsolute);
    }

    if (isAbsolute) {
        m_urlString = url;
    } else {
        STARFISH_ASSERT(baseURL->contains("://"));
        bool baseEndsWithSlash = baseURL->charAt(baseURL->length() - 1) == '/';

        if (url->startsWith("./")) {
            url = url->substring(2, url->length() - 2);
        }

        if (baseEndsWithSlash) {
            m_urlString = baseURL->concat(url);
        } else {
            size_t f = baseURL->find("://");
            STARFISH_ASSERT(f != SIZE_MAX);
            f += 3;
            size_t f2 = baseURL->find("/", f);
            if (f2 != SIZE_MAX) {
                baseURL = baseURL->substring(0, baseURL->lastIndexOf('/'));
            }
            m_urlString = baseURL->concat(String::createASCIIString("/"))->concat(url);
        }
    }
}

String* URL::baseURI() const
{
    if (m_urlString->startsWith("data:")) {
        return String::emptyString;
    }

    size_t pos = m_urlString->find("://");
    STARFISH_ASSERT(pos != SIZE_MAX);
    size_t pos2 = m_urlString->find("/", pos + 3);
    if (pos2 != SIZE_MAX) {
        return m_urlString->substring(0, m_urlString->lastIndexOf('/') + 1);
    } else {
        return m_urlString;
    }
}

String* URL::urlStringWithoutSearchPart() const
{
    size_t idx = m_urlString->lastIndexOf('?');
    if (idx != SIZE_MAX) {
        return m_urlString->substring(0, idx);
    } else {
        return m_urlString;
    }
}

}
