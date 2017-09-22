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

#if defined(STARFISH_ENABLE_TTS) && !defined(STARFISH_TIZEN_TV)
#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/tts/TTS.h"
#include "core/modules/message_loop/MessageLoop.h"

namespace StarFish {

void TTS::init()
{
#ifdef STARFISH_ENABLE_TEST
    m_isTTSEnable = true;
    if (m_isTTSEnable) {
        String* text = String::createASCIIString("Hi, I am StarFish");
        speech(text);
    }
#endif
}

void TTS::speech(String* text)
{
    STARFISH_ASSERT(isMainThread());
    if (text && text != String::emptyString) {
        m_starFish->messageLoop()->addIdler(
            nullptr,
            [](size_t, void* data) {
                String* text = (String*)data;
                STARFISH_LOG_INFO("TTS speech : %s\n",
                                  text->toUTF8NonGCString().data());
            },
            text);
    }
}
}
#endif
