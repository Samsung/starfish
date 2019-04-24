
/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishDOMExceptionOr__
#define __StarfishDOMExceptionOr__

namespace Starfish {

class DOMException;

template <typename T>
class DOMExceptionOr : public gc {
public:
    union Data {
        DOMException* m_exception;
        T m_other;

        Data(DOMException* exception)
            : m_exception(exception)
        {
        }

        Data(T value)
            : m_other(value)
        {
        }
    };

    DOMExceptionOr(T value)
        : m_data(value)
        , m_isDOMException(false)
    {
    }

    DOMExceptionOr(DOMException* exception)
        : m_data(exception)
        , m_isDOMException(true)
    {
        STARFISH_ASSERT(exception != nullptr);
    }

    DOMExceptionOr(nullptr_t) = delete;

    bool isDOMException() const
    {
        return m_isDOMException;
    }

    T asOtherType() const
    {
        STARFISH_ASSERT(!isDOMException());
        return m_data.m_other;
    }

    DOMException* asDOMException() const
    {
        STARFISH_ASSERT(isDOMException());
        return m_data.m_exception;
    }

private:
    DOMExceptionOr() = delete;

    Data m_data;
    bool m_isDOMException;
};
}
#endif
