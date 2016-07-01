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

#ifndef __StarFishScaleTransformData__
#define __StarFishScaleTransformData__

#include "style/Style.h"

namespace StarFish {

class ScaleTransform: public gc {
public:
    ScaleTransform(double a, double b)
        : m_x(a)
        , m_y(b)
    {
    }

    ~ScaleTransform()
    {
    }

    void setData(double a, double b)
    {
        m_x = a;
        m_y = b;
    }

    double x() { return m_x; }
    double y() { return m_y; }

    bool operator==(const ScaleTransform& o)
    {
        return this->m_x == o.m_x && this->m_y == o.m_y;
    }

    bool operator!=(const ScaleTransform& o)
    {
        return !operator==(o);
    }

private:
    double m_x;
    double m_y;
};

}

#endif
