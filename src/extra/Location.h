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

#ifndef __StarFishLocation__
#define __StarFishLocation__

#include "dom/binding/ScriptWrappable.h"
#include "platform/window/Window.h"
#include "dom/Document.h"

namespace StarFish {

class StarFish;
class URL;

class LocationObj : public ScriptWrappable {
public:
    LocationObj(StarFish* starFish);
    StarFish* starFish()
    {
        return m_starFish;
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    URL* url()
    {
        return m_starFish->window()->document()->documentURI();
    }

    String* getHref()
    {
        return url()->getHref();
    }

    String* getHost()
    {
        String* hostname = url()->getHostname();
        String* port = url()->getPort();
        if (!port->equals(String::emptyString) &&
            !hostname->equals(String::emptyString)) {
            return (hostname->concat(String::fromUTF8(":")))->concat(port);
        } else {
            return hostname;
        }
    }

    String* getHostname()
    {
        return url()->getHostname();
    }

    String* getProtocol()
    {
        return url()->getProtocol();
    }

    String* getPathname()
    {
        return url()->getPathname();
    }

    String* getSearch()
    {
        return url()->getSearch();
    }

    String* getHash()
    {
        return url()->getHash();
    }

    void setHref(String* newURL)
    {
        setLocation(newURL);
    }

    void setProtocol(String* newProtocol)
    {
        url()->setProtocol(newProtocol);
        setLocation(url()->urlString());
    }

    void setPathname(String* newPath, bool needRemovingDots = true)
    {
        url()->setPathname(newPath, needRemovingDots);
        setLocation(url()->urlString());
    }

    void setSearch(String* search)
    {
        url()->setSearch(search);
        setLocation(url()->urlString());
    }

    void setHash(String* search)
    {
        url()->setHash(search);
        setLocation(url()->urlString());
    }

    virtual Type type()
    {
        return ScriptWrappable::Type::LocationObject;
    }

    void close()
    {
    }

    void setLocation(String* newURL)
    {
        m_starFish->window()->navigateAsync(URL::createURL(
            m_starFish->window()->document()->documentURI()->urlString(),
            newURL));
    }

protected:
    StarFish* m_starFish;
};
}

#endif
