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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined (__StarFishHTMLVideoElement__)
#define __StarFishHTMLVideoElement__

#include "dom/HTMLMediaElement.h"

namespace StarFish {

class HTMLVideoElement : public HTMLMediaElement {
public:
    HTMLVideoElement(Document* document)
        : HTMLMediaElement(document)
        , m_hasPendingSrc(false)
    {
        VideoPlayer* player = new VideoPlayer(this);
        m_mediaPlayer = player;
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual String* localName()
    {
        return document()->window()->starFish()->staticStrings()->m_videoTagName.localName();
    }

    virtual QualifiedName name()
    {
        return document()->window()->starFish()->staticStrings()->m_videoTagName;
    }

    virtual bool isHTMLVideoElement() const
    {
        return true;
    }

    virtual void didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved);

    String* width()
    {
        return getAttribute(document()->window()->starFish()->staticStrings()->m_width);
    }

    void setWidth(int width)
    {
        setAttribute(document()->window()->starFish()->staticStrings()->m_width, String::fromInt(width));
    }

    String* height()
    {
        return getAttribute(document()->window()->starFish()->staticStrings()->m_height);
    }

    void setHeight(int height)
    {
        setAttribute(document()->window()->starFish()->staticStrings()->m_height, String::fromInt(height));
    }

    unsigned long videoWidth();
    unsigned long videoHeight();
    String* poster();

    void setPoster(String* poster);

    VideoPlayer* videoPlayer()
    {
        STARFISH_ASSERT(m_mediaPlayer && m_mediaPlayer->isVideoPlayer());
        return (VideoPlayer*)m_mediaPlayer;
    }

protected:
    bool m_hasPendingSrc;
};
}

#endif
