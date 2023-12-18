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

#ifndef __StarfishGridTrackSize__
#define __StarfishGridTrackSize__

#include "GridLength.h"

namespace Starfish {

enum class GridTrackSizeType {
    kLength,
    kMinMax,
    kMinContent,
    kMaxContent,
    kFixedRepeat,
    kAutoRepeat,
};

class GridTrackSize : public gc {
public:
    static String* toStringWithGridLengths(GCVector<GridTrackSize*>* v);

    GridTrackSize(GridTrackSizeType type);

    GridTrackSizeType type() const
    {
        return m_type;
    }

    template <typename T>
    T* as()
    {
        return static_cast<T*>(this);
    }

    bool equals(GridTrackSize* other);

    String* toString();

    virtual void checkComputed(Length curFontSize, Length rootFontSize,
                               Font* font, LayoutSize windowSize,
                               ComputedStyle* cs) = 0;

protected:
    GridTrackSizeType m_type;
};

class GridTrackSizeLength : public GridTrackSize {
public:
    GridTrackSizeLength(const GridLength& length);

    GridLength gridLegnth() const
    {
        return m_length;
    }

    virtual void checkComputed(Length curFontSize, Length rootFontSize,
                               Font* font, LayoutSize windowSize,
                               ComputedStyle* cs);

private:
    GridLength m_length;
};

class GridTrackSizeMinMax : public GridTrackSize {
public:
    GridTrackSizeMinMax(const GridLength& min, const GridLength& max);

    GridLength min() const
    {
        return m_min;
    }

    GridLength max() const
    {
        return m_max;
    }

    virtual void checkComputed(Length curFontSize, Length rootFontSize,
                               Font* font, LayoutSize windowSize,
                               ComputedStyle* cs);

private:
    GridLength m_min;
    GridLength m_max;
};

class GridTrackSizeMinContent : public GridTrackSize {
public:
    GridTrackSizeMinContent();

    virtual void checkComputed(Length curFontSize, Length rootFontSize,
                               Font* font, LayoutSize windowSize,
                               ComputedStyle* cs)
    {
    }
};

class GridTrackSizeMaxContent : public GridTrackSize {
public:
    GridTrackSizeMaxContent();

    virtual void checkComputed(Length curFontSize, Length rootFontSize,
                               Font* font, LayoutSize windowSize,
                               ComputedStyle* cs)
    {
    }
};

class GridTrackSizeRepeat : public GridTrackSize {
public:
    GridTrackSizeRepeat(GridTrackSizeType type,
                        const GCVector<GridTrackSize*>& gridTrackSizes);

    const GCVector<GridTrackSize*>& gridTrackSizes()
    {
        return m_gridTrackSizes;
    }

    virtual void checkComputed(Length curFontSize, Length rootFontSize,
                               Font* font, LayoutSize windowSize,
                               ComputedStyle* cs);

protected:
    GCVector<GridTrackSize*> m_gridTrackSizes;
};

class GridTrackSizeFixedRepeat : public GridTrackSizeRepeat {
public:
    GridTrackSizeFixedRepeat(const GCVector<GridTrackSize*>& gridTrackSizes,
                             uint32_t repeatCount);

    uint32_t repeatCount()
    {
        return m_repeatCount;
    }

private:
    uint32_t m_repeatCount;
};

enum class AutoRepeatType {
    kAutoFit,
    kAutoFill,
};

class GridTrackSizeAutoRepeat : public GridTrackSizeRepeat {
public:
    GridTrackSizeAutoRepeat(const GCVector<GridTrackSize*>& gridTrackSizes,
                            AutoRepeatType autoRepeatType);

    AutoRepeatType autoRepeatType()
    {
        return m_autoRepeatType;
    }

private:
    AutoRepeatType m_autoRepeatType;
};

} // namespace Starfish

#endif
