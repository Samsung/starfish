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
#include "TextTrack.h"

namespace StarFish {

void TextTrack::setKind(String* kind)
{
    TextTrack::Kind kindEnum = TextTrack::stringToKind(kind);
    if (!kindEnum == TextTrack::Kind::InvalidKind) {
        m_kind = kindEnum;
    }
}

void TextTrack::setMode(String* mode)
{
    TextTrack::Mode modeEnum = TextTrack::stringToMode(mode);
    if (!modeEnum == TextTrack::Mode::InvalidMode) {
        m_mode = modeEnum;
    }
}

}

#endif
