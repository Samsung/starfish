/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishFlexBasisData__
#define __StarFishFlexBasisData__

#include "core/style/Length.h"

namespace StarFish {
class FlexBasisData {
    enum Type { Content, Width };

    Type m_type;
    Length m_width;

public:
    FlexBasisData()
        : m_type(Content)
        , m_width(Length())
    {
    }

    FlexBasisData(bool isContent, Length width = Length())
        : m_width(width)
    {
        if (isContent) {
            m_type = Content;
        } else {
            m_type = Width;
        }
    }

    bool isContent() const
    {
        return m_type == Content;
    }

    bool isWidth() const
    {
        return m_type == Width;
    }

    Length width() const
    {
        return m_width;
    }

    bool operator==(const FlexBasisData& o)
    {
        if (m_type != o.m_type) {
            return false;
        }

        if (m_type == Content) {
            return true;
        }

        return width() == o.width();
    }

    bool operator!=(const FlexBasisData& o)
    {
        return !operator==(o);
    }
};
}

#endif
