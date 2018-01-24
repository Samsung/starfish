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

ProfilerTimer::~ProfilerTimer()
{
    uint64_t end = longTickCount();
    float time = (float)((end - m_start) / 1000.f);
    STARFISH_LOG_INFO("did %s in %f ms\n", m_msg, time);
    AtomicString aTag = AtomicString::createAtomicString(m_starFish, m_msg);
    float accumTime = m_starFish->profileRecode(aTag.string()) + time;
    STARFISH_LOG_INFO("in total, did %s in %f ms\n", m_msg, accumTime);
    m_starFish->updateProfileRecord(aTag.string(), accumTime);
}
}
