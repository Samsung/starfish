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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && \
    !defined(__StarFishMediaPlayerTizen__)
#define __StarFishMediaPlayerTizenTV__

#include "MediaPlayerTizen.h"

#include <media/player.h>

namespace StarFish {

class CanvasSurface;
class MediaSource;
class MediaPlayerTizenMediaSourceClient;

class MediaPlayerTizenTV : public MediaPlayerTizen {
public:
    MediaPlayerTizenTV(HTMLMediaElement* element) : MediaPlayerTizen(element)
    {
        m_lastVideoPts = m_lastAudioPts =
            element->defaultPlaybackStartPosition() * 1000;
        m_videoInitSegmentIndex = 0;
        m_audioInitSegmentIndex = 0;
#ifndef NDEBUG
        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           STARFISH_LOG_INFO(
                                               "[TRACE_MSE_GC] "
                                               "MediaPlayerTizenTV::~"
                                               "MediaPlayerTizenTV (%p)\n",
                                               obj);
                                       },
                                       NULL, NULL, NULL);
#endif
    }

    virtual void initDisplay()
    {
    }

    virtual void setNativePlayerDefaultOptions(URL* url);
    virtual void drawVideo(Canvas* canvas, const LayoutRect& videoRect,
                           const LayoutRect& absVideoRect);
    virtual double currentTime();
    virtual void fillVideoBuffer(bool useLock = true);
    virtual void fillAudioBuffer(bool useLock = true);
    virtual void prepareMediaSource();
    void setVideoStreamInfo(size_t initSegmentIndex = 0);
    void setAudioStreamInfo(size_t initSegmentIndex = 0);
    virtual void printNativePlayerError(int errorCode);
    virtual void mediaEndOperation()
    {
        player_stop(m_nativePlayer);
        if (m_activeMediaSource) {
            Locker<Mutex> videoLock(*m_videoBufferMutex);
            Locker<Mutex> audioLock(*m_audioBufferMutex);
            m_lastAudioPts = m_lastVideoPts = 0;
        }
    }
    virtual void seekOperation(int timeInMS);
    virtual void handleSeeked();
    virtual void handleSeekFail();

    uint64_t m_lastVideoPts;
    uint64_t m_lastAudioPts;
    size_t m_videoInitSegmentIndex;
    size_t m_audioInitSegmentIndex;
};
}

#endif
