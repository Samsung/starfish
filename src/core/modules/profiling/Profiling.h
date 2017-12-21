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

#ifndef __StarFishProfiling__
#define __StarFishProfiling__

namespace StarFish {

uint64_t tickCount();     // increase 1000 by 1 second
uint64_t longTickCount(); // increase 1000000 by 1 second
uint64_t timestamp();     // increase 1000 by 1 second

class ProfilerTimer {
public:
    ProfilerTimer(StarFish* starFish, const char* msg)
    {
        m_starFish = starFish;
        m_start = longTickCount();
        m_msg = msg;
    }
    ~ProfilerTimer();

protected:
    StarFish* m_starFish;
    uint64_t m_start;
    const char* m_msg;
};

#ifdef STARFISH_ENABLE_PROFILING
#define STARFISH_ENABLE_PROFILE_TIMER
#endif

#ifdef STARFISH_ENABLE_PROFILE_TIMER
#define INSTALL_PROFILE_TIMER(starFish, msg) ProfilerTimer _p(starFish, msg);
#else
#define INSTALL_PROFILE_TIMER(starFish, msg)
#endif
}

#endif
