#ifndef __StarFishComputedStyle__
#define __StarFishComputedStyle__

#include "style/Style.h"
#include "style/StyleBackgroundData.h"
#include "style/StyleSurroundData.h"
#include "style/StyleTransformData.h"
#include "style/StyleTransformOrigin.h"
#include "style/DefaultStyle.h"

namespace StarFish {

class Frame;

enum ComputedStyleDamage {
    ComputedStyleDamageNone = 0,
    ComputedStyleDamageInherited = 1,
    ComputedStyleDamageRebuildFrame = 1 << 1,
    ComputedStyleDamageLayout = 1 << 2,
    ComputedStyleDamagePainting = 1 << 3,
    ComputedStyleDamageComposite = 1 << 4,
};

class ComputedStyle : public gc {
    friend class StyleResolver;
    friend void resolveDOMStyleInner(StyleResolver* resolver, Element* element, ComputedStyle* parentStyle, bool force);
    friend ComputedStyleDamage compareStyle(ComputedStyle* oldStyle, ComputedStyle* newStyle);

public:
    ComputedStyle()
    {
        m_font = nullptr;

        m_inheritedStyles.m_color = Color(0, 0, 0, 255);
        m_inheritedStyles.m_fontSize = Length(Length::Fixed, DEFAULT_FONT_SIZE);
        m_inheritedStyles.m_fontWeight = FontWeightValue::NormalFontWeightValue;
        m_inheritedStyles.m_direction = DirectionValue::LtrDirectionValue;
        m_inheritedStyles.m_visibility = VisibilityValue::VisibleVisibilityValue;
        m_inheritedStyles.m_letterSpacing = Length(Length::Fixed, 0);
        // -100 is used to represent 'normal' value.
        m_inheritedStyles.m_lineHeight = Length(Length::Percent, -100);

        initNonInheritedStyles();
    }

    ComputedStyle(ComputedStyle* from)
    {
        m_font = nullptr;

        m_inheritedStyles = from->m_inheritedStyles;
        initNonInheritedStyles();
    }

    DisplayValue originalDisplay()
    {
        return m_originalDisplay;
    }

    DisplayValue display()
    {
        return m_display;
    }

    void setDisplay(DisplayValue v)
    {
        m_display = v;
    }

    PositionValue position()
    {
        return m_position;
    }

    Length width()
    {
        return m_width;
    }

    void setWidth(const Length& l)
    {
        m_width = l;
    }

    Length height()
    {
        return m_height;
    }

    void setHeight(const Length& l)
    {
        m_height = l;
    }

    void setColor(Color r)
    {
        m_inheritedStyles.m_color = r;
    }

    Color color()
    {
        return m_inheritedStyles.m_color;
    }

    VerticalAlignValue verticalAlign()
    {
        return m_verticalAlign;
    }

    void setVerticalAlign(VerticalAlignValue v)
    {
        m_verticalAlign = v;
    }

    Length verticalAlignLength()
    {
        return m_verticalAlignLength;
    }

    void setVerticalAlignLength(Length l)
    {
        setVerticalAlign(VerticalAlignValue::NumericVAlignValue);
        m_verticalAlignLength = l;
    }

    bool isNumericVerticalAlign()
    {
        return (verticalAlign() == VerticalAlignValue::NumericVAlignValue);
    }

    TextAlignValue textAlign()
    {
        if (m_inheritedStyles.m_textAlign == TextAlignValue::NamelessTextAlignValue) {
            if (m_inheritedStyles.m_direction == DirectionValue::RtlDirectionValue)
                return TextAlignValue::RightTextAlignValue;
            return TextAlignValue::LeftTextAlignValue;
        }
        return m_inheritedStyles.m_textAlign;
    }

    void setTextAlign(TextAlignValue t)
    {
        m_inheritedStyles.m_textAlign = t;
    }

    TextDecorationValue textDecoration()
    {
        return m_textDecoration;
    }

    void setTextDecoration(TextDecorationValue decoration)
    {
        m_textDecoration = decoration;
    }

    DirectionValue direction()
    {
        return m_inheritedStyles.m_direction;
    }

    void setLineHeight(Length length)
    {
        m_inheritedStyles.m_lineHeight = length;
    }

    void setBackgroundIfNeeded()
    {
        if (m_background == NULL) {
            m_background = new StyleBackgroundData();
        }
    }

    bool hasTransforms(Frame* frame);

    // DO NOT USE THIS FUNCTION
    StyleTransformDataGroup* uncheckedTransforms()
    {
        return m_transforms;
    }

    StyleTransformDataGroup* transforms(Frame* frame);

    SkMatrix transformsToMatrix(LayoutUnit containerWidth, LayoutUnit containerHeight);
    void setTransformIfNeeded()
    {
        if (m_transforms == NULL) {
            m_transforms = new StyleTransformDataGroup();
        }
    }

    void setTransformMatrix(double a, double b, double c, double d, double e, double f)
    {
        setTransformIfNeeded();
        StyleTransformData t(StyleTransformData::OperationType::Matrix);
        t.setMatrix(a, b, c, d, e, f);
        m_transforms->append(t);
    }

    void setTransformScale(double a, double b)
    {
        setTransformIfNeeded();
        StyleTransformData t(StyleTransformData::OperationType::Scale);
        t.setScale(a, b);
        m_transforms->append(t);
    }

    void setTransformRotate(double a)
    {
        setTransformIfNeeded();
        StyleTransformData t(StyleTransformData::OperationType::Rotate);
        t.setRotate(a);
        m_transforms->append(t);
    }

    void setTransformSkew(double a, double b)
    {
        setTransformIfNeeded();
        StyleTransformData t(StyleTransformData::OperationType::Skew);
        t.setSkew(a, b);
        m_transforms->append(t);
    }

    void setTransformTranslate(Length a, Length b)
    {
        setTransformIfNeeded();
        StyleTransformData t(StyleTransformData::OperationType::Translate);
        t.setTranslate(a, b);
        m_transforms->append(t);
    }

    void setTransformOriginIfNeeded()
    {
        if (m_transformOrigin == NULL)
            m_transformOrigin = new StyleTransformOrigin();
    }

    void setTransformOriginValue(Length x, Length y)
    {
        setTransformOriginIfNeeded();
        m_transformOrigin->setOriginValue(x, y);
    }

    void setBackgroundColor(Color color)
    {
        setBackgroundIfNeeded();
        m_background->setBgColor(color);
    }

    void setBackgroundImage(String* img)
    {
        setBackgroundIfNeeded();
        m_background->setBgImage(img);
    }

    void setBackgroundImageResource(ImageResource* img)
    {
        setBackgroundIfNeeded();
        m_background->setBgImageResource(img);
    }

    void setBackgroundRepeatX(BackgroundRepeatValue repeat)
    {
        setBackgroundIfNeeded();
        m_background->setRepeatX(repeat);
    }

    void setBackgroundRepeatY(BackgroundRepeatValue repeat)
    {
        setBackgroundIfNeeded();
        m_background->setRepeatY(repeat);
    }

    void setBackgroundSizeType(BackgroundSizeType type)
    {
        setBackgroundIfNeeded();
        m_background->setSizeType(type);
    }

    void setBackgroundSizeValue(LengthSize* size)
    {
        setBackgroundIfNeeded();
        m_background->setSizeValue(size);
    }

    Color backgroundColor()
    {
        if (m_background == NULL)
            return Color();
        return m_background->bgColor();
    }

    String* backgroundImage()
    {
        if (m_background == NULL)
            return String::emptyString;
        return m_background->bgImage();
    }

    ImageData* backgroundImageData()
    {
        if (m_background == NULL)
            return NULL;
        return m_background->bgImageData();
    }

    BackgroundRepeatValue backgroundRepeatX()
    {
        if (m_background == NULL)
            return BackgroundRepeatValue::RepeatRepeatValue;
        return m_background->repeatX();
    }

    BackgroundRepeatValue backgroundRepeatY()
    {
        if (m_background == NULL)
            return BackgroundRepeatValue::RepeatRepeatValue;
        return m_background->repeatY();
    }

    BackgroundSizeType bgSizeType()
    {
        if (m_background == NULL)
            return BackgroundSizeType::SizeValue;
        return m_background->sizeType();
    }

    LengthSize* bgSizeValue()
    {
        if (m_background == NULL)
            return new LengthSize();

        STARFISH_ASSERT(m_background && m_background->sizeType() == BackgroundSizeType::SizeValue);
        return m_background->sizeValue();
    }

    Font* font()
    {
        STARFISH_ASSERT(m_font);
        return m_font;
    }

    Length lineHeight()
    {
        // According to the CSS spec, the computed value is the absolute value for <length> and <percentage> & otherwise as specified.
        // However, our computed value is the absolute value.
        return m_inheritedStyles.m_lineHeight;
    }

    bool hasNormalLineHeight()
    {
        return m_inheritedStyles.m_lineHeight.isPercent() && m_inheritedStyles.m_lineHeight.percent() == -100;
    }

    float opacity()
    {
        return m_opacity;
    }

    int32_t zIndex()
    {
        return m_zIndex;
    }

    bool IsSpecifiedZIndex()
    {
        return m_zIndexSpecifiedByUser;
    }

    StyleBackgroundData* background()
    {
        return m_background;
    }

    bool hasBorderColor()
    {
        if (m_surround == nullptr) {
            return false;
        } else {
            return m_surround->border.hasBorderColor();
        }
    }

    bool hasBorderStyle()
    {
        if (m_surround == nullptr) {
            return false;
        } else {
            return m_surround->border.hasBorderStyle();
        }
    }

    bool hasBorderImageData()
    {
        if (m_surround == nullptr) {
            return false;
        } else {
            return m_surround->border.hasBorderImageData();
        }
    }

    Color borderTopColor()
    {
        if (m_surround == nullptr || !m_surround->border.top().hasBorderColor()) {
            return m_inheritedStyles.m_color;
        } else {
            return m_surround->border.top().color();
        }
    }

    Color borderRightColor()
    {
        if (m_surround == nullptr || !m_surround->border.right().hasBorderColor()) {
            return m_inheritedStyles.m_color;
        } else {
            return m_surround->border.right().color();
        }
    }

    Color borderBottomColor()
    {
        if (m_surround == nullptr || !m_surround->border.bottom().hasBorderColor()) {
            return m_inheritedStyles.m_color;
        } else {
            return m_surround->border.bottom().color();
        }
    }

    Color borderLeftColor()
    {
        if (m_surround == nullptr || !m_surround->border.left().hasBorderColor()) {
            return m_inheritedStyles.m_color;
        } else {
            return m_surround->border.left().color();
        }
    }

    BorderStyleValue borderTopStyle()
    {
        if (m_surround == nullptr) {
            return BorderStyleValue::BNone;
        } else {
            return m_surround->border.top().style();
        }
    }

    BorderStyleValue borderRightStyle()
    {
        if (m_surround == nullptr) {
            return BorderStyleValue::BNone;
        } else {
            return m_surround->border.right().style();
        }
    }

    BorderStyleValue borderBottomStyle()
    {
        if (m_surround == nullptr) {
            return BorderStyleValue::BNone;
        } else {
            return m_surround->border.bottom().style();
        }
    }

    BorderStyleValue borderLeftStyle()
    {
        if (m_surround == nullptr) {
            return BorderStyleValue::BNone;
        } else {
            return m_surround->border.left().style();
        }
    }

    Length borderTopWidth()
    {
        if (m_surround == nullptr) {
            return Length(Length::Fixed, 0);
        } else {
            return m_surround->border.top().width();
        }
    }

    Length borderRightWidth()
    {
        if (m_surround == nullptr) {
            return Length(Length::Fixed, 0);
        } else {
            return m_surround->border.right().width();
        }
    }

    Length borderBottomWidth()
    {
        if (m_surround == nullptr) {
            return Length(Length::Fixed, 0);
        } else {
            return m_surround->border.bottom().width();
        }
    }

    Length borderLeftWidth()
    {
        if (m_surround == nullptr) {
            return Length(Length::Fixed, 0);
        } else {
            return m_surround->border.left().width();
        }
    }

    StyleTransformOrigin* transformOrigin()
    {
        return m_transformOrigin;
    }

    bool hasTransformOrigin()
    {
        return m_transformOrigin != nullptr;
    }

    void clearBorderTopColor()
    {
        if (m_surround)
            surround()->border.top().clearColor();
    }

    void clearBorderRightColor()
    {
        if (m_surround)
            surround()->border.right().clearColor();
    }

    void clearBorderBottomColor()
    {
        if (m_surround)
            surround()->border.bottom().clearColor();
    }

    void clearBorderLeftColor()
    {
        if (m_surround)
            surround()->border.left().clearColor();
    }

    void setBorderTopColor(Color color)
    {
        surround()->border.top().setColor(color);
    }

    void setBorderRightColor(Color color)
    {
        surround()->border.right().setColor(color);
    }

    void setBorderBottomColor(Color color)
    {
        surround()->border.bottom().setColor(color);
    }

    void setBorderLeftColor(Color color)
    {
        surround()->border.left().setColor(color);
    }

    void setBorderTopStyle(BorderStyleValue style)
    {
        surround()->border.top().setStyle(style);
    }

    void setBorderRightStyle(BorderStyleValue style)
    {
        surround()->border.right().setStyle(style);
    }

    void setBorderBottomStyle(BorderStyleValue style)
    {
        surround()->border.bottom().setStyle(style);
    }

    void setBorderLeftStyle(BorderStyleValue style)
    {
        surround()->border.left().setStyle(style);
    }

    void setBorderTopWidth(Length width)
    {
        surround()->border.top().setWidth(width);
    }

    void setBorderRightWidth(Length width)
    {
        surround()->border.right().setWidth(width);
    }

    void setBorderBottomWidth(Length width)
    {
        surround()->border.bottom().setWidth(width);
    }

    void setBorderLeftWidth(Length width)
    {
        surround()->border.left().setWidth(width);
    }

    String* borderImageSource()
    {
        if (m_surround)
            return surround()->border.image().url();
        return String::emptyString;
    }
    LengthBox borderImageSlices() { return surround()->border.image().slices(); }
    bool borderImageSliceFill() { return surround()->border.image().sliceFill(); }
    BorderImageRepeatValue borderImageRepeatX() { return surround()->border.image().repeatX(); }
    BorderImageRepeatValue borderImageRepeatY() { return surround()->border.image().repeatY(); }
    BorderImageLengthBox borderImageWidths() { return surround()->border.image().widths(); }

    void setBorderImageSource(String* url) { surround()->border.image().setUrl(url); }
    void setBorderImageSlices(LengthBox slices) { surround()->border.image().setSlices(slices); }
    void setBorderImageSliceFill(bool fill) { surround()->border.image().setSliceFill(fill); }
    void setBorderImageRepeatX(BorderImageRepeatValue value) { surround()->border.image().setRepeatX(value); }
    void setBorderImageRepeatY(BorderImageRepeatValue value) { surround()->border.image().setRepeatY(value); }
    void setBorderImageWidths(BorderImageLengthBox value) { surround()->border.image().setWidths(value); }
    void setBorderImageResource(ImageResource* value) { surround()->border.image().setImageResource(value); }

    void setBorderImageSliceFromOther(ComputedStyle* other)
    {
        setBorderImageSlices(other->borderImageSlices());
        setBorderImageSliceFill(other->borderImageSliceFill());
    }
    void setBorderImageRepeatFromOther(ComputedStyle* other)
    {
        setBorderImageRepeatX(other->borderImageRepeatX());
        setBorderImageRepeatY(other->borderImageRepeatY());
    }

    StyleSurroundData* surround()
    {
        if (m_surround == nullptr) {
            m_surround = new StyleSurroundData();
        }
        return m_surround;
    }

    OverflowValue overflow()
    {
        return m_overflow;
    }

    void setTop(Length top)
    {
        surround()->offset.setTop(top);
    }

    void setRight(Length right)
    {
        surround()->offset.setRight(right);
    }

    void setBottom(Length bottom)
    {
        surround()->offset.setBottom(bottom);
    }

    void setLeft(Length left)
    {
        surround()->offset.setLeft(left);
    }

    void setMarginTop(Length top)
    {
        surround()->margin.setTop(top);
    }

    void setMarginBottom(Length bottom)
    {
        surround()->margin.setBottom(bottom);
    }

    void setMarginRight(Length right)
    {
        surround()->margin.setRight(right);
    }

    void setMarginLeft(Length left)
    {
        surround()->margin.setLeft(left);
    }

    void setPaddingTop(Length top)
    {
        surround()->padding.setTop(top);
    }

    void setPaddingRight(Length right)
    {
        surround()->padding.setRight(right);
    }

    void setPaddingBottom(Length bottom)
    {
        surround()->padding.setBottom(bottom);
    }

    void setPaddingLeft(Length left)
    {
        surround()->padding.setLeft(left);
    }

    Length top()
    {
        if (m_surround == nullptr) {
            return Length();
        } else {
            return m_surround->offset.top();
        }
    }

    Length right()
    {
        if (m_surround == nullptr) {
            return Length();
        } else {
            return m_surround->offset.right();
        }
    }

    Length bottom()
    {
        if (m_surround == nullptr) {
            return Length();
        } else {
            return m_surround->offset.bottom();
        }
    }

    Length left()
    {
        if (m_surround == nullptr) {
            return Length();
        } else {
            return m_surround->offset.left();
        }
    }

    Length marginTop()
    {
        if (m_surround == nullptr) {
            return Length(Length::Fixed, 0);
        } else {
            return m_surround->margin.top();
        }
    }

    Length marginRight()
    {
        if (m_surround == nullptr) {
            return Length(Length::Fixed, 0);
        } else {
            return m_surround->margin.right();
        }
    }

    Length marginBottom()
    {
        if (m_surround == nullptr) {
            return Length(Length::Fixed, 0);
        } else {
            return m_surround->margin.bottom();
        }
    }

    Length marginLeft()
    {
        if (m_surround == nullptr) {
            return Length(Length::Fixed, 0);
        } else {
            return m_surround->margin.left();
        }
    }

    Length paddingTop()
    {
        if (m_surround == nullptr) {
            return Length(Length::Fixed, 0);
        } else {
            return m_surround->padding.top();
        }
    }

    Length paddingRight()
    {
        if (m_surround == nullptr) {
            return Length(Length::Fixed, 0);
        } else {
            return m_surround->padding.right();
        }
    }

    Length paddingBottom()
    {
        if (m_surround == nullptr) {
            return Length(Length::Fixed, 0);
        } else {
            return m_surround->padding.bottom();
        }
    }

    Length paddingLeft()
    {
        if (m_surround == nullptr) {
            return Length(Length::Fixed, 0);
        } else {
            return m_surround->padding.left();
        }
    }

    Length fontSize()
    {
        return m_inheritedStyles.m_fontSize;
    }

    void setTextOverflow(TextOverflowValue val)
    {
        m_textOverflow = val;
    }

    void setLetterSpacing(Length len)
    {
        m_inheritedStyles.m_letterSpacing = len;
    }

    TextOverflowValue textOverflow()
    {
        return m_textOverflow;
    }

    VisibilityValue visibility()
    {
        return m_inheritedStyles.m_visibility;
    }

    FontStyleValue fontStyle()
    {
        return m_inheritedStyles.m_fontStyle;
    }

    FontWeightValue fontWeight()
    {
        return m_inheritedStyles.m_fontWeight;
    }

    Length letterSpacing()
    {
        return m_inheritedStyles.m_letterSpacing;
    }

    static VerticalAlignValue initialVerticalAlign() { return VerticalAlignValue::BaselineVAlignValue; }
    static TextAlignValue initialTextAlign() { return TextAlignValue::NamelessTextAlignValue; }
    static Length initialPadding() { return Length(Length::Fixed, 0); }
    static Length initialMargin() { return Length(Length::Fixed, 0); }
    static String* initialBgImage() { return String::emptyString; }
    static String* initialBorderImageSource() { return String::emptyString; }
    static LengthBox initialBorderImageSlices() { return LengthBox(Length(Length::Fixed, 0), Length(Length::Fixed, 0), Length(Length::Fixed, 0), Length(Length::Fixed, 0)); }
    static bool initialBorderImageSliceFill() { return false; }
    static BorderImageRepeatValue initialBorderImageRepeat() { return BorderImageRepeatValue::StretchValue; }

    void loadResources(Node* consumer, ComputedStyle* prevComputedStyleValueForReferenceLoadedResources = nullptr);
    void arrangeStyleValues(ComputedStyle* parentStyle, Node* current = nullptr);

    void clearTransforms()
    {
        m_transforms = nullptr;
    }

    void setUnicodeBidi(UnicodeBidiValue value)
    {
        m_unicodeBidi = value;
    }

    UnicodeBidiValue unicodeBidi()
    {
        return m_unicodeBidi;
    }

protected:
    void initNonInheritedStyles()
    {
        m_display = DisplayValue::InlineDisplayValue;
        m_opacity = 1;
        m_zIndex = 0;
        m_zIndexSpecifiedByUser = false;
        m_background = nullptr;
        m_surround = nullptr;
        m_overflow = OverflowValue::VisibleOverflow;
        m_textDecoration = TextDecorationValue::NoneTextDecorationValue;
        m_verticalAlign = initialVerticalAlign();
        m_transforms = nullptr;
        m_transformOrigin = nullptr;
        m_unicodeBidi = UnicodeBidiValue::NormalUnicodeBidiValue;
    }

    // NOTICE
    // if you add new property, you MUST implement comparing style for new property in [compareStyle function]

    struct InheritedStyles {
        Color m_color;
        Length m_fontSize;
        Length m_letterSpacing;
        Length m_lineHeight;
        FontStyleValue m_fontStyle : 2;
        FontWeightValue m_fontWeight : 4;
        TextAlignValue m_textAlign : 3;
        DirectionValue m_direction : 2;
        VisibilityValue m_visibility : 1;
    } m_inheritedStyles;

    DisplayValue m_display : 3;
    DisplayValue m_originalDisplay : 3;
    PositionValue m_position : 2;
    TextOverflowValue m_textOverflow : 1;
    VerticalAlignValue m_verticalAlign : 4;
    OverflowValue m_overflow : 1;
    TextDecorationValue m_textDecoration : 3;
    UnicodeBidiValue m_unicodeBidi : 2;

    Length m_width;
    Length m_height;
    Length m_verticalAlignLength;

    float m_opacity;
    int32_t m_zIndex;
    bool m_zIndexSpecifiedByUser;
    Font* m_font;
    StyleBackgroundData* m_background;
    StyleSurroundData* m_surround;
    StyleTransformDataGroup* m_transforms;
    StyleTransformOrigin* m_transformOrigin;
};

ComputedStyleDamage compareStyle(ComputedStyle* oldStyle, ComputedStyle* newStyle);
}

#endif
