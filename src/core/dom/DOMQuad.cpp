/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#include "StarFishConfig.h"
#include "core/dom/DOMRect.h"
#include "core/dom/DOMRectReadOnly.h"
#include "core/dom/DOMPoint.h"
#include "core/dom/DOMQuad.h"
#include "core/dom/Document.h"

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

DOMRect* DOMQuad::getBounds() const
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

        m_bounds = new DOMRect(m_scriptBindingInstance->ownerDocument(), left,
                               top, right - left, bottom - top);
    }
    return m_bounds;
}

DOMQuad::DOMQuad(Document* document, const DOMPointInit& p1,
                 const DOMPointInit& p2, const DOMPointInit& p3,
                 const DOMPointInit& p4)
    : ScriptWrappable(this)
    , m_scriptBindingInstance(document->scriptBindingInstance())
    , m_p1(new DOMPoint(document, p1))
    , m_p2(new DOMPoint(document, p2))
    , m_p3(new DOMPoint(document, p3))
    , m_p4(new DOMPoint(document, p4))
{
    m_bounds = nullptr;
}
}
