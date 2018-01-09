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
class WebOrigin;
class FormSubmitData;
class DocumentURL;

class ResourceURL : public gc {
    friend WebOrigin;

public:
    enum Protocol {
        FILE_PROTOCOL,
        BLOB_PROTOCOL,
        DATA_PROTOCOL,
        ABOUT_PROTOCOL,
        // FILE, BLOB, DATA should be smaller than HTTP
        // (there's code which assumes this)
        HTTP_PROTOCOL,
        HTTPS_PROTOCOL,
        JAVASCRIPT_PROTOCOL,
        UNKNOWN,
    };

    static String* createPercentEncodingString(String* src, bool forForm);
    static String* createPercentDecodingString(String* src);

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
        m_usernameStart = src.m_usernameStart;
        m_usernameEnd = src.m_usernameEnd;
        m_passwordEnd = src.m_passwordEnd;
        m_hostEnd = src.m_hostEnd;
        m_portEnd = src.m_portEnd;
        m_pathEnd = src.m_pathEnd;
        m_searchEnd = src.m_searchEnd;
        m_hashEnd = src.m_hashEnd;

        m_protocol = src.m_protocol;
        m_isValid = src.m_isValid;
    }

    static ResourceURL* AboutBlankURL()
    {
        return new ResourceURL("about:blank");
    }

    static String* mergeDocumentURIWithURIString(Document* document,
                                                 String* url);
    static String* mergeDocumentURIWithURIString(String* documentURI,
                                                 String* url);
    String* baseURI() const;

    static bool isValidURL(String* url);

    bool isValid()
    {
        return m_isValid;
    }

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

    bool isJavascriptURL() const
    {
        return m_protocol == JAVASCRIPT_PROTOCOL;
    }

    bool isUnknownURL() const
    {
        return m_protocol == UNKNOWN;
    }

    bool isNetworkURL() const
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

    Protocol protocolKind()
    {
        return m_protocol;
    }

    // http://foo.com/asdf?asdf=1 -> http://foo.com/asdf
    String* urlStringWithoutSearchPart() const;
    String* getUrlPathString() const;

    bool operator==(const ResourceURL& other) const
    {
        return other.urlString()->equals(m_urlString);
    }

    bool operator!=(const ResourceURL& other) const
    {
        return !operator==(other);
    }

    String* origin();
    String* href();
    ResourceURL* setHref(String* newHref);
    String* protocol();
    ResourceURL* setProtocol(String* newProtocol);
    String* username();
    ResourceURL* setUsername(String* newUsername);
    String* password();
    ResourceURL* setPassword(String* newPassword);
    String* host();
    ResourceURL* setHost(String* newHost);
    String* hostname();
    ResourceURL* setHostname(String* newHostname);
    String* port();
    ResourceURL* setPort(String* newPort);
    String* pathname();
    ResourceURL* setPathname(String* newPathname, bool needRemovingDots = true);
    String* search();
    ResourceURL* setSearch(String* newSearch);
    String* hash();
    ResourceURL* setHash(String* newHash);

    virtual bool isDocumentURL()
    {
        return false;
    }

    DocumentURL* asDocumentURL()
    {
        STARFISH_ASSERT(isDocumentURL());
        return (DocumentURL*)this;
    }

protected:
    void resolvePositions();
    void parseURLString(String* baseURL, String* url);
    bool isValidPort();

    String* m_string;
    String* m_urlString;

    unsigned int m_protocolEnd;
    unsigned int m_usernameStart;
    unsigned int m_usernameEnd;
    unsigned int m_passwordEnd;
    unsigned int m_hostEnd;
    unsigned int m_portEnd;
    unsigned int m_pathEnd;
    unsigned int m_searchEnd;
    unsigned int m_hashEnd;

    enum Protocol m_protocol;

    bool m_isValid;
};

class DocumentURL : public ResourceURL {
public:
    DocumentURL(String* url);
    DocumentURL(String* url, FormSubmitData* formSubmitData);
    DocumentURL(ResourceURL* url);
    DocumentURL(ResourceURL* url, FormSubmitData* formSubmitData);

    virtual bool isDocumentURL() override
    {
        return true;
    }

    FormSubmitData* formSubmitData()
    {
        return m_formSubmitData;
    }

private:
    FormSubmitData* m_formSubmitData;
};
}

#endif
