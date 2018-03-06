/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
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
    m_isTTSEnabled = true;
    if (m_isTTSEnabled) {
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
