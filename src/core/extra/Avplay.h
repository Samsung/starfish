/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
#ifndef __StarFishAvplay__
#define __StarFishAvplay__

#include "binding/ScriptWrappable.h"
#include "binding/StarFishHoldable.h"
#include <media/player.h>

namespace StarFish {

class StarFish;

#define AVPLAY_APIS(F)      \
    F(open)                 \
    F(prepare)              \
    F(setDisplayRect)       \
    F(play)                 \
    F(close)                \
    F(pause)                \
    F(stop)                 \
    F(suspend)              \
    F(restore)              \
    F(getState)             \
    F(getCurrentTime)       \
    F(getDuration)          \
    F(setStreamingProperty) \
    F(prepareAsync)         \
    F(setListener)          \
    F(seekTo)

class Avplay : public gc, public StarFishHoldable {
public:
    enum AVPLAY_CALLBACK_TYPE {
        prepare_async_CALLBACK,
        onbufferingstart_CALLBACK,
        onbufferingprogress_CALLBACK,
        onbufferingcomplete_CALLBACK,
        oncurrentplaytime_CALLBACK,
        onevent_CALLBACK,
        onerror_CALLBACK,
        onsubtitlechange_CALLBACK,
        ondrmevent_CALLBACK,
        onstreamcompleted_CALLBACK
    };

    Avplay(StarFish* starFish);
    ~Avplay();

    void open(String* url);
    void prepare();
    void setDisplayRect(double offsetLeft, double offsetTop, double offsetWidth,
                        double offsetHeight);
    void play();
    void close();
    void pause();
    void stop();
    void suspend();
    void restore();

    String* getState();
    double getCurrentTime();
    double getDuration();

    void seekTo(double seekTime);
    void setStreamingProperty(String* arg1, String* arg2);
    void prepareAsync(ScriptValue listener);
    void setListener(ScriptValue listener);

    void callJSCallback(AVPLAY_CALLBACK_TYPE type);

    void setBufferingPercent(int percent)
    {
        m_bufferingPercent = percent;
    }

protected:
    double m_offsetLeft;
    double m_offsetTop;
    double m_offsetWidth;
    double m_offsetHeight;

    int m_bufferingPercent;

    player_h m_nativePlayer;

    ScriptValue m_prepare_async;
    ScriptValue m_listener;
};
}

#endif
#endif
