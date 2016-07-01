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

#ifndef __StarFishSkewTransformData__
#define __StarFishSkewTransformData__

#include "style/Style.h"

namespace StarFish {

class SkewTransform: public gc {
public:
    SkewTransform(double anglex, double angley)
        : m_angleX(anglex)
        , m_angleY(angley)
    {
    }

    ~SkewTransform()
    {
    }

    void setData(double anglex, double angley)
    {
        m_angleX = anglex;
        m_angleY = angley;
    }

    double angleX() { return m_angleX; }
    double angleY() { return m_angleY; }

    bool operator==(const SkewTransform& o)
    {
        return this->m_angleX == o.m_angleX && this->m_angleY == o.m_angleY;
    }

    bool operator!=(const SkewTransform& o)
    {
        return !operator==(o);
    }

private:
    double m_angleX;
    double m_angleY;
};

}

#endif
