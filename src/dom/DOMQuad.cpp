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
#include "dom/DOMRectReadOnly.h"
#include "dom/DOMRect.h"
#include "dom/DOMPoint.h"
#include "dom/DOMQuad.h"

namespace StarFish {

static inline double min4(double a, double b, double c, double d)
{
    return std::min(std::min(a, b), std::min(c, d));
}

static inline double max4(double a, double b, double c, double d)
{
    return std::max(std::max(a, b), std::max(c, d));
}

static inline double saturateInf(double value)
{
    if (std::isinf(value)) {
        return std::signbit(value) ? std::numeric_limits<int>::min()
                                   : std::numeric_limits<int>::max();
    }
    return value;
}

DOMRectReadOnly* DOMQuad::bounds() const
{
    if (m_bounds == nullptr) {
        double left =
            saturateInf(min4(m_p1->x(), m_p2->x(), m_p3->x(), m_p4->x()));
        double top =
            saturateInf(min4(m_p1->y(), m_p2->y(), m_p3->y(), m_p4->y()));
        double right =
            saturateInf(max4(m_p1->x(), m_p2->x(), m_p3->x(), m_p4->x()));
        double bottom =
            saturateInf(max4(m_p1->y(), m_p2->y(), m_p3->y(), m_p4->y()));

        m_bounds = new DOMRectReadOnly(left, top, right - left, bottom - top);
    }
    return m_bounds;
}

DOMQuad::DOMQuad(const DOMPointInit& p1, const DOMPointInit& p2,
                 const DOMPointInit& p3, const DOMPointInit& p4)
    : ScriptWrappable(this)
    , m_p1(new DOMPoint(p1))
    , m_p2(new DOMPoint(p2))
    , m_p3(new DOMPoint(p3))
    , m_p4(new DOMPoint(p4))
{
    m_bounds = nullptr;
}

DOMQuad::DOMQuad(const DOMRectInit& rect)
    : DOMQuad(DOMPointInit(rect.x, rect.y),
              DOMPointInit(rect.x + rect.width, rect.y),
              DOMPointInit(rect.x + rect.width, rect.y + rect.height),
              DOMPointInit(rect.x, rect.y + rect.height))
{
}
}
