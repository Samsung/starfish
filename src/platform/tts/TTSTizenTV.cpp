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

#if defined(STARFISH_ENABLE_TTS) && defined(STARFISH_TIZEN_TV)

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/tts/TTS.h"
#include <Elementary.h>
#include "core/modules/message_loop/MessageLoop.h"
#include <vconf.h>
#include <vconf-internal-keys-menu-system.h>

namespace StarFish {

static void onAccessibilityChanged(keynode_t* keynodeName, void* data)
{
    STARFISH_LOG_INFO("onAccessibilityChanged");
    // Do not free data
    TTS* tts = (TTS*)data;
    int at = 0;
    int vconf_ret = vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, &at);

    tts->setTTSEnabled((vconf_ret == 0 && at == 1));
}

void TTS::init()
{
    int at = 0;
    int vconf_ret = vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, &at);

    m_isTTSEnabled = (vconf_ret == 0 && at == 1);

    // Add listener
    vconf_notify_key_changed(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS,
                             onAccessibilityChanged, this);
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
                elm_access_say((char*)text->toUTF8NonGCString().data());
            },
            text);
    }
}
}
#endif
