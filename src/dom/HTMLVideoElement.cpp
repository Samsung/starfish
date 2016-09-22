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

#ifdef STARFISH_ENABLE_MULTIMEDIA

#include "StarFishConfig.h"
#include "dom/HTMLVideoElement.h"

#define STARFISH_VIDEO_WIDTH_WHEN_VIDEO_NOT_EXISTS 300
#define STARFISH_VIDEO_HEIGHT_WHEN_VIDEO_NOT_EXISTS 150

namespace StarFish {

unsigned long HTMLVideoElement::videoWidth()
{
    if (m_readyState > HTMLMediaElement::HAVE_NOTHING) {
        return videoPlayer()->width();
    }
    return STARFISH_VIDEO_WIDTH_WHEN_VIDEO_NOT_EXISTS;
}

unsigned long HTMLVideoElement::videoHeight()
{
    if (m_readyState > HTMLMediaElement::HAVE_NOTHING) {
        return videoPlayer()->height();
    }
    return STARFISH_VIDEO_HEIGHT_WHEN_VIDEO_NOT_EXISTS;
}

void HTMLVideoElement::didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved)
{
    HTMLMediaElement::didAttributeChanged(name, old, value, attributeCreated, attributeRemoved);

    if (name == document()->window()->starFish()->staticStrings()->m_width
        || name == document()->window()->starFish()->staticStrings()->m_height) {
        if (frame()) {
            setNeedsLayout();
        }
    }
}

String* HTMLVideoElement::poster()
{
    // TODO
    return String::emptyString;
}

void HTMLVideoElement::setPoster(String* poster)
{
    // TODO
}

void VideoPlayer::onPrepared(URL* url)
{
    if (!url) {
        return;
    }
    if (m_videoElement) {
        m_videoElement->setReadyState(HTMLMediaElement::HAVE_METADATA);
        m_videoElement->setNeedsLayout();
    }

}

void VideoPlayer::onUnprepared()
{
    if (m_videoElement) {
        m_videoElement->setReadyState(HTMLMediaElement::HAVE_NOTHING);
    }
}

void VideoPlayer::onPlayFinished(URL* url)
{

}

}

#endif
