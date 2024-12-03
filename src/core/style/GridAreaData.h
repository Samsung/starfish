/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishGridAreaData__
#define __StarfishGridAreaData__

#include <cstddef>

namespace Starfish {

struct GridAreaData {
    size_t columnStart;
    size_t columnEnd;
    size_t rowStart;
    size_t rowEnd;

    inline bool operator==(const GridAreaData& other) const
    {
        return columnStart == other.columnStart &&
               columnEnd == other.columnEnd && rowStart == other.rowStart &&
               rowEnd == other.rowEnd;
    }
    inline bool operator!=(const GridAreaData& other) const
    {
        return !(*this == other);
    }
};

class NamedGridAreaDataMap : public gc {
public:
    NamedGridAreaDataMap();

    GCUnorderedMap<String*, GCVector<GridAreaData>>& map();

    void insertNamedGridAreaData(String* name,
                                 const GridAreaData& gridAreaData);

    String* toString();

    bool compare(const NamedGridAreaDataMap* other) const;

    void reset();

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(NamedGridAreaDataMap)] = { 0 };

            markHashTable(desc, GC_WORD_OFFSET(NamedGridAreaDataMap,
                                               m_namedGridAreaDataMap));
            descr = GC_make_descriptor(desc, GC_WORD_LEN(NamedGridAreaDataMap));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }

    void* operator new[](size_t size) = delete;

private:
    size_t m_minRow;
    size_t m_maxRow;
    size_t m_minColumn;
    size_t m_maxColumn;
    GCUnorderedMap<String*, GCVector<GridAreaData>> m_namedGridAreaDataMap;
};

} // namespace Starfish

#endif
