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
#include "core/style/ColorInterpolation.h"

namespace Starfish {

// A <color> whose value depends on the element's computed style:
// `currentcolor` needs the element's color, and light-dark()
// (css-color-5 #light-dark) needs its used color scheme. Colors nested
// inside such a function are kept as leaves of the same tree, so the whole
// specified value can be serialized and resolved once the style is final.
// color-mix() (css-color-5 #color-mix) is a node of the same tree so that
// its arguments may be any of the above. Nodes are immutable.
class UnresolvedColor : public gc {
public:
    enum Kind : unsigned char {
        LiteralKind,
        NamedKind,
        CurrentColorKind,
        LightDarkKind,
        ColorMixKind,
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

    // `currentcolor` carries no data, so one instance serves every use.
    static UnresolvedColor* currentColor()
    {
        static UnresolvedColor* s_currentColor =
            new UnresolvedColor(CurrentColorKind);
        return s_currentColor;
    }

    static UnresolvedColor* createLightDark(UnresolvedColor* light,
                                            UnresolvedColor* dark)
    {
        UnresolvedColor* c = new UnresolvedColor(LightDarkKind);
        c->m_children.m_first = light;
        c->m_children.m_second = dark;
        return c;
    }

    // Percentages are as specified; a negative value stands for "omitted".
    static UnresolvedColor* createColorMix(ColorInterpolation::Space space,
                                           ColorInterpolation::HueMethod hue,
                                           UnresolvedColor* first,
                                           double firstPercent,
                                           UnresolvedColor* second,
                                           double secondPercent)
    {
        UnresolvedColor* c = new UnresolvedColor(ColorMixKind);
        c->m_space = space;
        c->m_hueMethod = hue;
        c->m_children.m_first = first;
        c->m_children.m_second = second;
        c->m_children.m_firstPercent = firstPercent;
        c->m_children.m_secondPercent = secondPercent;
        return c;
    }

    // False when the value depends on the element (currentcolor, or the
    // used color scheme), i.e. it cannot be resolved at parse time.
    bool isConstant() const
    {
        switch (m_kind) {
        case LiteralKind:
        case NamedKind:
            return true;
        case CurrentColorKind:
        case LightDarkKind:
            return false;
        case ColorMixKind:
            return m_children.m_first->isConstant() &&
                   m_children.m_second->isConstant();
        }
        return false;
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
        case ColorMixKind: {
            double p1, p2;
            mixPercentages(&p1, &p2);
            double sum = p1 + p2;
            return ColorInterpolation::mix(
                (ColorInterpolation::Space)m_space,
                (ColorInterpolation::HueMethod)m_hueMethod,
                m_children.m_first->resolve(currentColor, dark), p1 / sum,
                m_children.m_second->resolve(currentColor, dark), p2 / sum,
                sum < 100 ? sum / 100 : 1);
        }
        }
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return Unit::Color();
    }

    // The tree with every light-dark() decided for `dark`: that choice is
    // part of the computed value, whereas `currentcolor` stays unresolved
    // when the value is inherited (css-color-4 #currentcolor-color).
    UnresolvedColor* foldLightDark(bool dark)
    {
        switch (m_kind) {
        case LiteralKind:
        case NamedKind:
        case CurrentColorKind:
            return this;
        case LightDarkKind:
            return (dark ? m_children.m_second : m_children.m_first)
                ->foldLightDark(dark);
        case ColorMixKind: {
            UnresolvedColor* first = m_children.m_first->foldLightDark(dark);
            UnresolvedColor* second = m_children.m_second->foldLightDark(dark);
            if (first == m_children.m_first && second == m_children.m_second) {
                return this;
            }
            return createColorMix((ColorInterpolation::Space)m_space,
                                  (ColorInterpolation::HueMethod)m_hueMethod,
                                  first, m_children.m_firstPercent, second,
                                  m_children.m_secondPercent);
        }
        }
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return this;
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
        case ColorMixKind:
            return m_space == other->m_space &&
                   m_hueMethod == other->m_hueMethod &&
                   m_children.m_firstPercent ==
                       other->m_children.m_firstPercent &&
                   m_children.m_secondPercent ==
                       other->m_children.m_secondPercent &&
                   m_children.m_first->equals(other->m_children.m_first) &&
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
        case ColorMixKind: {
            // css-color-5 #serial-color-mix: the hue method is omitted when
            // it is the default, and the percentages when they are 50%/50%.
            StringBuilder builder;
            builder.appendString("color-mix(in ");
            const char* space = ColorInterpolation::spaceName(
                (ColorInterpolation::Space)m_space);
            builder.appendString(space, strlen(space));
            if (m_hueMethod != ColorInterpolation::Shorter) {
                builder.appendChar(' ');
                const char* method = ColorInterpolation::hueMethodName(
                    (ColorInterpolation::HueMethod)m_hueMethod);
                builder.appendString(method, strlen(method));
                builder.appendString(" hue");
            }
            double p1, p2;
            mixPercentages(&p1, &p2);
            bool showPercent = !(p1 == 50 && p2 == 50);
            builder.appendString(", ");
            builder.appendString(m_children.m_first->toString());
            if (showPercent) {
                appendPercent(builder, p1);
            }
            builder.appendString(", ");
            builder.appendString(m_children.m_second->toString());
            if (showPercent) {
                appendPercent(builder, p2);
            }
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
        , m_space(0)
        , m_hueMethod(0)
        , m_color()
    {
    }

    // css-color-5 #color-mix-percent-norm: an omitted percentage is the
    // complement of the other, or 50% when both are omitted.
    void mixPercentages(double* p1, double* p2) const
    {
        double a = m_children.m_firstPercent;
        double b = m_children.m_secondPercent;
        if (a < 0 && b < 0) {
            a = b = 50;
        } else if (a < 0) {
            a = 100 - b;
        } else if (b < 0) {
            b = 100 - a;
        }
        *p1 = a;
        *p2 = b;
    }

    static void appendPercent(StringBuilder& builder, double percent)
    {
        // StringBuilder keeps a reference to each piece, so the text has to
        // outlive the local buffer
        char buf[32];
        snprintf(buf, sizeof(buf), " %.6g%%", percent);
        builder.appendString(String::createASCIIString(buf, strlen(buf)));
    }

    Kind m_kind;
    // color-mix() only; kept beside the kind so the union stays two words
    unsigned char m_space;
    unsigned char m_hueMethod;
    // One payload per kind; the object is GC-scanned conservatively, so the
    // child pointers are found wherever the union places them.
    union {
        Unit::Color m_color;
        NamedColor::NamedColorValue m_named;
        struct {
            UnresolvedColor* m_first;
            UnresolvedColor* m_second;
            // color-mix() only
            float m_firstPercent;
            float m_secondPercent;
        } m_children;
    };
};

} // namespace Starfish

#endif
