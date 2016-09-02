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

#include "StarFishConfig.h"
#include "Profiling.h"

namespace StarFish {

namespace {
    class __GET_TICK_COUNT {
    public:
        __GET_TICK_COUNT()
        {
            if (gettimeofday(&tv_, NULL) != 0)
                throw 0;
        }
        timeval tv_;
    };
    __GET_TICK_COUNT timeStart;
}

uint64_t tickCount()
{
    static time_t secStart = timeStart.tv_.tv_sec;
    static time_t usecStart = timeStart.tv_.tv_usec;
    timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec - secStart) * 1000 + (tv.tv_usec - usecStart);
}

}
