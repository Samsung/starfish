/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishScrollOptions__
#define __StarfishScrollOptions__

namespace Starfish {

struct ScrollOptions {
public:
    enum class ScrollBehavior { Auto, Instant, Smooth };

    STARFISH_MAKE_STACK_ALLOCATED()
    ScrollOptions(ScrollBehavior behavior = ScrollBehavior::Auto)
        : m_behavior(behavior)
    {
    }

    void setBehavior(ScrollBehavior scrollBehavior)
    {
        m_behavior = scrollBehavior;
    }

    void setBehavior(String* scrollBehaviorStr)
    {
        m_behavior = stringToBehavior(scrollBehaviorStr);
    }

    ScrollBehavior behaviorValue() const
    {
        return m_behavior;
    }

    String* behavior() const
    {
        return ScrollOptions::behaviorToString(m_behavior);
    }

    static ScrollBehavior stringToBehavior(String* scrollBehaviorStr)
    {
        if (scrollBehaviorStr->equals("auto")) {
            return ScrollOptions::ScrollBehavior::Auto;
        } else if (scrollBehaviorStr->equals("instant")) {
            return ScrollOptions::ScrollBehavior::Instant;
        } else if (scrollBehaviorStr->equals("smooth")) {
            return ScrollOptions::ScrollBehavior::Smooth;
        }
        return ScrollOptions::ScrollBehavior::Auto;
    }

    static String* behaviorToString(ScrollBehavior scrollBehavior)
    {
        switch (scrollBehavior) {
        case ScrollBehavior::Auto:
            return String::fromUTF8("auto");
        case ScrollBehavior::Instant:
            return String::fromUTF8("instant");
        case ScrollBehavior::Smooth:
            return String::fromUTF8("smooth");
        default:
            return String::emptyString;
        }
        return String::emptyString;
    }

private:
    ScrollBehavior m_behavior;
};

struct ScrollToOptions : public ScrollOptions {
public:
    STARFISH_MAKE_STACK_ALLOCATED()
    ScrollToOptions()
        : ScrollOptions()
        , m_left(0)
        , m_top(0)
        , m_hasLeft(false)
        , m_hasTop(false)
    {
    }

    ScrollToOptions(double left, double top)
        : ScrollOptions()
        , m_left(left)
        , m_top(top)
        , m_hasLeft(true)
        , m_hasTop(true)
    {
        if (std::isnan(left)) {
            m_left = 0;
        }
        if (std::isnan(top)) {
            m_top = 0;
        }
    }

    void setLeft(double left)
    {
        if (std::isnan(left)) {
            m_left = 0;
        } else {
            m_left = left;
        }
        m_hasLeft = true;
    }

    double left() const
    {
        return m_left;
    }

    void setTop(double top)
    {
        if (std::isnan(top)) {
            m_top = 0;
        } else {
            m_top = top;
        }
        m_hasTop = true;
    }

    double top() const
    {
        return m_top;
    }

    bool hasLeft() const
    {
        return m_hasLeft;
    }

    bool hasTop() const
    {
        return m_hasTop;
    }

private:
    double m_left;
    double m_top;
    bool m_hasLeft;
    bool m_hasTop;
};

enum class ScrollLogicalPosition { Start, Center, End, Nearest };

struct ScrollIntoViewOptions : ScrollOptions {
public:
    STARFISH_MAKE_STACK_ALLOCATED()
    ScrollIntoViewOptions(
        ScrollBehavior behavior = ScrollBehavior::Auto,
        ScrollLogicalPosition block = ScrollLogicalPosition::Center,
        ScrollLogicalPosition inLine = ScrollLogicalPosition::Center)
        : ScrollOptions(behavior)
        , m_block(block)
        , m_inline(inLine)
    {
    }

    void setBlock(ScrollLogicalPosition block)
    {
        m_block = block;
    }
    void setBlock(String* blockStr)
    {
        m_block = stringToPosition(blockStr);
    }
    ScrollLogicalPosition blockValue() const
    {
        return m_block;
    }
    String* block() const
    {
        return ScrollIntoViewOptions::positionToString(m_block);
    }

    void setInLine(ScrollLogicalPosition inLine)
    {
        m_inline = inLine;
    }
    void setInLine(String* inlineStr)
    {
        m_inline = stringToPosition(inlineStr);
    }
    ScrollLogicalPosition inlineValue() const
    {
        return m_inline;
    }
    String* inLine() const
    {
        return ScrollIntoViewOptions::positionToString(m_inline);
    }

    static ScrollLogicalPosition stringToPosition(String* positionStr)
    {
        if (positionStr->equals("start")) {
            return ScrollLogicalPosition::Start;
        } else if (positionStr->equals("center")) {
            return ScrollLogicalPosition::Center;
        } else if (positionStr->equals("end")) {
            return ScrollLogicalPosition::End;
        } else if (positionStr->equals("nearest")) {
            return ScrollLogicalPosition::Nearest;
        }
        return ScrollLogicalPosition::Center;
    }

    static String* positionToString(ScrollLogicalPosition position)
    {
        switch (position) {
        case ScrollLogicalPosition::Start:
            return String::fromUTF8("start");
        case ScrollLogicalPosition::Center:
            return String::fromUTF8("center");
        case ScrollLogicalPosition::End:
            return String::fromUTF8("end");
        case ScrollLogicalPosition::Nearest:
            return String::fromUTF8("nearest");
        default:
            return String::emptyString;
        }
        return String::emptyString;
    }

private:
    ScrollLogicalPosition m_block;
    ScrollLogicalPosition m_inline;
};
} // namespace Starfish

#endif
