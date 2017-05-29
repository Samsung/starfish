/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishPlatformTimer__
#define __StarFishPlatformTimer__

namespace StarFish {

class PlatformTimer : public gc {
    friend class StarFish;
    friend class Window;

public:
    PlatformTimer(StarFish* sf);
    size_t addTimer(double delay, WindowSetTimeoutHandler handler, void* data,
                    bool repetitive);
    void removeTimer(size_t reqID);

    size_t addAnimator(WindowSetTimeoutHandler handler, void* data);
    void removeAnimator(size_t reqID);

    // bool hasPendingIdler()
    // {
    //     return m_idlers.size();
    // }

    void clear();

protected:
    StarFish* m_starFish;

    int32_t m_timeoutCounter;
    GCUnorderedMap<int32_t, void*> m_timeoutHandler;

    int32_t m_requestAnimationFrameCounter;
    GCUnorderedMap<int32_t, void*> m_requestAnimationFrameHandler;
};
}

#endif
