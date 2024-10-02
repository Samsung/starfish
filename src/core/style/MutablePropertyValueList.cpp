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

#include "StarfishConfig.h"
#include "MutablePropertyValueList.h"

namespace Starfish {

void* MutablePropertyValueList::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(MutablePropertyValueList));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(MutablePropertyValueList)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(MutablePropertyValueList, m_values));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(MutablePropertyValueList));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

size_t MutablePropertyValueList::findProperty(AtomicString key) const
{
    if (m_bloomFilter.mayContain(key.string())) {
        for (size_t i = 0; i < m_values.size(); i++) {
            if (m_values[i].name() == key) {
                return i;
            }
        }
    }
    return std::numeric_limits<size_t>::max();
}

Optional<String*> MutablePropertyValueList::property(AtomicString key) const
{
    if (m_bloomFilter.mayContain(key.string())) {
        for (size_t i = 0; i < m_values.size(); i++) {
            if (m_values[i].name() == key) {
                return m_values[i].value();
            }
        }
    }
    return nullptr;
}

void MutablePropertyValueList::removeProperty(AtomicString key)
{
    if (m_bloomFilter.mayContain(key.string())) {
        for (size_t i = 0; i < m_values.size(); i++) {
            if (m_values[i].name() == key) {
                m_values.erase(i);
                m_bloomFilter.remove(key.string());
                return;
            }
        }
    }
}

void MutablePropertyValueList::setProperty(AtomicString key, String* value)
{
    size_t idx = findProperty(key);
    if (idx == std::numeric_limits<size_t>::max()) {
        m_values.push_back(MutablePropertyValue(key, value));
        m_bloomFilter.add(key.string());
    } else {
        m_values[idx].setValue(value);
    }
}

void MutablePropertyValueList::addPropertyIfNotExists(AtomicString key,
                                                      String* value)
{
    size_t idx = findProperty(key);
    if (idx == std::numeric_limits<size_t>::max()) {
        m_values.push_back(MutablePropertyValue(key, value));
        m_bloomFilter.add(key.string());
    }
}

} // namespace Starfish
