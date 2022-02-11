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

#ifndef __StarfishMutablePropertyValueList__
#define __StarfishMutablePropertyValueList__

#include "core/util/BloomFilter.h"
#include "core/style/MutablePropertyValue.h"

namespace Starfish {

class MutablePropertyValueList : public gc {
public:
    MutablePropertyValueList()
    {
    }

    MutablePropertyValueList(MutablePropertyValueList&& src)
        : m_values(src.m_values)
        , m_bloomFilter(src.m_bloomFilter)
    {
        src.m_bloomFilter.clear();
    }

    MutablePropertyValueList(const MutablePropertyValueList& src)
    {
        operator=(src);
    }

    void operator=(const MutablePropertyValueList& src)
    {
        m_values = src.m_values;
        m_bloomFilter = src.m_bloomFilter;
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    const GCVector<MutablePropertyValue>& values() const
    {
        return m_values;
    }

    Nullable<String*> property(AtomicString key) const;
    size_t findProperty(AtomicString key) const;
    bool hasProperty(AtomicString key) const
    {
        return findProperty(key) != std::numeric_limits<size_t>::max();
    }
    void removeProperty(AtomicString key);
    void setProperty(AtomicString key, String* value);
    void addPropertyIfNotExists(AtomicString key, String* value);
    void clear()
    {
        m_values.clear();
        m_bloomFilter.clear();
    }

private:
    GCVector<MutablePropertyValue> m_values;
    BloomFilter<12> m_bloomFilter;
};
} // namespace Starfish

#endif
