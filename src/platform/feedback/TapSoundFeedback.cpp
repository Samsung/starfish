/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "platform/feedback/TapSoundFeedback.h"

#if !defined(STARFISH_TIZEN)

namespace Starfish {

// The platform tap-sound service (libfeedback) is Tizen-only; on every other
// target there is nothing to play (and no <dlfcn.h> on Windows).
void playPlatformTapSoundFeedback()
{
}

} // namespace Starfish

#else

#include <dlfcn.h>

namespace Starfish {

namespace {

    // Tizen feedback API, loaded at runtime via dlopen so there is no
    // build-time dependency on feedback-devel (not provisioned in every
    // buildroot) and desktop builds degrade to a logged no-op. See
    // <feedback.h>: feedback_play_type_by_name takes BOTH the type and the
    // pattern as strings -- the spellings of the feedback_type_e /
    // feedback_pattern_e enumerators themselves, which libfeedback matches
    // against its own name tables -- so no enum values need to be mirrored
    // here. Passing the enum value instead makes libfeedback strlen() the
    // integer as a pointer and the process dies in strlen.
    static const char* const kFeedbackTypeSound = "FEEDBACK_TYPE_SOUND";
    static const char* const kFeedbackPatternTap = "FEEDBACK_PATTERN_TAP";

    typedef int (*feedback_initialize_fn)(void);
    typedef int (*feedback_play_type_by_name_fn)(const char* type,
                                                 const char* pattern);

    // Owns the dlopen handle for the process lifetime: the function pointer
    // handed out below points into libfeedback, so the library must stay
    // mapped for as long as it can be called -- which is until exit.
    void* s_feedbackHandle = nullptr;

    feedback_play_type_by_name_fn loadFeedbackPlayFn()
    {
        void* handle = dlopen("libfeedback.so.0", RTLD_LAZY | RTLD_LOCAL);
        if (handle == nullptr) {
            return nullptr;
        }
        feedback_initialize_fn init =
            (feedback_initialize_fn)dlsym(handle, "feedback_initialize");
        feedback_play_type_by_name_fn play =
            (feedback_play_type_by_name_fn)dlsym(handle,
                                                 "feedback_play_type_by_name");
        if (init == nullptr || play == nullptr || init() != 0) {
            dlclose(handle);
            return nullptr;
        }
        // Never dlclose'd: the feedback session stays open and taps recur as
        // long as the page is interactive.
        s_feedbackHandle = handle;
        return play;
    }

} // namespace

void playPlatformTapSoundFeedback()
{
    static feedback_play_type_by_name_fn s_play = loadFeedbackPlayFn();
    if (s_play == nullptr) {
        STARFISH_LOG_INFO(
            "TapSoundFeedback: platform feedback unavailable "
            "(tap sound skipped)");
        return;
    }
    int ret = s_play(kFeedbackTypeSound, kFeedbackPatternTap);
    STARFISH_LOG_INFO("TapSoundFeedback: play tap sound ret:%d", ret);
}

} // namespace Starfish

#endif
