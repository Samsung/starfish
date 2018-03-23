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

#ifndef __StarFishListCounterBaseList__
#define __StarFishListCounterBaseList__

namespace StarFish {

typedef GCAtomicVector<std::pair<AtomicString, int32_t>> CounterBaseList;

inline bool operator==(const CounterBaseList& a, const CounterBaseList& b)
{
    if (&a == &b) {
        return true;
    }
    size_t size = a.size();
    if (size != b.size()) {
        return false;
    }
    for (size_t i = 0; i < size; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

inline bool operator!=(const CounterBaseList& a, const CounterBaseList& b)
{
    return !operator==(a, b);
}
}

#endif
