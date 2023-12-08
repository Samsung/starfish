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

#ifndef __StarfishTimingFunction__
#define __StarfishTimingFunction__

namespace Starfish {

class CubicBezier;
class Steps;

class TimingFunction : public gc {
public:
    enum class TimingFunctionType { CUBIC_BEZIER, STEPS };
    virtual ~TimingFunction()
    {
    }
    virtual double getValue(double x) = 0;
    virtual String* toString() const = 0;
    virtual TimingFunctionType timingFunctionType() = 0;
    virtual bool isCubicBezier() const
    {
        return false;
    }
    virtual bool isSteps() const
    {
        return false;
    }
    CubicBezier* asCubicBezier() const
    {
        return (CubicBezier*)this;
    }
    Steps* asSteps() const
    {
        return (Steps*)this;
    }
    bool operator==(const TimingFunction& b) const;
    bool operator!=(const TimingFunction& b) const;
};
} // namespace Starfish
#endif
