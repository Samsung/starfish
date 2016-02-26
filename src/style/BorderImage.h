#ifndef __StarBorderImage__
#define __StarBorderImage__

#include "style/BorderImageLength.h"

namespace StarFish {

#define DEFAULT_VALUE_IMAGE_WIDTH() \
    (BorderImageLengthBox(1.0))
#define DEFAULT_VALUE_IMAGE_SLICE() \
    (LengthBox(Length(Length::Percent, 1.f), Length(Length::Percent, 1.f), Length(Length::Percent, 1.f), Length(Length::Percent, 1.f)))
#define DEFAULT_VALUE_IMAGE_REPEAT() \
    (StretchValue)

class BorderImageImpl : public gc {
public:
    BorderImageImpl()
        : m_repeatX(DEFAULT_VALUE_IMAGE_REPEAT())
        , m_repeatY(DEFAULT_VALUE_IMAGE_REPEAT())
        , m_url(nullptr)
        , m_sliceFill(false)
        , m_slices(DEFAULT_VALUE_IMAGE_SLICE())
        , m_widths(DEFAULT_VALUE_IMAGE_WIDTH())
    {
    }

    void checkComputed(Length fontSize, Font* font)
    {
        m_slices.checkComputed(fontSize, font);
    }

    bool operator==(const BorderImageImpl& o)
    {
        return m_repeatX == o.m_repeatX && m_repeatY == o.m_repeatY
            && m_url->equals(o.m_url)
            && m_sliceFill == o.m_sliceFill
            && m_slices == o.m_slices
            && m_widths == o.m_widths;
    }

    bool operator!=(const BorderImageImpl& o)
    {
        return !operator==(o);
    }

public:
    BorderImageRepeatValue m_repeatX; // [border-image-repeat]
    BorderImageRepeatValue m_repeatY; // [border-image-repeat]

    // TODO: Need Image Data Structure
    String* m_url; // [border-image-source]
    bool m_sliceFill; // [border-image-slice]
    LengthBox m_slices; // [border-image-slice]
    BorderImageLengthBox m_widths; // [border-image-width]
};

class BorderImage {
public:
    BorderImage()
        : m_data(nullptr)
    {
    }

    String* url() { return isNull() ? String::emptyString : m_data->m_url; }
    LengthBox slices() { return isNull() ? DEFAULT_VALUE_IMAGE_SLICE() : m_data->m_slices; }
    bool sliceFill() { return isNull() ? false : m_data->m_sliceFill; }
    BorderImageRepeatValue repeatX() { return isNull() ? DEFAULT_VALUE_IMAGE_REPEAT() : m_data->m_repeatX; }
    BorderImageRepeatValue repeatY() { return isNull() ? DEFAULT_VALUE_IMAGE_REPEAT() : m_data->m_repeatY; }
    BorderImageLengthBox widths() { return isNull() ? DEFAULT_VALUE_IMAGE_WIDTH() : m_data->m_widths; }

    void setUrl(String* url) { data()->m_url = url; }
    void setSlices(const LengthBox& slices) { data()->m_slices = slices; }
    void setSliceFill(bool fill) { data()->m_sliceFill = fill; }
    void setRepeatX(BorderImageRepeatValue value) { data()->m_repeatX = value; }
    void setRepeatY(BorderImageRepeatValue value) { data()->m_repeatY = value; }
    void setWidths(BorderImageLengthBox value) { data()->m_widths = value; }

    void checkComputed(Length fontSize, Font* font)
    {
        if (m_data) {
            m_data->checkComputed(fontSize, font);
        }
    }

    bool operator==(const BorderImage& o)
    {
        if (m_data == NULL && o.m_data == NULL) {

        } else if (m_data == NULL || o.m_data == NULL) {
            return false;
        } else if (*m_data != *o.m_data) {
            return false;
        }
        return true;
    }

    bool operator!=(const BorderImage& o)
    {
        return !operator==(o);
    }

private:
    bool isNull() { return m_data == nullptr; }
    BorderImageImpl* data()
    {
        if (isNull())
            m_data = new BorderImageImpl();
        return m_data;
    }

private:
    BorderImageImpl* m_data;
};

} /* namespace StarFish */

#endif /* __StarBorderImage__ */
