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

    tts->setTTSEnable((vconf_ret == 0 && at == 1));
}

void TTS::init()
{
    int at = 0;
    int vconf_ret = vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, &at);

    m_isTTSEnable = (vconf_ret == 0 && at == 1);

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
