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

#ifndef __StarFishLayoutRepaintTracker__
#define __StarFishLayoutRepaintTracker__

namespace Starfish {

class Node;
class FrameBox;
class FrameBlockBox;
class FrameDocument;

class LayoutRepaintTracker {
public:
    struct InlineLayoutResultItem : public gc {
        LayoutRect m_frameRect;
        size_t m_textStartEndValue;

        bool operator==(const InlineLayoutResultItem& src)
        {
            return src.m_frameRect == m_frameRect &&
                   src.m_textStartEndValue == m_textStartEndValue;
        }

        bool operator!=(const InlineLayoutResultItem& src)
        {
            return !operator==(src);
        }
    };

    typedef GCAtomicVector<InlineLayoutResultItem> InlineLayoutResult;

    bool traceRepaintRegion(FrameDocument* fd);
    void dispose()
    {
        m_rootedNodeSet.clear();
        m_lastInlineTextLayoutResult.clear();
        std::unordered_map<Node*, std::pair<LayoutRect, Node*>>().swap(
            m_lastLayoutResult);
        std::unordered_map<Node*, LayoutRect>().swap(
            m_dirtyAreaPerStackingContextOwners);
    }

    void clearDatasRelatedWithStackingContext()
    {
        m_dirtyAreaPerStackingContextOwners.clear();
    }

    const std::unordered_map<Node*, LayoutRect>&
    dirtyAreaPerStackingContextOwners()
    {
        return m_dirtyAreaPerStackingContextOwners;
    }

protected:
    std::unordered_set<Node*, std::hash<Node*>, std::equal_to<Node*>,
                       GCUtil::gc_malloc_allocator<Node*>>
        m_rootedNodeSet;

    // layout results
    std::unordered_map<Node*, std::pair<LayoutRect, Node*>> m_lastLayoutResult;
    GCVector<std::tuple<FrameBlockBox*, FrameBox*, InlineLayoutResult*>>
        m_lastInlineTextLayoutResult;

    // computed dirty areas
    std::unordered_map<Node*, LayoutRect> m_dirtyAreaPerStackingContextOwners;
};
}

#endif
