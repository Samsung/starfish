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

#include "binding/ScriptWrappable.h"
#include "platform/window/Window.h"
#include "dom/Document.h"

namespace StarFish {

class StarFish;
class URL;

class Location : public ScriptWrappable {
public:
    Location(StarFish* starFish);
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

    String* href()
    {
        return url()->href();
    }

    String* host()
    {
        String* hostname = url()->hostname();
        String* port = url()->port();
        if (!port->equals(String::emptyString) &&
            !hostname->equals(String::emptyString)) {
            return (hostname->concat(String::fromUTF8(":")))->concat(port);
        } else {
            return hostname;
        }
    }

    String* hostname()
    {
        return url()->hostname();
    }

    String* protocol()
    {
        return url()->protocol();
    }

    String* pathname()
    {
        return url()->pathname();
    }

    String* search()
    {
        return url()->search();
    }

    String* hash()
    {
        return url()->hash();
    }

    void setHref(String* newURL)
    {
        setLocation(newURL);
    }

    void setHost(String* newHost)
    {
        url()->setHost(newHost);
        setLocation(url()->urlString());
    }

    void setHostname(String* newHostname)
    {
        url()->setHostname(newHostname);
        setLocation(url()->urlString());
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

    virtual bool isLocation() const
    {
        return true;
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
