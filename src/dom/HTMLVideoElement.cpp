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

namespace StarFish {

unsigned long HTMLVideoElement::width()
{
    // TODO
    return 0;
}

unsigned long HTMLVideoElement::height()
{
    // TODO
    return 0;
}

unsigned long HTMLVideoElement::videoWidth()
{
    // TODO
    return 0;
}

unsigned long HTMLVideoElement::videoHeight()
{
    // TODO
    return 0;
}

String* HTMLVideoElement::poster()
{
    // TODO
    return String::emptyString;
}

void HTMLVideoElement::setWidth(unsigned long width)
{
    // TODO
}

void HTMLVideoElement::setHeight(unsigned long height)
{
    // TODO
}

void HTMLVideoElement::setPoster(String* poster)
{
    // TODO
}

void HTMLVideoElement::didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated, attributeRemoved);
    if (name == document()->window()->starFish()->staticStrings()->m_src) {
        if (!value->equals(String::emptyString)) {
            loadSrc(value);
        }
    }
}

void HTMLVideoElement::didNodeInsertedToDocumenTree()
{
    HTMLElement::didNodeInsertedToDocumenTree();
    m_live = true;
    if (m_hasPendingRequest) {
        loadSrc();
    } else if (autoplay() && m_player->isReady()) {
        m_player->play();
    }
}

void HTMLVideoElement::didNodeRemovedFromDocumenTree()
{
    HTMLElement::didNodeRemovedFromDocumenTree();
    m_live = false;
    m_player->destroy();
}

void HTMLVideoElement::loadSrc()
{
    loadSrc(src());
}

void HTMLVideoElement::loadSrc(String* src)
{
    if (!m_live) {
        m_hasPendingRequest = true;
        return;
    }
    if (src->equals(String::emptyString)) {
        return;
    }

    STARFISH_ASSERT(m_player);
    STARFISH_ASSERT(m_videoSurface);

    m_player->prepare(document(), m_videoSurface, src);
    m_hasPendingRequest = false;
}

void HTMLVideoElement::play()
{
    if (m_player)
        m_player->play();
}

void VideoPlayer::onPrepared(bool hasError)
{
    if (hasError) {
        // TODO
        return;
    }
    STARFISH_LOG_ERROR("VideoPlayer::onPrepared()\n");
    if (m_videoElement->autoplay() && isReady())
        play();
}

void VideoPlayer::onPlayFinished()
{
    STARFISH_LOG_ERROR("VideoPlayer::onPlayFinished()\n");
}

}

#endif
