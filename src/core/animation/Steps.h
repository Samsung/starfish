/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd

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

#ifndef __StarfishSteps__
#define __StarfishSteps__

#include "core/animation/TimingFunction.h"

namespace Starfish {

class Steps : public TimingFunction {
public:
    enum class StepPosition { START, END };

    static Steps* createSteps(size_t numberOfSteps, StepPosition position)
    {
        return new Steps(numberOfSteps, position);
    }

    double getValue(double t) override
    {
        if (m_position == StepPosition::END) {
            return floor(m_numberOfSteps * t) / m_numberOfSteps;
        }
        return std::min(1.0,
                        (floor(m_numberOfSteps * t) + 1) / m_numberOfSteps);
    }

    TimingFunctionType timingFunctionType() override
    {
        return TimingFunctionType::STEPS;
    }

    static void* operator new(size_t size)
    {
        return GC_MALLOC_ATOMIC(size);
    }

    bool isSteps() const override
    {
        return true;
    }

    String* toString() const override
    {
        StringBuilder builder;
        builder.appendString("steps(");
        builder.appendString(String::fromInt(m_numberOfSteps));
        if (m_position == StepPosition::END) {
            builder.appendString(", end)");
        } else {
            builder.appendString(", start)");
        }
        return builder.finalize();
    }

    bool operator==(const Steps& b) const
    {
        if (m_position != b.m_position) {
            return false;
        }
        if (m_numberOfSteps != b.m_numberOfSteps) {
            return false;
        }
        return true;
    }

private:
    Steps(size_t numberOfSteps, StepPosition position)
        : m_position(position)
        , m_numberOfSteps(numberOfSteps)
    {
    }

    StepPosition m_position;
    size_t m_numberOfSteps;
};
} // namespace Starfish
#endif
