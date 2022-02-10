/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishMutablePropertyValue__
#define __StarfishMutablePropertyValue__

namespace Starfish {

class MutablePropertyValue : public gc {
public:
    MutablePropertyValue(AtomicString name, String* value)
        : m_name(name)
        , m_value(value)
    {
    }

    AtomicString name() const
    {
        return m_name;
    }

    String* value() const
    {
        return m_value;
    }

    void setName(AtomicString name)
    {
        m_name = name;
    }

    void setValue(String* value)
    {
        m_value = value;
    }

    bool operator==(const MutablePropertyValue& v)
    {
        return name() == v.name() && value()->equals(v.value());
    }

    bool operator!=(const MutablePropertyValue& v)
    {
        return !operator==(v);
    }

private:
    AtomicString m_name;
    String* m_value;
};
} // namespace Starfish

#endif
