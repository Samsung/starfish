/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __StarFishResourceURL__
#define __StarFishResourceURL__

namespace StarFish {

class Document;

class ResourceURL : public gc {
    enum Protocol {
        FILE_PROTOCOL,
        BLOB_PROTOCOL,
        DATA_PROTOCOL,
        ABOUT_PROTOCOL,
        // FILE, BLOB, DATA should be smaller than HTTP
        // (there's code which assumes this)
        HTTP_PROTOCOL,
        HTTPS_PROTOCOL,
        UNKNOWN,
    };

public:
    ResourceURL(const char* url)
        : ResourceURL(String::createASCIIString(url))
    {
    }
    ResourceURL(String* url);
    ResourceURL(String* url, String* baseURL);

    ResourceURL(const ResourceURL& src)
    {
        m_string = src.m_string;
        m_urlString = src.m_urlString;

        m_protocolEnd = src.m_protocolEnd;
        m_userStart = src.m_userStart;
        m_userEnd = src.m_userEnd;
        m_passwordEnd = src.m_passwordEnd;
        m_hostEnd = src.m_hostEnd;
        m_portEnd = src.m_portEnd;
        m_pathEnd = src.m_pathEnd;
        m_queryEnd = src.m_queryEnd;
        m_fragmentEnd = src.m_fragmentEnd;

        m_protocol = src.m_protocol;
    }

    static String* mergeDocumentURIWithURIString(Document* document,
                                                 String* url);
    static String* mergeDocumentURIWithURIString(String* documentURI,
                                                 String* url);
    String* baseURI() const;

    static bool isValidURL(String* url);

    bool url()
    {
        return m_protocol == HTTP_PROTOCOL || m_protocol == HTTPS_PROTOCOL;
    }

    bool isFileURL() const
    {
        return m_protocol == FILE_PROTOCOL;
    }

    bool isDataURL() const
    {
        return m_protocol == DATA_PROTOCOL;
    }

    bool isBlobURL() const
    {
        return m_protocol == BLOB_PROTOCOL;
    }

    bool isAboutURL() const
    {
        return m_protocol == ABOUT_PROTOCOL;
    }

    bool isNetworkURL()
    {
        return m_protocol == HTTP_PROTOCOL || m_protocol == HTTPS_PROTOCOL;
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
    String* getUrlPathString() const;
    bool operator==(const ResourceURL& other) const
    {
        return other.urlString()->equals(m_urlString);
    }

    String* origin();
    String* href();
    ResourceURL* setHref(String* newHref);
    String* protocol();
    ResourceURL* setProtocol(String* newProtocol);
    String* username();
    ResourceURL* setUsername(String* newPath);
    String* password();
    ResourceURL* setPassword(String* newPath);
    String* host();
    ResourceURL* setHost(String* newHost);
    String* hostname();
    ResourceURL* setHostname(String* newHostname);
    String* port();
    ResourceURL* setPort(String* newPost);
    String* pathname();
    ResourceURL* setPathname(String* newPath, bool needRemovingDots = true);
    String* search();
    ResourceURL* setSearch(String* newPath);
    String* hash();
    ResourceURL* setHash(String* newPath);

protected:
    void resolvePositions();
    void parseURLString(String* baseURL, String* url);

    String* m_string;
    String* m_urlString;

    unsigned int m_protocolEnd;
    unsigned int m_userStart;
    unsigned int m_userEnd;
    unsigned int m_passwordEnd;
    unsigned int m_hostEnd;
    unsigned int m_portEnd;
    unsigned int m_pathEnd;
    unsigned int m_queryEnd;
    unsigned int m_fragmentEnd;

    enum Protocol m_protocol;
};
}

#endif
