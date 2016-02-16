#ifndef __StarFishLength__
#define __StarFishLength__

namespace StarFish {

class Length {
public:
    enum Type {
        Auto,
        Percent,
        Fixed,

        //After finishing resolveStyle, ex/em values should be changed to Fixed
        ExToBeFixed,
        EmToBeFixed
    };

    Length(Type type = Auto, float data = 0.f)
        : m_type(type)
        , m_data(data)
    {

    }

    void changeToFixedIfNeeded(Length fontSize, Font* font) {
        if(!isComputed()) {
            if (m_type == EmToBeFixed) {
            	float fSize = 0.0f;
            	if(fontSize.isFixed())
            		fSize = fontSize.fixed();

            	m_data = fSize * m_data;
            	m_type = Fixed;
            } else if (m_type == ExToBeFixed) {
                // TODO: calculate ex (x-height of font)
            }
        }
    }

    void roundBorderWidth() {
        // NOTE: Border Widths are rounded to the nearest integer number of pixels,
        // but values between zero and one pixels are always rounded up to one device pixel.
        if (m_data > 0.0 && m_data < 1.0)
            m_data = 1.0;
        else {
            if (m_data < 0)
                m_data -= 0.01;
            else
                m_data += 0.01;
            m_data = ((m_data > std::numeric_limits<unsigned>::max()) || (m_data < std::numeric_limits<unsigned>::min())) ? 0 : static_cast<unsigned>(m_data);
        }
    }

    bool isSpecified() const { return isFixed() || isPercent(); }
    bool isAuto() const { return m_type == Auto; }
    bool isFixed() const { return m_type == Fixed; }
    bool isPercent() const { return m_type == Percent; }
    bool isComputed() const { return isFixed() || isPercent() || isAuto(); }
    Type type() const { return m_type; }

    float percent() const
    {
        STARFISH_ASSERT(m_type == Percent);
        //0~1
        return m_data;
    }

    float fixed() const
    {
        STARFISH_ASSERT(m_type == Fixed);
        return m_data;
    }

    void percentToFixed(float refer)
    {
        STARFISH_ASSERT(m_type == Percent);
        m_data = refer * m_data;
        m_type = Fixed;
    }

    float specifiedValue(float parentLength) const
    {
        STARFISH_ASSERT(isSpecified());
        if (isFixed())
            return fixed();
        else
            return percent() * parentLength;
    }

    bool operator == (const Length& src)
    {
        return this->m_type == src.m_type && this->m_data == src.m_data;
    }

    bool operator != (const Length& src)
    {
        return !operator ==(src);
    }

    String* dumpString()
    {
        char temp[100];
        if (isFixed())
            sprintf(temp, "%f", fixed());
        else if(isPercent())
            sprintf(temp, "%f", percent());
        else if(isAuto())
            sprintf(temp, "auto");
        return String::fromUTF8(temp);
    }

protected:
    Type m_type;
    float m_data;
};

class LengthSize : public gc {
public:
    LengthSize()
    {
    }

    LengthSize(Length width)
        : m_width(width) {
    }

    LengthSize(Length width, Length height)
        : m_width(width),
          m_height(height) {
    }

    Length width() {
        return m_width;
    }

    Length height() {
        return m_height;
    }

    void checkComputed(Length fontSize, Font* font)
    {
        m_width.changeToFixedIfNeeded(fontSize, font);
        m_height.changeToFixedIfNeeded(fontSize, font);
    }

    Length m_width;
    Length m_height;
};

class LengthBox {
public:
    LengthBox()
    {
    }

    LengthBox(float v)
        : m_left(Length(Length::Fixed, v))
        , m_right(Length(Length::Fixed, v))
        , m_top(Length(Length::Fixed, v))
        , m_bottom(Length(Length::Fixed, v))
    {
    }

    LengthBox(const Length& t, const Length& r, const Length& b, const Length& l)
        : m_left(l)
        , m_right(r)
        , m_top(t)
        , m_bottom(b)
    {
    }

    LengthBox(float t, float r, float b, float l)
        : m_left(Length(Length::Fixed, l))
        , m_right(Length(Length::Fixed, r))
        , m_top(Length(Length::Fixed, t))
        , m_bottom(Length(Length::Fixed, b))
    {
    }

    bool operator==(const LengthBox& o)
    {
        return this->m_left == o.m_left && this->m_right == o.m_right && this->m_top == o.m_top && this->m_bottom == o.m_bottom;
    }

    bool operator!=(const LengthBox& o)
    {
        return !operator ==(o);
    }

    void checkComputed(Length fontSize, Font* font)
    {
        m_left.changeToFixedIfNeeded(fontSize, font);
        m_right.changeToFixedIfNeeded(fontSize, font);
        m_top.changeToFixedIfNeeded(fontSize, font);
        m_bottom.changeToFixedIfNeeded(fontSize, font);
    }

    const Length& left() const { return m_left; }
    const Length& right() const { return m_right; }
    const Length& top() const { return m_top; }
    const Length& bottom() const { return m_bottom; }

    Length m_left;
    Length m_right;
    Length m_top;
    Length m_bottom;
};

}

#endif
