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

#ifndef __StarFishCSSCounterFunction__
#define __StarFishCSSCounterFunction__

namespace StarFish {

class CSSCounterFunction : public gc {
public:
    CSSCounterFunction(const AtomicString& name)
        : m_name(name)
        , m_separator()
        , m_style()
    {
    }

    const AtomicString& name() const
    {
        return m_name;
    }

    Nullable<String*> separator()
    {
        return m_separator;
    }

    void setSeparator(Nullable<String*> sp)
    {
        m_separator = sp;
    }

    Nullable<AtomicString> style()
    {
        return m_style;
    }

    void setStyle(const AtomicString& style)
    {
        m_style = style;
    }

    String* toString()
    {
        StringBuilder result;
        if (m_separator.hasValue()) {
            result.appendString("counters(");
            result.appendString(m_name.string());
            result.appendString(", \"");
            result.appendString(m_separator.getValue());
            result.appendString("\"");
        } else {
            result.appendString("counter(");
            result.appendString(m_name.string());
        }
        if (m_style.hasValue()) {
            result.appendString(", ");
            result.appendString(m_style.getValue().string());
        }
        result.appendChar(')');
        return result.finalize();
    }

protected:
    AtomicString m_name;
    Nullable<String*> m_separator;
    Nullable<AtomicString> m_style;
};
}

#endif
