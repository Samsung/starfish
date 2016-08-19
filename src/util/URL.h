/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#ifndef __StarFishURL__
#define __StarFishURL__

#include "util/String.h"
#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class Blob;

class URL : public ScriptWrappable {
    URL(String* baseURL, String* url);
    enum Protocol {
        FILE_PROTOCOL,
        BLOB_PROTOCOL,
        DATA_PROTOCOL,
        // FILE, BLOB, DATA should be smaller than HTTP (there's code which assumes this)
        HTTP_PROTOCOL,
        HTTPS_PROTOCOL,
        UNKNOWN,
    };
public:
    static String* getURLString(String* baseURL, String* url);
    static String* createObjectURL(Blob* blob);
    static URL* createURL(String* baseURL, String* url)
    {
        return new URL(baseURL, url);
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this, instance);
    }

    String* baseURI() const;

    bool isNetworkURL()
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
    bool operator==(const URL& other) const
    {
        return other.urlString()->equals(m_urlString);
    }

    String* origin();
    String* getHref();
    String* getProtocol();
    String* getUsername();
    void setUsername(String* newPath);
    String* getPassword();
    void setPassword(String* newPath);
    String* getHost();
    String* getHostname();
    String* getPort();
    String* getPathname();
    void setPathname(String* newPath, bool needRemovingDots = true);
    String* getSearch();
    String* getHash();

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
