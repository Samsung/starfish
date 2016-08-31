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

#define STARFISH_VIDEO_WIDTH_WHEN_VIDEO_NOT_EXISTS 300
#define STARFISH_VIDEO_HEIGHT_WHEN_VIDEO_NOT_EXISTS 150

namespace StarFish {

class HTMLVideoElement : public HTMLMediaElement {
public:
    HTMLVideoElement(Document* document)
        : HTMLMediaElement(document)
    {
        m_videoSurface = CanvasSurface::create(document->window(), STARFISH_VIDEO_WIDTH_WHEN_VIDEO_NOT_EXISTS, STARFISH_VIDEO_HEIGHT_WHEN_VIDEO_NOT_EXISTS);
        VideoPlayer* player = new VideoPlayer(this);
#ifdef STARFISH_TIZEN_MOBILE
        player->setDisplayArea(m_videoSurface);
#endif
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

    unsigned long width();
    unsigned long height();
    unsigned long videoWidth();
    unsigned long videoHeight();
    String* poster();

    void setWidth(unsigned long width);
    void setHeight(unsigned long height);
    void setPoster(String* poster);

    CanvasSurface* videoSurface()
    {
        return m_videoSurface;
    }

#ifdef STARFISH_TIZEN_TV
    void setPlayerDisplayArea(int x, int y, int width, int height)
    {
        videoPlayer()->setDisplayArea(x, y, width, height);
    }
#endif

protected:
    VideoPlayer* videoPlayer()
    {
        STARFISH_ASSERT(m_mediaPlayer && m_mediaPlayer->isVideoPlayer());
        return (VideoPlayer*)m_mediaPlayer;
    }

protected:
    CanvasSurface* m_videoSurface;
    bool m_hasPendingSrc;
};
}

#endif
