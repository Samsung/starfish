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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "Profiling.h"

#if defined(STARFISH_ANDROID)
#include <sys/time.h>
#else
#include <sys/timeb.h>
#endif

namespace StarFish {

uint64_t tickCount()
{
    struct timeval gettick;
    unsigned int tick;
    int ret;
    gettimeofday(&gettick, NULL);

    tick = gettick.tv_sec * 1000 + gettick.tv_usec / 1000;
    return tick;
}

uint64_t longTickCount()
{
    struct timeval gettick;
    unsigned int tick;
    int ret;
    gettimeofday(&gettick, NULL);

    tick = gettick.tv_sec * 1000000 + gettick.tv_usec;
    return tick;
}

uint64_t timestamp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

// #define STARFISH_PROFILER_TIMER_PRINT_TOTAL 1

ProfilerTimer::~ProfilerTimer()
{
    uint64_t end = longTickCount();
    float time = (float)((end - m_start) / 1000.f);
    STARFISH_LOG_INFO("did %s in %f ms\n", m_msg, time);
#ifdef STARFISH_PROFILER_TIMER_PRINT_TOTAL
    AtomicString aTag = AtomicString::createAtomicString(m_starFish, m_msg);
    float accumTime = m_starFish->profileRecode(aTag.string()) + time;
    STARFISH_LOG_INFO("in total, did %s in %f ms\n", m_msg, accumTime);
    m_starFish->updateProfileRecord(aTag.string(), accumTime);
#endif
}
}
