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

#include "StarfishConfig.h"
#include "GridAreaData.h"

namespace Starfish {

NamedGridAreaDataMap::NamedGridAreaDataMap()
    : m_minRow(SIZE_MAX)
    , m_maxRow(0)
    , m_minColumn(SIZE_MAX)
    , m_maxColumn(0)
{
}

GCUnorderedMap<String*, GCVector<GridAreaData>>& NamedGridAreaDataMap::map()
{
    return m_namedGridAreaDataMap;
}

void NamedGridAreaDataMap::insertNamedGridAreaData(
    String* name, const GridAreaData& gridAreaData)
{
    if (gridAreaData.rowStart < m_minRow) {
        m_minRow = gridAreaData.rowStart;
    }
    if (gridAreaData.columnStart < m_minColumn) {
        m_minColumn = gridAreaData.columnStart;
    }
    if (m_maxRow < gridAreaData.rowEnd) {
        m_maxRow = gridAreaData.rowEnd;
    }
    if (m_maxColumn < gridAreaData.columnEnd) {
        m_maxColumn = gridAreaData.columnEnd;
    }

    auto iter = m_namedGridAreaDataMap.find(name);
    if (iter != m_namedGridAreaDataMap.end()) {
        iter.value().push_back(gridAreaData);
    } else {
        GCVector<GridAreaData> vector;
        vector.push_back(gridAreaData);
        m_namedGridAreaDataMap.insert(std::make_pair(name, std::move(vector)));
    }
}

String* NamedGridAreaDataMap::toString()
{
    // Generate a string representation of grid-template-areas. The format is:
    // '"head head" "nav main" ". foot"'
    if (m_namedGridAreaDataMap.size()) {
        GCVector<String*> sv;
        for (size_t r = 1; r < m_maxRow; r++) {
            StringBuilder builder;
            for (size_t c = 1; c < m_maxColumn; c++) {
                bool found = false;
                for (auto& pair : m_namedGridAreaDataMap) {
                    String* areaName = pair.first;
                    for (auto& gridAreaData : pair.second) {
                        if ((r >= gridAreaData.rowStart &&
                             c >= gridAreaData.columnStart) &&
                            (r < gridAreaData.rowEnd &&
                             c < gridAreaData.columnEnd)) {
                            if (builder.length()) {
                                builder.appendChar(' ');
                            }
                            builder.appendString(areaName);
                        }
                    }
                }
            }
            sv.push_back(builder.finalize());
        }
        StringBuilder builder;
        for (auto s : sv) {
            if (builder.length()) {
                builder.appendChar(' ');
            }
            builder.appendChar('"');
            builder.appendString(s);
            builder.appendChar('"');
        }
        return builder.finalize();
    }

    return String::emptyString;
}

bool NamedGridAreaDataMap::compare(const NamedGridAreaDataMap* other) const
{
    if (!other) {
        return false;
    }

    if (m_minRow != other->m_minRow || m_maxRow != other->m_maxRow ||
        m_minColumn != other->m_minColumn ||
        m_maxColumn != other->m_maxColumn) {
        return false;
    }

    if (m_namedGridAreaDataMap.size() != other->m_namedGridAreaDataMap.size()) {
        return false;
    }

    for (auto& pair : m_namedGridAreaDataMap) {
        String* name = pair.first;
        const GCVector<GridAreaData>& value = pair.second;
        auto iter = other->m_namedGridAreaDataMap.find(name);
        if (iter == other->m_namedGridAreaDataMap.end()) {
            return false;
        }

        const GCVector<GridAreaData>& otherValue = iter.value();
        if (value.size() != otherValue.size()) {
            return false;
        }

        for (size_t i = 0; i < value.size(); i++) {
            if (value[i] != otherValue[i]) {
                return false;
            }
        }
    }
    return true;
}
} // namespace Starfish
