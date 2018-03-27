/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishTextOverflowData__
#define __StarFishTextOverflowData__

#include "core/util/String.h"
#include "core/style/Length.h"
#include "core/style/Style.h"

namespace StarFish {

enum TextOverflowValue {
    TextOverflowClipValue = 1, // Default value
    TextOverflowEllipsisValue = 3,
};

// TODO needs to implement pair value support(CSS4)
class TextOverflowData : public gc {
public:
    TextOverflowData(TextOverflowValue v = TextOverflowClipValue)
        : m_enumValue(v)
    {
    }

    TextOverflowData(String* v)
        : m_stringValue(v)
    {
    }

    bool hasClipValue() const
    {
        return (size_t)m_enumValue == (size_t)TextOverflowClipValue;
    }

    bool hasEllipsisValue() const
    {
        return (size_t)m_enumValue == (size_t)TextOverflowEllipsisValue;
    }

    bool hasStringValue() const
    {
        return !hasEnumValue();
    }

    String* stringValue() const
    {
        STARFISH_ASSERT(!hasEnumValue());
        return m_stringValue;
    }

    bool operator==(const TextOverflowData& data)
    {
        if (hasEnumValue() && data.hasEnumValue()) {
            return hasClipValue() && data.hasClipValue();
        } else if (!hasEnumValue() && !data.hasEnumValue()) {
            return stringValue()->equals(data.stringValue());
        } else {
            return false;
        }
    }

    bool operator!=(const TextOverflowData& data)
    {
        return !operator==(data);
    }

private:
    bool hasEnumValue() const
    {
        if ((size_t)m_stringValue & 1) {
            return true;
        }
        return false;
    }

    union {
        TextOverflowValue m_enumValue;
        String* m_stringValue;
    };
};
}

#endif
