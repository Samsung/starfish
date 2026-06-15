/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPAnimationDomain__)
#define __StarfishCDPAnimationDomain__

#include <string>

namespace Starfish {

class CDPDispatcher;
class CDPCommand;
class WebView;

// Animation domain.
//
// enable/disable toggle the session flag that gates animationCreated/
// animationStarted emission. getPlaybackRate/setPlaybackRate store and report a
// global playback rate (set->get round-trips). The engine drives CSS
// animations/transitions off the real wall clock (tickCount) and exposes no
// global time-scale, pause, or seek hook, so the stored playback rate is not
// applied to running animations and setPaused/seekAnimations/getCurrentTime/
// releaseAnimations/resolveAnimation/setTiming are acked (no per-instance
// control). animationCreated/animationStarted are real: emitted from the
// engine's keyframe-animation start hook (see emitAnimationStarted) when
// Animation is enabled. Handlers run on the main thread.
class AnimationDomain {
public:
    AnimationDomain(CDPDispatcher* d)
        : m_dispatcher(d)
    {
    }
    void processMessage(CDPCommand& cmd, const std::string& method);

    // Engine bridge (main thread). Called when a CSS keyframe animation starts
    // applying to an element. Emits Animation.animationCreated followed by
    // Animation.animationStarted on the session owning `webView`, if Animation
    // is enabled. animationName is the @keyframes name. No-op if the WebView
    // has no attached/enabled session.
    void emitAnimationStarted(WebView* webView,
                              const std::string& animationName,
                              double durationMs);

private:
    CDPDispatcher* m_dispatcher;
};

} // namespace Starfish

#endif
