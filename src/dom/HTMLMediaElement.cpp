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
#include "dom/HTMLMediaElement.h"
#include "dom/TextTrack.h"

namespace StarFish {

HTMLMediaElement::HTMLMediaElement(Document* document)
    : HTMLElement(document)
    , m_textTracks(new TextTrackList())
{
}

TextTrack* HTMLMediaElement::addTextTrack(String* kind, String* label, String* language)
{
    TextTrack::Kind kindEnum = TextTrack::stringToKind(kind);
    if (kindEnum == TextTrack::Kind::InvalidKind) {
        return nullptr;
    }
    TextTrack* textTrack = new TextTrack(kindEnum, label, language);
    m_textTracks->add(textTrack);
    return textTrack;
}

}

#endif
