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
#if !defined(STARFISH_TIZEN)

#ifndef __StarfishMediaPlayerAudioMock__
#define __StarfishMediaPlayerAudioMock__

#include "platform/multimedia/MediaPlayerAudio.h"

#include <chrono>

namespace Starfish {
class HTMLMediaElement;
class AudioNode;
class AudioBufferData;
class AudioBus;
class AudioOutputDevice;
class MediaAudioPlaybackState;
class MediaAudioDecodeWork;
class MediaPlayerAudioLinux;
class WebView;
class Window;

class MediaAudioDecodeJob : public gc {
public:
    MediaAudioDecodeJob(MediaPlayerAudioLinux* player, WebView* webView,
                        Window* window, MediaAudioDecodeWork* work);
    Window* window() const;
    void cancel();
    void complete();
    void discard();

private:
    MediaPlayerAudioLinux* m_player;
    WebView* m_webView;
    Window* m_window;
    MediaAudioDecodeWork* m_work;
};

class MediaPlayerAudioLinux : public MediaPlayerAudio {
    friend class MediaAudioDecodeJob;

public:
    MediaPlayerAudioLinux(AudioNode* element);
    MediaPlayerAudioLinux(HTMLMediaElement* element);
    virtual ~MediaPlayerAudioLinux(){};

    virtual void destroy() override;
    virtual void play() override;
    void pause() override;
    void seek(double time) override;
    void setLoop(bool loop) override;
    MediaAudioPlaybackState* audioPlaybackState() override;

    void prepare(ResourceURL* url) override;
    void onAudioDownloadCompleted() override;

    double currentTime() override;

    double duration() override;

    void setVolume(double volume) override;
    void setMuted(bool muted) override;
    void setPlaybackRate(double rate) override;
    virtual void prepareMediaSource() override {};

private:
    bool acceptsEncodedSize(size_t size) const override
    {
        return size <= 32 * 1024 * 1024;
    }
    void didDecodeAudio(AudioBufferData* pcm);
    void playbackTick();

    MediaAudioDecodeJob* m_decodeJob{ nullptr };
    MediaAudioPlaybackState* m_playbackState{ nullptr };
    AudioOutputDevice* m_outputDevice{ nullptr };
    AudioBus* m_outputBus{ nullptr };
    uint64_t m_nextOutputFrame{ 0 };
    std::chrono::steady_clock::time_point m_outputClockStart;
    double m_outputPosition{ 0 };
    unsigned m_tickCount{ 0 };
};
} // namespace Starfish

#endif
#endif
#endif
