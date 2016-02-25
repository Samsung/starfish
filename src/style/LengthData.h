#ifndef __StarFishLengthData__
#define __StarFishLengthData__

namespace StarFish {

class LengthData {
public:
    LengthData()
        : m_top(Length(Length::Fixed, 0))
        , m_right(Length(Length::Fixed, 0))
        , m_bottom(Length(Length::Fixed, 0))
        , m_left(Length(Length::Fixed, 0))
    {
    }

    LengthData(Length length)
        : m_top(length)
        , m_right(length)
        , m_bottom(length)
        , m_left(length)
    {
    }

    Length top()
    {
        return m_top;
    }

    Length right()
    {
        return m_right;
    }

    Length bottom()
    {
        return m_bottom;
    }

    Length left()
    {
        return m_left;
    }

    void setTop(Length top)
    {
        m_top = top;
    }

    void setRight(Length right)
    {
        m_right = right;
    }

    void setBottom(Length bottom)
    {
        m_bottom = bottom;
    }

    void setLeft(Length left)
    {
        m_left = left;
    }

    void checkComputed(Length fontSize, Font* font)
    {
        m_left.changeToFixedIfNeeded(fontSize, font);
        m_right.changeToFixedIfNeeded(fontSize, font);
        m_top.changeToFixedIfNeeded(fontSize, font);
        m_bottom.changeToFixedIfNeeded(fontSize, font);
    }

    bool operator==(const LengthData& o)
    {
        return m_left == o.m_left && m_right == o.m_right && m_top == o.m_top && m_bottom == o.m_bottom;
    }

    bool operator!=(const LengthData& o)
    {
        return !operator ==(o);
    }

private:
    Length m_top;
    Length m_right;
    Length m_bottom;
    Length m_left;
};

} /* namespace StarFish */

#endif /* __StarFishLengthData__ */
