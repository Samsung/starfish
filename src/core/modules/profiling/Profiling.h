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

#ifndef __StarfishProfiling__
#define __StarfishProfiling__

#include <unordered_map>

namespace Starfish {

uint64_t tickCount();     // increase 1000 by 1 second
uint64_t longTickCount(); // increase 1000000 by 1 second
uint64_t timestamp();     // increase 1000 by 1 second

enum class ProfileKind {
    kStyle,
    kLayout,
    kPaint,
    kScript,
    kMISC,
};

class ProfilerTimer {
public:
    ProfilerTimer(const char* msg);
    ProfilerTimer(ProfileKind kind, const char* msg);
    ~ProfilerTimer();

protected:
    uint64_t m_start;
    const char* m_msg;
    bool m_needToRecord;
    ProfileKind m_kind;
};

class LongTaskFinder {
public:
    LongTaskFinder(const char* msg, size_t loggingTimeInMS = 1)
    {
        m_loggingTime = loggingTimeInMS;
        m_start = longTickCount();
        m_msg = msg;
    }
    ~LongTaskFinder();

protected:
    size_t m_loggingTime;
    uint64_t m_start;
    const char* m_msg;
};

class Profiler {
public:
    Profiler();
    ~Profiler();
    Profiler(const Profiler& other) = delete;
    Profiler& operator=(const Profiler& other) = delete;

    void Update(ProfileKind kind, float elapsedTimeInMS);

    void start()
    {
        m_isStarted = true;
        init();
    }

    void stop()
    {
        m_isStarted = false;
    }

    bool isStarted()
    {
        return m_isStarted;
    }

    void report();

private:
    void init();

    bool m_isStarted = false;
    float m_totalElapsedTime = 0.0f;
    std::unordered_map<unsigned, float> m_records;
};

#ifdef STARFISH_ENABLE_PROFILING
#define STARFISH_ENABLE_PROFILE_TIMER
#endif

#ifdef STARFISH_ENABLE_PROFILE_TIMER
#define INSTALL_PROFILE_TIMER(msg) ProfilerTimer _p(msg);
#else
#define INSTALL_PROFILE_TIMER(msg)
#endif

#ifdef STARFISH_ENABLE_PROFILE_TIMER
#define INSTALL_PROFILE_TIMER(msg) ProfilerTimer _p(msg);
#define INSTALL_RECORDABLE_PROFILE_TIMER(kind, msg) ProfilerTimer _p(kind, msg);
#else
#define INSTALL_PROFILE_TIMER(msg)
#define INSTALL_RECORDABLE_PROFILE_TIMER(kind, msg)
#endif
} // namespace Starfish

#endif
