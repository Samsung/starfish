/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishPaintPassMemo__
#define __StarfishPaintPassMemo__

#include "core/layout/LayoutUtil.h"

namespace Starfish {

class FrameBox;
class StackingContext;

// Side table for a memo that is valid for exactly one paint pass, keyed by the
// object it belongs to. Holding these off to the side rather than in a field
// per object matters because the keyed objects are the numerous ones: a page
// with a large box tree pays sizeof(memo) on every box for values that only a
// painted, clip-tested subset ever asks for.
//
// Open addressing with linear probing over a power-of-two slot array. Every
// slot records the generation it was written in, so dropping the whole table
// between passes is a generation bump instead of a walk: a slot left from an
// older generation reads as empty and is overwritten in place.
//
// Keys are GC pointers, and the slot array is a GCVector, so the array is
// GC-allocated and its keys are traced. Entries do not keep an object alive
// beyond the pass in any way that matters, since the table is dropped at the
// end of every pass.
template <typename Key, typename Value>
class PaintPassTable {
public:
    // The memoized value for key, or nullptr when this pass has not computed
    // it yet. Only valid until the next put() - that may grow the array and
    // move the slots.
    Value* find(Key key)
    {
        if (!m_slots.size()) {
            return nullptr;
        }
        size_t mask = m_slots.size() - 1;
        size_t i = hashOf(key) & mask;
        while (true) {
            Slot& slot = m_slots[i];
            if (slot.m_generation != m_generation) {
                // Empty for this pass; an occupied key would have been placed
                // here, so the probe ends.
                return nullptr;
            }
            if (slot.m_key == key) {
                return &slot.m_value;
            }
            i = (i + 1) & mask;
        }
    }

    void put(Key key, const Value& value)
    {
        if ((m_liveCount + 1) * 4 >= m_slots.size() * 3) {
            grow();
        }
        size_t mask = m_slots.size() - 1;
        size_t i = hashOf(key) & mask;
        while (true) {
            Slot& slot = m_slots[i];
            if (slot.m_generation != m_generation) {
                slot.m_key = key;
                slot.m_value = value;
                slot.m_generation = m_generation;
                m_liveCount++;
                return;
            }
            if (slot.m_key == key) {
                slot.m_value = value;
                return;
            }
            i = (i + 1) & mask;
        }
    }

    // Drops every entry. O(1): the slots stay allocated for the next pass and
    // are recognised as empty by their stale generation.
    void clear()
    {
        m_generation++;
        m_liveCount = 0;
        if (UNLIKELY(m_generation == 0)) {
            // Wrapped: stale slots would read as live again.
            m_generation = 1;
            for (size_t i = 0; i < m_slots.size(); i++) {
                m_slots[i].m_generation = 0;
            }
        }
    }

private:
    struct Slot {
        Key m_key{ nullptr };
        Value m_value;
        uint32_t m_generation{ 0 };
    };

    // Vector::resize() default-initializes elements only for a non-trivial
    // type; for a trivial one it leaves the memory as it found it. A slot
    // whose generation came up as garbage would read as a live entry, so the
    // initializer above is load-bearing.
    static_assert(!std::is_trivial<Slot>::value,
                  "PaintPassTable relies on Slot being default-initialized by "
                  "Vector::resize()");

    static size_t hashOf(Key key)
    {
        // Keys are heap pointers, so the low bits are alignment padding.
        uintptr_t h = reinterpret_cast<uintptr_t>(key) >> 4;
        return static_cast<size_t>(h * 2654435761u);
    }

    void grow()
    {
        size_t newSize = m_slots.size() ? m_slots.size() * 2 : 64;
        GCVector<Slot> old;
        old.resize(m_slots.size());
        for (size_t i = 0; i < m_slots.size(); i++) {
            old[i] = m_slots[i];
        }
        uint32_t generation = m_generation;

        m_slots.clear();
        m_slots.resize(newSize);
        // A fresh array's slots are generation 0, so restart the numbering
        // rather than carrying the old one onto slots that never held it.
        m_generation = 1;
        m_liveCount = 0;

        for (size_t i = 0; i < old.size(); i++) {
            if (old[i].m_generation == generation) {
                put(old[i].m_key, old[i].m_value);
            }
        }
    }

    GCVector<Slot> m_slots;
    size_t m_liveCount{ 0 };
    uint32_t m_generation{ 1 };
};

// StackingContext's paint-pass memos. They are gathered into one entry so a
// context costs a single slot and a single probe, rather than one table each.
struct StackingContextPassMemo {
    enum Field : uint8_t {
        CullRectInParentSpace = 1 << 0,
        FastBufferedVisit = 1 << 1,
        FastCaptureDescendants = 1 << 2,
        SubtreeContainsGraphicsBufferLayer = 1 << 3,
    };

    bool isComputed(Field f) const
    {
        return m_computed & f;
    }

    bool flag(Field f) const
    {
        return m_flags & f;
    }

    void setFlag(Field f, bool value)
    {
        m_computed |= f;
        if (value) {
            m_flags |= f;
        } else {
            m_flags &= ~f;
        }
    }

    LayoutRect m_cullRectInParentSpace;
    uint8_t m_computed{ 0 }; // which fields hold a value for this pass
    uint8_t m_flags{ 0 };    // the boolean fields' values
};

// The memo tables for one paint pass, owned by the WebView driving the pass
// and threaded through the paint code by the painting context objects. A null
// pointer means "not inside a pass": the memoized functions still return the
// right answer, they just recompute.
class PaintPassMemos : public gc {
public:
    void beginPass()
    {
        m_paintExtent.clear();
        m_stackingContext.clear();
    }

    PaintPassTable<FrameBox*, LayoutRect> m_paintExtent;
    PaintPassTable<StackingContext*, StackingContextPassMemo> m_stackingContext;
};

} // namespace Starfish

#endif
