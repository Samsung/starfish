/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

uint64_t tickCount(); // increase 1000 by 1 second
uint64_t timestamp(); // increase 1000 by 1 second

class Timer {
public:
    Timer(const char* msg)
    {
        m_start = tickCount();
        m_msg = msg;
    }
    ~Timer()
    {
        unsigned long end = tickCount();
        STARFISH_LOG_INFO("did %s in %f ms\n", m_msg, (float)(end - m_start));
        fflush(stdout);
    }

protected:
    unsigned long m_start;
    const char* m_msg;
};
}

#endif
