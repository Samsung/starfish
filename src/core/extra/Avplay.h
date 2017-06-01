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

#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
#ifndef __StarFishAvplay__
#define __StarFishAvplay__

#include "binding/ScriptWrappable.h"
#include "binding/StarFishHoldable.h"
#include <media/player.h>

namespace StarFish {

class StarFish;

class Avplay : public ScriptWrappable, public StarFishHoldable {
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

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override
    {
        scriptObject()->set__proto__(
            fetchData(instance)->fnAvplay()->protoType());
    }

    virtual bool isAvplay() const override
    {
        return true;
    }

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
