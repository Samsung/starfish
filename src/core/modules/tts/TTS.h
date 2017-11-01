/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifdef STARFISH_ENABLE_TTS
#ifndef __StarFishTTS__
#define __StarFishTTS__

#include "binding/StarFishHoldable.h"

namespace StarFish {
class TTS : public gc, public StarFishHoldable {
public:
    TTS(StarFish* starFish)
        : StarFishHoldable(starFish)
        , m_isTTSEnabled(false)
    {
        init();
    }

    ~TTS()
    {
    }

    bool isTTSEnabled()
    {
        return m_isTTSEnabled;
    }
    void setTTSEnabled(bool value)
    {
        m_isTTSEnabled = value;
    }

    void speech(String* text);

private:
    void init();
    bool m_isTTSEnabled;
};
}
#endif
#endif
