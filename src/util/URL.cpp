#include "StarFishConfig.h"
#include "URL.h"

namespace StarFish {

URL::URL(String* baseURL, String* url)
    : m_string(url)
{
    bool isAbsolute = url->contains("://");

    if (url->startsWith("//")) {
        isAbsolute = true;
        STARFISH_ASSERT(baseURL->length());
        size_t idx = baseURL->indexOf(':');
        url = baseURL->substring(0, idx + 1)->concat(url);
    }

    if (url->startsWith("data:")) {
        m_urlString = m_string = url;
        return;
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
        } else if (url->startsWith("/")) {
            url = url->substring(1, url->length() - 1);
        }

        if (baseEndsWithSlash) {
            m_urlString = baseURL->concat(url);
        } else {
            baseURL = baseURL->substring(0, baseURL->lastIndexOf('/'));
            m_urlString = baseURL->concat(String::createASCIIString("/"))->concat(url);
        }
    }
}

String* URL::baseURI() const
{
    if (m_urlString->startsWith("data:")) {
        return String::emptyString;
    }
    return m_urlString->substring(0, m_urlString->lastIndexOf('/') + 1);
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
