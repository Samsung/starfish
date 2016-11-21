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

#if defined(STARFISH_TIZEN_TV) && defined (STARFISH_ENABLE_AVPLAY)
#ifndef __StarFishAVPLAY__
#define __StarFishAVPLAY__

#include "dom/binding/ScriptWrappable.h"
#include "platform/window/Window.h"
#include "dom/Document.h"
#include <media/player.h>

namespace StarFish {

class StarFish;
class webapis : public ScriptWrappable {
public:
    webapis(StarFish* starFish);
    StarFish* starFish()
    {
        return m_starFish;
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual Type type()
    {
        return ScriptWrappable::Type::webapisObject;
    }

    avplay* AVPlay()
    {
        return m_avplay;
    }

protected:
    StarFish* m_starFish;
    avplay* m_avplay;
};

class avplay : public ScriptWrappable {
public:
    avplay(StarFish* starFish);
    StarFish* starFish()
    {
        return m_starFish;
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    void open(String* url);
    void prepare();
    void setDisplayRect(double offsetLeft, double offsetTop, double offsetWidth, double offsetHeight);
    void play();
    void close();
    void pause();
    void stop();
    void suspend();
    void restore();


    String* getState();
    double getCurrentTime();
    double getDuration();
    // void setListener(listener);

    void seekTo(double seekTime);
    void setStreamingProperty(String* arg1, String* arg2);
    void prepareAsync(ScriptValue listener);

    virtual Type type()
    {
        return ScriptWrappable::Type::avplayObject;
    }
    void callprepareCallback();
protected:
    StarFish* m_starFish;
    double m_offsetLeft;
    double m_offsetTop;
    double m_offsetWidth;
    double m_offsetHeight;

    player_h m_nativePlayer;
    ScriptValue m_async_prepre;

};

}

#endif
#endif
