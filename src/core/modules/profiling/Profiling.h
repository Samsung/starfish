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
