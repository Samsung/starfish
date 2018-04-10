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

#ifndef __StarFishListWillChangeData__
#define __StarFishListWillChangeData__

namespace StarFish {

class WillChangeData : public GCVector<AtomicString> {
public:
    WillChangeData()
        : m_contents(false)
        , m_scrollPosition(false)
    {
    }

    bool contents() const
    {
        return m_contents;
    }

    void setContents()
    {
        m_contents = true;
    }

    void setScrollPosition()
    {
        m_contents = true;
    }

    bool scrollPosition() const
    {
        return m_scrollPosition;
    }

protected:
    bool m_contents;
    bool m_scrollPosition;
};
}
#endif
