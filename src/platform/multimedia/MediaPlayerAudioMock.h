/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_WEBAUDIO)
#if defined(STARFISH_USE_MOCK_MEDIAPLAYER) || !defined(STARFISH_TIZEN)

#ifndef __StarfishMediaPlayerAudioMock__
#define __StarfishMediaPlayerAudioMock__

#include "platform/multimedia/MediaPlayerAudio.h"

namespace Starfish {
class HTMLMediaElement;
class AudioNode;

class MediaPlayerAudioMock : public MediaPlayerAudio {
public:
    MediaPlayerAudioMock(AudioNode* element);
    MediaPlayerAudioMock(HTMLMediaElement* element);
    virtual ~MediaPlayerAudioMock(){};

    virtual void destroy() override;
    virtual void play() override;
    virtual void pause() override{};
    virtual void seek(double time) override{};

    void prepare(ResourceURL* url) override;

    virtual double currentTime()
    {
        return 0;
    }

    virtual double duration()
    {
        return 0;
    }

    virtual void setVolume(double volume) override{};
    virtual void setMuted(bool muted) override{};
    virtual void prepareMediaSource() override{};

private:
};
} // namespace Starfish

#endif
#endif
#endif
