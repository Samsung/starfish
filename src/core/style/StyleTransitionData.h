/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#ifndef __StarFishStyleTransitionData__
#define __StarFishStyleTransitionData__

#include "core/style/Style.h"

namespace StarFish {

class StyleTransitionData : public gc {
public:
    StyleTransitionData()
        : m_property(TransitionPropertyValue::TransitionPropertyAllValue)
        , m_duration(0)
    {
    }

    ~StyleTransitionData()
    {
    }

    TransitionPropertyValue transitionProperty()
    {
        return m_property;
    }

    void setTransitionProperty(TransitionPropertyValue property)
    {
        m_property = property;
    }

    CSSTime transitionDuration()
    {
        return m_duration;
    }

    void setTransitionDuration(CSSTime duration)
    {
        m_duration = duration;
    }

private:
    friend inline bool operator==(const StyleTransitionData& a,
                                  const StyleTransitionData& b);
    friend inline bool operator!=(const StyleTransitionData& a,
                                  const StyleTransitionData& b);

    TransitionPropertyValue m_property;
    CSSTime m_duration;
};

bool operator==(const StyleTransitionData& a, const StyleTransitionData& b)
{
    if (a.m_property != b.m_property) {
        return false;
    }

    if (a.m_duration != b.m_duration) {
        return false;
    }

    return true;
}

bool operator!=(const StyleTransitionData& a, const StyleTransitionData& b)
{
    return !operator==(a, b);
}
}

#endif
