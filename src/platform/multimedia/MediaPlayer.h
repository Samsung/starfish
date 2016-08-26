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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined (__StarFishMediaPlayer__)
#define __StarFishMediaPlayer__

#include "platform/canvas/Canvas.h"

#ifdef STARFISH_TIZEN
#include <player.h>
#endif

namespace StarFish {

class Document;
class URL;
class PlayerWindowData;

class MediaPlayer : public gc {
public:
    MediaPlayer();

    void create();

    void prepare(Document* document, CanvasSurface* surface, String* path);
    void destroy();
    void play();
    void pause();
    void stop();

    virtual void onPrepared(bool hasError) { }
    virtual void onPlayFinished() { }

    void setLoop(bool loop) { }
    bool isReady();

protected:
#ifdef STARFISH_TIZEN
    player_h m_player;
#endif
    URL* m_url;
};

}

#endif
