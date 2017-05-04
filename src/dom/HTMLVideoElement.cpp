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

#ifdef STARFISH_ENABLE_MULTIMEDIA

#include "StarFish.h"
#include "dom/Document.h"
#include "dom/HTMLVideoElement.h"
#include "platform/multimedia/MediaPlayer.h"
#include "platform/window/Window.h"

namespace StarFish {

String* HTMLVideoElement::localName()
{
    return document()
        ->window()
        ->starFish()
        ->staticStrings()
        ->m_videoTagName.localName();
}

QualifiedName HTMLVideoElement::name()
{
    return document()->window()->starFish()->staticStrings()->m_videoTagName;
}

uint32_t HTMLVideoElement::videoWidth()
{
    return m_mediaPlayer ? m_mediaPlayer->videoWidth()
                         : STARFISH_VIDEO_WIDTH_WHEN_VIDEO_NOT_EXISTS;
}

uint32_t HTMLVideoElement::videoHeight()
{
    return m_mediaPlayer ? m_mediaPlayer->videoHeight()
                         : STARFISH_VIDEO_HEIGHT_WHEN_VIDEO_NOT_EXISTS;
}

void HTMLVideoElement::didAttributeChanged(QualifiedName name, String* old,
                                           String* value, bool attributeCreated,
                                           bool attributeRemoved)
{
    HTMLMediaElement::didAttributeChanged(name, old, value, attributeCreated,
                                          attributeRemoved);

    if (name == document()->window()->starFish()->staticStrings()->m_width ||
        name == document()->window()->starFish()->staticStrings()->m_height) {
        if (frame()) {
            setNeedsLayout();
        }
    }
}

uint32_t HTMLVideoElement::width()
{
    String* widthStr = getAttributeOrEmpty(
        document()->window()->starFish()->staticStrings()->m_width);
    return String::parseInt(widthStr);
}

void HTMLVideoElement::setWidth(uint32_t width)
{
    setAttribute(document()->window()->starFish()->staticStrings()->m_width,
                 String::fromInt(width));
}

uint32_t HTMLVideoElement::height()
{
    String* heightStr = getAttributeOrEmpty(
        document()->window()->starFish()->staticStrings()->m_height);
    return String::parseInt(heightStr);
}

void HTMLVideoElement::setHeight(uint32_t height)
{
    setAttribute(document()->window()->starFish()->staticStrings()->m_height,
                 String::fromInt(height));
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
}

#endif
