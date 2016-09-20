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

#include <sys/timeb.h>

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

uint64_t timestamp()
{
    struct timeb timer_msec;
    long long int timestamp_msec;
    ftime(&timer_msec);
    timestamp_msec = ((long long int) timer_msec.time) * 1000ll +
        (long long int) timer_msec.millitm;
    return timestamp_msec;
}

}
