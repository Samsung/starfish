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

#ifndef __StarfishUnresolvedColor__
#define __StarfishUnresolvedColor__

#include "core/style/Unit.h"
#include "core/style/NamedColors.h"

namespace Starfish {

// A <color> whose value depends on the element's computed style:
// `currentcolor` needs the element's color, and light-dark()
// (css-color-5 #light-dark) needs its used color scheme. Colors nested
// inside such a function are kept as leaves of the same tree, so the whole
// specified value can be serialized and resolved once the style is final.
class UnresolvedColor : public gc {
public:
    enum Kind : unsigned char {
        LiteralKind,
        NamedKind,
        CurrentColorKind,
        LightDarkKind,
    };

    static UnresolvedColor* createLiteral(const Unit::Color& color)
    {
        UnresolvedColor* c = new UnresolvedColor(LiteralKind);
        c->m_color = color;
        return c;
    }

    static UnresolvedColor* createNamed(NamedColor::NamedColorValue named)
    {
        UnresolvedColor* c = new UnresolvedColor(NamedKind);
        c->m_named = named;
        return c;
    }

    static UnresolvedColor* createCurrentColor()
    {
        return new UnresolvedColor(CurrentColorKind);
    }

    static UnresolvedColor* createLightDark(UnresolvedColor* light,
                                            UnresolvedColor* dark)
    {
        UnresolvedColor* c = new UnresolvedColor(LightDarkKind);
        c->m_children.m_first = light;
        c->m_children.m_second = dark;
        return c;
    }

    Unit::Color resolve(const Unit::Color& currentColor, bool dark) const
    {
        switch (m_kind) {
        case LiteralKind:
            return m_color;
        case NamedKind:
            return NamedColor::namedColorToColor(m_named);
        case CurrentColorKind:
            return currentColor;
        case LightDarkKind:
            return (dark ? m_children.m_second : m_children.m_first)
                ->resolve(currentColor, dark);
        }
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return Unit::Color();
    }

    bool equals(const UnresolvedColor* other) const
    {
        if (m_kind != other->m_kind) {
            return false;
        }
        switch (m_kind) {
        case LiteralKind:
            return m_color == other->m_color;
        case NamedKind:
            return m_named == other->m_named;
        case CurrentColorKind:
            return true;
        case LightDarkKind:
            return m_children.m_first->equals(other->m_children.m_first) &&
                   m_children.m_second->equals(other->m_children.m_second);
        }
        return false;
    }

    // Specified-value serialization.
    String* toString() const
    {
        switch (m_kind) {
        case LiteralKind:
            return m_color.toString();
        case NamedKind:
            return NamedColor::namedColorToString(m_named);
        case CurrentColorKind:
            return String::fromUTF8("currentcolor");
        case LightDarkKind: {
            StringBuilder builder;
            builder.appendString("light-dark(");
            builder.appendString(m_children.m_first->toString());
            builder.appendString(", ");
            builder.appendString(m_children.m_second->toString());
            builder.appendChar(')');
            return builder.finalize();
        }
        }
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return String::emptyString;
    }

private:
    explicit UnresolvedColor(Kind kind)
        : m_kind(kind)
        , m_color()
    {
    }

    Kind m_kind;
    // One payload per kind; the object is GC-scanned conservatively, so the
    // child pointers are found wherever the union places them.
    union {
        Unit::Color m_color;
        NamedColor::NamedColorValue m_named;
        struct {
            UnresolvedColor* m_first;
            UnresolvedColor* m_second;
        } m_children;
    };
};

} // namespace Starfish

#endif
