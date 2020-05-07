/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishMediaPlayerAudio__
#define __StarfishMediaPlayerAudio__

#include "platform/multimedia/MediaPlayer.h"

#include "core/fetch/stream/ReadableStreamChunk.h"
#include "platform/loader/ElementResourceClient.h"

namespace Starfish {
class HTMLMediaElement;
class AudioNode;
class Compositor;
class AudioDownloadClient;

class MediaPlayerAudio : public MediaPlayer {
    friend class AudioDownloadClient;

public:
    MediaPlayerAudio(AudioNode* element);
    MediaPlayerAudio(HTMLMediaElement* element);
    virtual ~MediaPlayerAudio(){};

    static MediaPlayerAudio* create(HTMLMediaElement* element);
    static MediaPlayerAudio* create(AudioNode* element);

    virtual void destroy();
    virtual void play();
    virtual void pause(){};
    virtual void seek(double time){};

    virtual void setBuffer(uint8_t* buffer, uint32_t length);
    virtual void prepare(ResourceURL* url);

    virtual double currentTime()
    {
        return 0;
    }

    virtual double duration()
    {
        return 0;
    }

    virtual void setVolume(double volume){};
    virtual void setMuted(bool muted){};
    virtual void prepareMediaSource(){};

    virtual void didDrawVideo(Compositor* canvas, const LayoutRect& videoRect,
                              const LayoutRect& absVideoRect){};
    virtual void willDrawVideo(Compositor* canvas,
                               const LayoutRect& videoRect){};

protected:
    Resource* m_audioResource{ nullptr };
    ReadableStreamChunk m_audioData;

    virtual void downloadAudioData(ResourceURL* url);
    virtual void onAudioDownloadCompleted();
};
} // namespace Starfish

#endif
#endif
