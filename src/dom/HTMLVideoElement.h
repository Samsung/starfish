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
#include "extra/MediaPlayer.h"

#define STARFISH_VIDEO_WIDTH_WHEN_VIDEO_NOT_EXISTS 300
#define STARFISH_VIDEO_HEIGHT_WHEN_VIDEO_NOT_EXISTS 150

namespace StarFish {

class HTMLVideoElement;

class VideoPlayer : public MediaPlayer {
public:
    VideoPlayer(HTMLVideoElement* videoElement)
        : MediaPlayer()
        , m_videoElement(videoElement) { }

    virtual void onPrepared(bool hasError);
    virtual void onPlayFinished() { }
protected:
    HTMLVideoElement* m_videoElement;
};

class HTMLVideoElement : public HTMLMediaElement {
public:
    HTMLVideoElement(Document* document)
        : HTMLMediaElement(document)
    {
        m_videoSurface = CanvasSurface::create(document->window(), STARFISH_VIDEO_WIDTH_WHEN_VIDEO_NOT_EXISTS, STARFISH_VIDEO_HEIGHT_WHEN_VIDEO_NOT_EXISTS);
        m_player = new VideoPlayer(this);
        m_hasPendingRequest = false;
        m_live = false;
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
    virtual void didNodeInsertedToDocumenTree();
    virtual void didNodeRemovedFromDocumenTree();

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

    void loadSrc();
    void loadSrc(String* src);

private:
    CanvasSurface* m_videoSurface;
    VideoPlayer* m_player;
    bool m_hasPendingRequest;
    bool m_live;
};
}

#endif
