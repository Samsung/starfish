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

namespace StarFish {

class Font;
class LinearGradientData;
class RadialGradientData;
class CSSGradientValue;
class FrameBox;
class CSSColorStop;

enum class GradientType;
enum class SideValue ENSURE_ENUM_UNSIGNED;
enum class RadialGradientShape;
enum class RadialGradientSizeKeyword;

class ColorStop : public gc {
public:
    ColorStop()
        : m_color()
        , m_offset()
        , m_specified(false)
    {
    }
    Unit::Color color()
    {
        return m_color;
    }

    void setColor(Unit::Color color)
    {
        m_color = color;
    }

    Length offset()
    {
        return m_offset;
    }

    void setOffset(Length offset)
    {
        m_offset = offset;
    }

    bool specified()
    {
        return m_specified;
    }

    void setSpecified(bool value)
    {
        m_specified = value;
    }

    bool equals(ColorStop* other) const
    {
        if ((m_color != other->m_color) || (m_offset != other->m_offset)) {
            return false;
        }
        return true;
    }

    void* operator new(size_t size)
    {
        STARFISH_ASSERT(size == sizeof(ColorStop));
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(ColorStop)] = { 0 };
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(ColorStop, m_offset));
            descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(ColorStop));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }

    void* operator new(size_t size, ColorStop* colorStop)
    {
        return colorStop;
    }

    void* operator new[](size_t size) = delete;

private:
    Unit::Color m_color;
    Length m_offset;
    bool m_specified;
};

struct GradientDrawingInfo : public gc {
    float x1;
    float y1;
    float x2;
    float y2;
    float r1;
    float r2;
    float firstRadius;
    float secondRadius;
    GCVector<ColorStop*> colorStops;

    GradientDrawingInfo()
        : x1(0.0f)
        , y1(0.0f)
        , x2(0.0f)
        , y2(0.0f)
        , r1(0.0f)
        , r2(0.0f)
        , firstRadius(0.0f)
        , secondRadius(0.0f)
        , colorStops()
    {
    }

    void* operator new(size_t size)
    {
        STARFISH_ASSERT(size == sizeof(GradientDrawingInfo));
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(GradientDrawingInfo)] = { 0 };
            GC_set_bit(desc, GC_WORD_OFFSET(GradientDrawingInfo, colorStops));
            descr = GC_make_descriptor(desc, GC_WORD_LEN(GradientDrawingInfo));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }

    void* operator new(size_t size, GradientDrawingInfo* gradientDrawingInfo)
    {
        return gradientDrawingInfo;
    }

    void* operator new[](size_t size) = delete;
};

class GradientData : public gc {
public:
    GradientData(GradientType gradientType)
        : m_type(gradientType)
        , m_horizentalSide()
        , m_verticalSide()
        , m_colorStopList()
    {
    }

    virtual ~GradientData()
    {
    }

    GradientType type()
    {
        return m_type;
    }

    LinearGradientData* asLinearGradientData();
    RadialGradientData* asRadialGradientData();

    SideValue horizontalSide()
    {
        return m_horizentalSide;
    }

    void setHorizontalSide(SideValue side);

    SideValue verticalSide()
    {
        return m_verticalSide;
    }

    void setVerticalSide(SideValue side);

    GCVector<ColorStop*>& colorStopList()
    {
        return m_colorStopList;
    }

    virtual GradientDrawingInfo* makeGradientDrawingInfo(const Unit::Rect& rect,
                                                         FrameBox* box) = 0;

    virtual CSSGradientValue* convertToCSSGradientValue() = 0;

    virtual void checkComputed(Length curFontSize, Length rootFontSize,
                               Font* font, LayoutSize windowSize,
                               ComputedStyle* cs);

    void makeSpecifiedColorStops(GCVector<ColorStop*>& out, float& x1,
                                 float& y1, float& r1, float& x2, float& y2,
                                 float& r2, FrameBox* owner);

    virtual bool equals(GradientData* other) const;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(GradientData, m_colorStopList));
    }

    void convertColorStopsToCSSColorStops(GCVector<CSSColorStop*>& out);

    GradientType m_type;
    SideValue m_horizentalSide;
    SideValue m_verticalSide;
    GCVector<ColorStop*> m_colorStopList;
};

class LinearGradientData : public GradientData {
public:
    LinearGradientData(float angleDeg = 180.0f);

    float angle()
    {
        return m_angleDeg;
    }

    void setAngle(float val)
    {
        m_angleDeg = val;
    }

    bool computeEndPointsFromAngle(const Unit::Rect& rect, const float angleDeg,
                                   float& x1, float& y1, float& x2, float& y2);
    bool computeEndPoints(const Unit::Rect& rect, float& x1, float& y1,
                          float& x2, float& y2);

    virtual GradientDrawingInfo* makeGradientDrawingInfo(
        const Unit::Rect& rect, FrameBox* box) override;
    virtual CSSGradientValue* convertToCSSGradientValue() override;
    virtual void checkComputed(Length curFontSize, Length rootFontSize,
                               Font* font, LayoutSize windowSize,
                               ComputedStyle* cs) override;
    virtual bool equals(GradientData* other) const override;

    void* operator new(size_t size)
    {
        STARFISH_ASSERT(size == sizeof(LinearGradientData));
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(LinearGradientData)] = { 0 };
            GradientData::fillGCDescriptor(desc);
            descr = GC_make_descriptor(desc, GC_WORD_LEN(LinearGradientData));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }

    void* operator new(size_t size, LinearGradientData* linearGradientData)
    {
        return linearGradientData;
    }

    void* operator new[](size_t size) = delete;

private:
    float m_angleDeg;
};

class RadialGradientData : public GradientData {
public:
    RadialGradientData();

    RadialGradientShape shape()
    {
        return m_shape;
    }

    void setShape(RadialGradientShape shape)
    {
        m_shape = shape;
    }

    Length horizentalSideOffset()
    {
        return m_horizentalSideOffset;
    }

    void setHorizontalSideOffset(Length offset)
    {
        m_horizentalSideOffset = offset;
    }

    Length verticalSideOffset()
    {
        return m_verticalSideOffset;
    }

    void setVerticalSideOffset(Length offset)
    {
        m_verticalSideOffset = offset;
    }
    Length firstRadius()
    {
        return m_firstRadius;
    }

    void setFirstRadius(Length radius)
    {
        m_firstRadius = radius;
    }

    Length secondRadius()
    {
        return m_secondRadius;
    }

    void setSecondRadius(Length radius)
    {
        m_secondRadius = radius;
    }

    RadialGradientSizeKeyword gradientSizeKeyword()
    {
        return m_gradientSizeKeyword;
    }

    void setKeyword(RadialGradientSizeKeyword keyword)
    {
        m_gradientSizeKeyword = keyword;
    }

    bool computeEndPoints(const Unit::Rect& rect, FrameBox* owner, float& x1,
                          float& y1, float& r1, float& x2, float& y2, float& r2,
                          float& firstRadius, float& secondRadius);

    virtual GradientDrawingInfo* makeGradientDrawingInfo(
        const Unit::Rect& rect, FrameBox* box) override;
    virtual CSSGradientValue* convertToCSSGradientValue() override;
    virtual void checkComputed(Length curFontSize, Length rootFontSize,
                               Font* font, LayoutSize windowSize,
                               ComputedStyle* cs) override;
    virtual bool equals(GradientData* other) const override;

    void* operator new(size_t size)
    {
        STARFISH_ASSERT(size == sizeof(RadialGradientData));
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(RadialGradientData)] = { 0 };
            GC_set_bit(desc, GC_WORD_OFFSET(RadialGradientData,
                                            m_horizentalSideOffset));
            GC_set_bit(
                desc, GC_WORD_OFFSET(RadialGradientData, m_verticalSideOffset));
            GC_set_bit(desc, GC_WORD_OFFSET(RadialGradientData, m_firstRadius));
            GC_set_bit(desc,
                       GC_WORD_OFFSET(RadialGradientData, m_secondRadius));
            GradientData::fillGCDescriptor(desc);
            descr = GC_make_descriptor(desc, GC_WORD_LEN(RadialGradientData));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }

    void* operator new(size_t size, RadialGradientData* radialGradientData)
    {
        return radialGradientData;
    }

    void* operator new[](size_t size) = delete;

private:
    void computeEndPointsFromSideValue(const Unit::Rect& rect, FrameBox* owner,
                                       float& x, float& y);
    void radiusToSide(const float x2, const float y2, const Unit::Rect& rect,
                      bool (*compare)(float, float), float& outDx,
                      float& outDy);

    void radiusToCorner(const float x2, const float y2, const Unit::Rect& rect,
                        bool (*compare)(float, float), float& r1, float& r2);

    RadialGradientShape m_shape;

    // Position of gradient center
    Length m_horizentalSideOffset;
    Length m_verticalSideOffset;

    // size of the gradient's ending shape
    Length m_firstRadius;
    Length m_secondRadius;
    RadialGradientSizeKeyword m_gradientSizeKeyword;
};
} // namespace StarFish
