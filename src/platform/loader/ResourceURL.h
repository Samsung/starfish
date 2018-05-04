/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#ifndef __StarFishResourceURL__
#define __StarFishResourceURL__

namespace StarFish {

class Document;
class WebOrigin;
class FormSubmitData;
class DocumentURL;
class ReferrerURL;

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
        m_baseURL = src.m_baseURL;

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
    virtual ~ResourceURL(){};

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;
    static void fillGCDescriptor(GC_word* desc);

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

    virtual bool isReferrerURL()
    {
        return false;
    }

    ReferrerURL* asReferrerURL()
    {
        STARFISH_ASSERT(isReferrerURL());
        return (ReferrerURL*)this;
    }

protected:
    void resolvePositions();
    void parseURLString(String* baseURL, String* url);
    bool isValidPort();

    String* m_string;
    String* m_urlString;
    String* m_baseURL;

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

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

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

class ReferrerURL : public ResourceURL {
public:
    enum ReferrerPolicy {
        NoReferrer,
        NoReferrerWhenDowngrade,
        Origin,
        OriginWhenCrossOrigin,
        SameOrigin,
        StrictOrigin,
        StrictOriginWhenCrossOrigin,
        UnsafeUrl
    };

    ReferrerURL(String* url);
    ReferrerURL(String* url, String* policy);
    ReferrerURL(ResourceURL* url);
    ReferrerURL(ResourceURL* url, String* policy);

    virtual bool isReferrerURL() override
    {
        return true;
    }

    String* referrerString(ResourceURL* url);
    ReferrerPolicy policy();
    static bool isValidPolicy(String* policy);

private:
    ReferrerPolicy m_policy;

    ReferrerPolicy policyFromString(String* policy);
};
}

#endif
