/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishIntersectionObserverEntry__
#define __StarfishIntersectionObserverEntry__

#include "binding/ScriptWrappable.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMRectInit.h"
#include "core/dom/DOMRectReadOnly.h"

namespace Starfish {

class Element;

struct IntersectionObserverEntryInit {
public:
    IntersectionObserverEntryInit()
        : m_time(0)
        , m_isIntersecting(false)
        , m_intersectionRatio(0)
        , m_target(nullptr)
    {
    }

    // Define getter/setters
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, time, Time);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(Nullable<DOMRectInit>, rootBounds,
                                      RootBounds);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(DOMRectInit, boundingClientRect,
                                      BoundingClientRect);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(DOMRectInit, intersectionRect,
                                      IntersectionRect);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(bool, isIntersecting, IsIntersecting);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, intersectionRatio,
                                      IntersectionRatio);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(Element*, target, Target);

    // Define memebers
    DEFINE_MEMBER_WITH_HASFLAG(double, time, Time);
    DEFINE_MEMBER_WITH_HASFLAG(Nullable<DOMRectInit>, rootBounds, RootBounds);
    DEFINE_MEMBER_WITH_HASFLAG(DOMRectInit, boundingClientRect,
                               BoundingClientRect);
    DEFINE_MEMBER_WITH_HASFLAG(DOMRectInit, intersectionRect, IntersectionRect);
    DEFINE_MEMBER_WITH_HASFLAG(bool, isIntersecting, IsIntersecting);
    DEFINE_MEMBER_WITH_HASFLAG(double, intersectionRatio, IntersectionRatio);
    DEFINE_MEMBER_WITH_HASFLAG(Element*, target, Target);
};

class IntersectionObserverEntry final : public ScriptWrappable {
public:
    IntersectionObserverEntry(
        ExecutionContext* executionContext,
        const IntersectionObserverEntryInit& intersectionObserverEntryInit);

    virtual ScriptBindingInstance* scriptBindingInstance() override;

    void init(ScriptBindingInstance*, void*) override;

    bool isIntersectionObserverEntry() const override;

    double time() const
    {
        return m_time;
    }

    DOMRectReadOnly* rootBounds() const
    {
        return m_rootBounds;
    }

    DOMRectReadOnly* boundingClientRect() const
    {
        return m_boundingClientRect;
    }

    DOMRectReadOnly* intersectionRect() const
    {
        return m_intersectionRect;
    }

    bool isIntersecting() const
    {
        return m_isIntersecting;
    }

    double intersectionRatio() const
    {
        return m_intersectionRatio;
    }

    Element* target() const
    {
        return m_target;
    }

private:
    void initialize(const IntersectionObserverEntryInit& init);

    ExecutionContext* m_executionContext = nullptr;

    double m_time = 0.0;
    DOMRectReadOnly* m_rootBounds = nullptr;
    DOMRectReadOnly* m_boundingClientRect = nullptr;
    DOMRectReadOnly* m_intersectionRect = nullptr;
    bool m_isIntersecting = false;
    double m_intersectionRatio = 0.0;
    Element* m_target = nullptr;
};
} // namespace Starfish

#endif
