/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/animation/Animation.h"
#include "core/animation/AnimationUtil.h"
#include "core/animation/CubicBezier.h"
#include "core/animation/Steps.h"
#include "core/dom/Node.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/page/BrowsingContext.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameTreeBuilder.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/style/CSSProperty.h"
#include "core/style/ComputedStyle.h"
#include "platform/window/PlatformWindow.h"

namespace StarFish {

#define _DAMAGED_KEYS(PropName, ...) (damagedKeys[PropName])

// args: property [, dependency_1 [, dependency_2]]
#define NEED_TRANSITION(...)       \
    (_DAMAGED_KEYS(__VA_ARGS__) && \
     (isPropertyAll || _checkCSSProperty(property, __VA_ARGS__)))

#define RETURN_NEED_TRANSITION(...)     \
    if (NEED_TRANSITION(__VA_ARGS__)) { \
        return true;                    \
    }

static inline bool _checkCSSProperty(CSSStyleValuePair::KeyKind kind,
                                     CSSStyleValuePair::KeyKind a)
{
    return kind == a;
}

static inline bool _checkCSSProperty(CSSStyleValuePair::KeyKind kind,
                                     CSSStyleValuePair::KeyKind a,
                                     CSSStyleValuePair::KeyKind b)
{
    return kind == a || kind == b;
}

static inline bool _checkCSSProperty(CSSStyleValuePair::KeyKind kind,
                                     CSSStyleValuePair::KeyKind a,
                                     CSSStyleValuePair::KeyKind b,
                                     CSSStyleValuePair::KeyKind c)
{
    return kind == a || kind == b || kind == c;
}

void* RareComputedStyleData::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(RareComputedStyleData));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(RareComputedStyleData)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(RareComputedStyleData, m_styles));
        descr =
            GC_make_descriptor(obj_bitmap, GC_WORD_LEN(RareComputedStyleData));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void* ComputedStyle::InheritedStylesRareData::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(ComputedStyle::InheritedStylesRareData));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(
            ComputedStyle::InheritedStylesRareData)] = { 0 };
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(ComputedStyle::InheritedStylesRareData,
                                  m_textShadowDataList));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(ComputedStyle::InheritedStylesRareData,
                                  m_listStyleData.m_counterStyle));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(ComputedStyle::InheritedStylesRareData,
                                  m_listStyleData.m_image));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(ComputedStyle::InheritedStylesRareData,
                                  m_listStyleData.m_imageResource));
        descr = GC_make_descriptor(
            obj_bitmap, GC_WORD_LEN(ComputedStyle::InheritedStylesRareData));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void* ComputedStyle::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(ComputedStyle));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(ComputedStyle)] = { 0 };
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(ComputedStyle, m_inheritedStyles.m_rareData));
        GC_set_bit(
            obj_bitmap,
            GC_WORD_OFFSET(ComputedStyle, m_inheritedStyles.m_fontFamilyDatas));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(ComputedStyle,
                                              m_inheritedStyles.m_lineHeight));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(ComputedStyle, m_font));
        GC_set_bit(
            obj_bitmap,
            GC_WORD_OFFSET(ComputedStyle, m_rareComputedStyleData.m_styles));
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(ComputedStyle));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

bool ComputedStyle::hasTransforms(Frame* frame)
{
    return transforms(frame) != nullptr;
}

bool ComputedStyle::hasComplexTransforms(Frame* frame)
{
    StyleTransformDataGroup* t = transforms(frame);
    if (t) {
        return t->hasComplexTransform();
    } else {
        return false;
    }
}

bool ComputedStyle::has3DTransforms(Frame* frame)
{
    StyleTransformDataGroup* t = transforms(frame);
    if (t) {
        return t->has3DTransform();
    } else {
        return false;
    }
}

StyleTransformDataGroup* ComputedStyle::transforms(Frame* frame)
{
    if (!hasRareComputeStyleData()) {
        return nullptr;
    }

    // https://www.w3.org/TR/css-transforms-1/#transformable-element
    if (frame && !frame->isTransformable()) {
        return nullptr;
    }

    StyleTransformDataGroup* transforms = m_rareComputedStyleData.transforms();
    if (transforms) {
        return transforms;
    }

    return nullptr;
}

AnimationTimingFunction* ComputedStyle::knownTransitionTimingFunction(
    TransitionTimingFunctionValue v)
{
    switch (v) {
    case TransitionTimingFunctionValue::TransitionTimingFunctionEaseValue:
        return new CubicBezier(0.25, 0.1, 0.25, 1);
    case TransitionTimingFunctionValue::TransitionTimingFunctionLinearValue:
        return new CubicBezier(0, 0, 1, 1);
    case TransitionTimingFunctionValue::TransitionTimingFunctionEaseInValue:
        return new CubicBezier(0.42, 0, 1, 1);
    case TransitionTimingFunctionValue::TransitionTimingFunctionEaseOutValue:
        return new CubicBezier(0.0, 0.0, 0.58, 1.0);
    case TransitionTimingFunctionValue::TransitionTimingFunctionEaseInOutValue:
        return new CubicBezier(0.42, 0.0, 0.58, 1.0);
    case TransitionTimingFunctionValue::TransitionTimingFunctionStepStartValue:
        return new Steps(1, false);
    case TransitionTimingFunctionValue::TransitionTimingFunctionStepEndValue:
        return new Steps(1, true);
    }
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

class StupidImageResourceClientBecauseItIsNotConsiderRePaintRegion
    : public ResourceClient {
public:
    StupidImageResourceClientBecauseItIsNotConsiderRePaintRegion(
        Resource* res, Document* document)
        : ResourceClient(res)
        , m_document(document)
    {
    }
    virtual void didLoadFinished()
    {
        m_document->setNeedsPainting();
    }

    Document* m_document;
};

void ComputedStyle::loadFont(Node* consumer, bool respectLetterSpacing)
{
    m_inheritedStyles.m_fontSize =
        Length(Length::Fixed,
               m_inheritedStyles.m_fontSize.specifiedFontValue(consumer));
    float fixedFontSize = this->fixedFontSize();

    char style = m_inheritedStyles.m_fontStyle;
    char fontWeight;

    switch (m_inheritedStyles.m_fontWeight) {
    case OneHundredFontWeightValue:
        fontWeight = 1;
        break;
    case TwoHundredsFontWeightValue:
        fontWeight = 2;
        break;
    case ThreeHundredsFontWeightValue:
        fontWeight = 3;
        break;
    case NormalFontWeightValue:
        fontWeight = 4;
        break;
    case FiveHundredsFontWeightValue:
        fontWeight = 5;
        break;
    case SixHundredsFontWeightValue:
        fontWeight = 6;
        break;
    case BoldFontWeightValue:
        fontWeight = 7;
        break;
    case EightHundredsFontWeightValue:
        fontWeight = 8;
        break;
    case NineHundredsFontWeightValue:
        fontWeight = 9;
        break;
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    FontSelector* fs = consumer->document()->fontSelector();
    Font* parentNodeFont = nullptr;
    if (consumer->parentNode() && consumer->parentNode()->style() &&
        consumer->parentNode()->style()->font()) {
        parentNodeFont = consumer->parentNode()->style()->font();
    }
    bool canUseParentFont = false;
    if (parentNodeFont) {
        ComputedStyle* parentStyle = consumer->parentNode()->style();
        if (parentStyle->fixedFontSize() == fixedFontSize &&
            parentStyle->fontStyle() == fontStyle() &&
            parentStyle->fontWeight() == this->fontWeight() &&
            parentStyle->letterSpacing() == letterSpacing()) {
            if (parentStyle->fontFamily()[0].m_length ==
                fontFamily()[0].m_length) {
                size_t len = fontFamily()[0].m_length;
                canUseParentFont = true;
                for (size_t i = 0; i < len; i++) {
                    auto a = parentStyle->fontFamily()[i + 1].m_familyName;
                    auto b = fontFamily()[i + 1].m_familyName;
                    if (a != b && !a->equals(b)) {
                        canUseParentFont = false;
                        break;
                    }
                }
            }
        }
    }

    float fixedLetterSpacing = 0;
    if (respectLetterSpacing || letterSpacing().isFixed()) {
        fixedLetterSpacing = letterSpacing().fixed();
    }

#ifdef STARFISH_ENABLE_TEST
    StarFish* sf = consumer->starFish();
    if (g_enablePixelTest) {
        String* str = String::fromUTF8("StarFishAhem");
        m_font = fs->loadFont(&str, 1, fixedFontSize, style, fontWeight,
                              fixedLetterSpacing);
    } else {
        if (sf->startUpFlag() & StarFishStartUpFlag::enableRegressionTest) {
            String* str = String::fromUTF8("SamsungOne");
            m_font = fs->loadFont(&str, 1, fixedFontSize, style, fontWeight,
                                  fixedLetterSpacing);
        } else {
            if (canUseParentFont) {
                m_font = parentNodeFont;
            } else {
                m_font = fs->loadFont(
                    (String**)&m_inheritedStyles.m_fontFamilyDatas[1],
                    m_inheritedStyles.m_fontFamilyDatas[0].m_length,
                    fixedFontSize, style, fontWeight, fixedLetterSpacing);
            }
        }
    }
#else
    if (canUseParentFont) {
        m_font = parentNodeFont;
    } else {
        m_font =
            fs->loadFont((String**)&m_inheritedStyles.m_fontFamilyDatas[1],
                         m_inheritedStyles.m_fontFamilyDatas[0].m_length,
                         fixedFontSize, style, fontWeight, fixedLetterSpacing);
    }
#endif
}

void ComputedStyle::loadBackgroundImage(
    Node* consumer,
    ComputedStyle* prevComputedStyleValueForReferenceLoadedResources)
{
    StarFish* sf = consumer->starFish();
    size_t bgIndex = 0;
    while (bgIndex < backgroundLayerSize()) {
        ImageValue* bImg = backgroundImage(bgIndex);

        if (bImg && bImg->type().isURL()) {
            ResourceURL* u = new ResourceURL(
                bImg->urlValue(), consumer->document()->baseURL()->baseURI());

            if (prevComputedStyleValueForReferenceLoadedResources &&
                prevComputedStyleValueForReferenceLoadedResources
                    ->background() &&
                prevComputedStyleValueForReferenceLoadedResources->background()
                    ->imageResource(bgIndex) &&
                *(prevComputedStyleValueForReferenceLoadedResources
                      ->background()
                      ->imageResource(bgIndex)
                      ->url()) == *u) {
                consumer->document()
                    ->resourceLoader()
                    .notifyImageResourceActiveState(
                        prevComputedStyleValueForReferenceLoadedResources
                            ->background()
                            ->imageResource(bgIndex));
                setBackgroundImageResource(
                    prevComputedStyleValueForReferenceLoadedResources
                        ->background()
                        ->imageResource(bgIndex),
                    bgIndex);
            } else {
                ImageResource* res =
                    consumer->document()->resourceLoader().fetchImage(u);
                setBackgroundImageResource(res, bgIndex);
                res->markThisResourceIsDoesNotAffectWindowOnLoad();
                res->addResourceClient(
                    new StupidImageResourceClientBecauseItIsNotConsiderRePaintRegion(
                        res, consumer->document()));
#ifdef STARFISH_ENABLE_TEST
                bool enableRegressionTest =
                    sf->startUpFlag() &
                    StarFishStartUpFlag::enableRegressionTest;
                res->request(
                    (g_enablePixelTest || enableRegressionTest)
                        ? Resource::ResourceRequestSyncLevel::AlwaysSync
                        : Resource::ResourceRequestSyncLevel::
                              SyncIfAlreadyLoaded,
                    consumer->document()->documentURI(), true);
#else
                res->request(
                    Resource::ResourceRequestSyncLevel::SyncIfAlreadyLoaded,
                    consumer->document()->documentURI(), true);
#endif
            }
        } else if (bImg && bImg->type().isGradient()) {
            // Do nothing at this time
        }
        bgIndex++;
    }
}

void ComputedStyle::loadBorderImage(
    Node* consumer,
    ComputedStyle* prevComputedStyleValueForReferenceLoadedResources)
{
    StarFish* sf = consumer->starFish();
    BorderData border = this->border();
    if (!border.image().url()->equals(String::emptyString)) {
        ResourceURL* u = new ResourceURL(
            border.image().url(), consumer->document()->baseURL()->baseURI());

        bool loaded = false;

        if (prevComputedStyleValueForReferenceLoadedResources) {
            BorderData prevBorder =
                prevComputedStyleValueForReferenceLoadedResources->border();
            if (prevBorder.hasBorderImageData()) {
                ImageResource* res = prevBorder.image().imageResource();
                if (*res->url() == *u) {
                    consumer->document()
                        ->resourceLoader()
                        .notifyImageResourceActiveState(res);
                    setBorderImageResource(res);
                    loaded = true;
                }
            }
        }

        if (!loaded) {
            ImageResource* res =
                consumer->document()->resourceLoader().fetchImage(u);
            res->markThisResourceIsDoesNotAffectWindowOnLoad();
            res->addResourceClient(
                new StupidImageResourceClientBecauseItIsNotConsiderRePaintRegion(
                    res, consumer->document()));
#ifdef STARFISH_ENABLE_TEST
            bool enableRegressionTest =
                sf->startUpFlag() & StarFishStartUpFlag::enableRegressionTest;
            res->request(
                (g_enablePixelTest || enableRegressionTest)
                    ? Resource::ResourceRequestSyncLevel::AlwaysSync
                    : Resource::ResourceRequestSyncLevel::SyncIfAlreadyLoaded,
                consumer->document()->documentURI(), true);
#else
            res->request(
                Resource::ResourceRequestSyncLevel::SyncIfAlreadyLoaded,
                consumer->document()->documentURI(), true);
#endif
            setBorderImageResource(res);
        }
    }
}

void ComputedStyle::loadListStyleImage(
    Node* consumer,
    ComputedStyle* prevComputedStyleValueForReferenceLoadedResources)
{
    StarFish* sf = consumer->starFish();
    const ListStyleData& listStyle = listStyleData();
    if (listStyle.image()->length() > 0) {
        ResourceURL* u = new ResourceURL(
            listStyle.image(), consumer->document()->baseURL()->baseURI());
        bool loaded = false;
        if (prevComputedStyleValueForReferenceLoadedResources) {
            const ListStyleData& prevListStyle =
                prevComputedStyleValueForReferenceLoadedResources
                    ->listStyleData();
            ImageResource* prevRes = prevListStyle.imageResource();
            if (prevRes && *prevRes->url() == *u) {
                consumer->document()
                    ->resourceLoader()
                    .notifyImageResourceActiveState(prevRes);
                setListStyleImage(prevRes);
                loaded = true;
            }
        }
        if (!loaded) {
            ImageResource* res =
                consumer->document()->resourceLoader().fetchImage(u);
            res->markThisResourceIsDoesNotAffectWindowOnLoad();
            res->addResourceClient(
                new StupidImageResourceClientBecauseItIsNotConsiderRePaintRegion(
                    res, consumer->document()));
#ifdef STARFISH_ENABLE_TEST
            bool enableRegressionTest =
                sf->startUpFlag() & StarFishStartUpFlag::enableRegressionTest;
            res->request(
                (g_enablePixelTest || enableRegressionTest)
                    ? Resource::ResourceRequestSyncLevel::AlwaysSync
                    : Resource::ResourceRequestSyncLevel::SyncIfAlreadyLoaded,
                consumer->document()->documentURI(), true);
#else
            res->request(
                Resource::ResourceRequestSyncLevel::SyncIfAlreadyLoaded,
                consumer->document()->documentURI(), true);
#endif
            setListStyleImage(res);
        }
    }
}

void ComputedStyle::loadResources(
    Node* consumer,
    ComputedStyle* prevComputedStyleValueForReferenceLoadedResources)
{
    loadBackgroundImage(consumer,
                        prevComputedStyleValueForReferenceLoadedResources);
    loadBorderImage(consumer,
                    prevComputedStyleValueForReferenceLoadedResources);
    loadListStyleImage(consumer,
                       prevComputedStyleValueForReferenceLoadedResources);
    loadFont(consumer);
}

void ComputedStyle::blockify(Node* current, bool force)
{
    // 9.7 Relationships between 'display', 'position', and 'float'
    if (m_originalDisplay != DisplayValue::NoneDisplayValue) {
        bool isAbsolutePositioned =
            position() == PositionValue::AbsolutePositionValue ||
            position() == PositionValue::FixedPositionValue;

        if (isAbsolutePositioned) {
            m_float = FloatValue::NoneFloatValue;
        }

        if (force || isAbsolutePositioned ||
            m_float != FloatValue::NoneFloatValue ||
            (current && current->isHTMLHtmlElement())) {
            switch (m_display) {
            case DisplayValue::InlineTableDisplayValue:
                m_display = DisplayValue::TableDisplayValue;
                break;
            case DisplayValue::InlineFlexDisplayValue:
                m_display = DisplayValue::FlexDisplayValue;
                break;
            case DisplayValue::InlineGridDisplayValue:
                m_display = DisplayValue::GridDisplayValue;
                break;
            case DisplayValue::InlineListItemDisplayValue:
            case DisplayValue::InlineDisplayValue:
            case DisplayValue::TableRowGroupDisplayValue:
            case DisplayValue::TableColumnDisplayValue:
            case DisplayValue::TableColumnGroupDisplayValue:
            case DisplayValue::TableHeaderGroupDisplayValue:
            case DisplayValue::TableFooterGroupDisplayValue:
            case DisplayValue::TableRowDisplayValue:
            case DisplayValue::TableCellDisplayValue:
            case DisplayValue::TableCaptionDisplayValue:
            case DisplayValue::InlineBlockDisplayValue:
                m_display = DisplayValue::BlockDisplayValue;
                break;
            default:
                break;
            }
        }

        if (current->isHTMLInputElement() || current->isHTMLButtonElement()) {
            switch (m_display) {
            case DisplayValue::InlineDisplayValue:
            case DisplayValue::InlineTableDisplayValue:
            case DisplayValue::InlineListItemDisplayValue:
            case DisplayValue::TableRowGroupDisplayValue:
            case DisplayValue::TableColumnDisplayValue:
            case DisplayValue::TableColumnGroupDisplayValue:
            case DisplayValue::TableHeaderGroupDisplayValue:
            case DisplayValue::TableFooterGroupDisplayValue:
            case DisplayValue::TableRowDisplayValue:
            case DisplayValue::TableCellDisplayValue:
            case DisplayValue::TableCaptionDisplayValue:
                m_display = DisplayValue::InlineBlockDisplayValue;
                break;
            default:
                break;
            }
        }
    }
}

void ComputedStyle::arrangeStyleValues(ComputedStyle* parentStyle,
                                       Node* current)
{
    m_originalDisplay = m_display;
    blockify(current, false);

    // https://www.w3.org/TR/css-overflow-3/#overflow-properties
    // visible is computed to 'auto' if either one of 'overflow-x' or
    // 'overflow-y'
    if (m_overflowX == OverflowValue::VisibleOverflow &&
        m_overflowY != OverflowValue::VisibleOverflow) {
        m_overflowX = OverflowValue::AutoOverflow;
    } else if (m_overflowY == OverflowValue::VisibleOverflow &&
               m_overflowX != OverflowValue::VisibleOverflow) {
        m_overflowY = OverflowValue::AutoOverflow;
    }

    if (fill() != InheritedStylesRareData().m_fill) {
        auto s = fill();
        s.updateCurrentColorToFixedColorIfNeeds(color());
        setFill(s);
    }

    StyleBackgroundData* background = this->background();
    if (background) {
        background->checkComputed(m_inheritedStyles.m_color);
    }

    if (!m_alignSelfSpecifiedByUser) {
        // https://www.w3.org/TR/css-flexbox-1/#propdef-align-self
        // initial value of  'align-self' is 'auto', 'auto' is computed to
        // parent's 'align-items' value; otherwise 'stretch'
        m_alignSelf = parentStyle->m_alignItems;
    }

    Length curFontSize = fontSize();
    Length rootFontSize = Length(Length::Fixed, DEFAULT_FONT_SIZE);
    HTMLHtmlElement* root = current->document()->rootElement();
    if (root->style()) {
        rootFontSize = root->style()->fontSize();
    }

    changeFontPercentToFixedIfNeeded(curFontSize, rootFontSize, font(),
                                     current);

    // When <center><table>...</table></center>, table is placed in the middle
    // of the parent block, but the content of the table is not affected by
    // <center>. To do so, Blink seems resets the text-align.
    if (m_inheritedStyles.m_textAlign ==
        TextAlignValue::StarFishCenterTextAlignValue) {
        switch (display()) {
        case DisplayValue::TableRowGroupDisplayValue:
        case DisplayValue::TableHeaderGroupDisplayValue:
        case DisplayValue::TableFooterGroupDisplayValue:
        case DisplayValue::TableRowDisplayValue:
        case DisplayValue::TableColumnGroupDisplayValue:
        case DisplayValue::TableColumnDisplayValue:
        case DisplayValue::TableCellDisplayValue:
            setTextAlign(TextAlignValue::StartTextAlignValue);
            break;
        default:
            break;
        }
    }
}

void ComputedStyle::changeFontPercentToFixedIfNeeded(Length curFontSize,
                                                     Length rootFontSize,
                                                     Font* font, Node* current)
{
    Window* w = current->window();
    LayoutSize windowSize(w->innerWidth(), w->innerHeight());

    if (!letterSpacing().isFixed()) {
        auto v = letterSpacing();
        v.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                windowSize.width(), windowSize.height(), this);
        setLetterSpacing(v);
        loadFont(current, true);
    }

    if (!lineHeight().isComputed()) {
        auto v = lineHeight();
        v.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                windowSize.width(), windowSize.height(), this);
        setLineHeight(v);
    }

    if (!textIndent().isComputed()) {
        auto v = textIndent();
        v.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                windowSize.width(), windowSize.height(), this);
        setTextIndent(v);
    }

    if (!textIndent().isComputed()) {
        auto v = textIndent();
        v.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                windowSize.width(), windowSize.height(), this);
        setTextIndent(v);
    }

    if (!horizontalBorderSpacing().isComputed()) {
        auto v = horizontalBorderSpacing();
        v.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                windowSize.width(), windowSize.height(), this);
        setHorizontalBorderSpacing(v);
    }

    if (!verticalBorderSpacing().isComputed()) {
        auto v = verticalBorderSpacing();
        v.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                windowSize.width(), windowSize.height(), this);
        setVerticalBorderSpacing(v);
    }

    if (!strokeWidth().isComputed()) {
        auto v = strokeWidth();
        v.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                windowSize.width(), windowSize.height(), this);
        setStrokeWidth(v);
    }

    if (hasRareComputeStyleData()) {
        Nullable<Length> width = m_rareComputedStyleData.width();
        if (width.hasValue()) {
            m_rareComputedStyleData.ensureWidth()->changeToFixedIfNeeded(
                curFontSize, rootFontSize, font, windowSize.width(),
                windowSize.height(), this);
        }

        Nullable<Length> height = m_rareComputedStyleData.height();
        if (height.hasValue()) {
            m_rareComputedStyleData.ensureHeight()->changeToFixedIfNeeded(
                curFontSize, rootFontSize, font, windowSize.width(),
                windowSize.height(), this);
        }

        Nullable<Length> minWidth = m_rareComputedStyleData.minWidth();
        if (minWidth.hasValue()) {
            m_rareComputedStyleData.ensureMinWidth()->changeToFixedIfNeeded(
                curFontSize, rootFontSize, font, windowSize.width(),
                windowSize.height(), this);
        }
        Nullable<Length> maxWidth = m_rareComputedStyleData.maxWidth();
        if (maxWidth.hasValue()) {
            m_rareComputedStyleData.ensureMaxWidth()->changeToFixedIfNeeded(
                curFontSize, rootFontSize, font, windowSize.width(),
                windowSize.height(), this);
        }

        Nullable<Length> minHeight = m_rareComputedStyleData.minHeight();
        if (minHeight.hasValue()) {
            m_rareComputedStyleData.ensureMinHeight()->changeToFixedIfNeeded(
                curFontSize, rootFontSize, font, windowSize.width(),
                windowSize.height(), this);
        }

        Nullable<Length> maxHeight = m_rareComputedStyleData.maxHeight();
        if (maxHeight.hasValue()) {
            m_rareComputedStyleData.ensureMaxHeight()->changeToFixedIfNeeded(
                curFontSize, rootFontSize, font, windowSize.width(),
                windowSize.height(), this);
        }

        Nullable<Length> verticalAlignLength =
            m_rareComputedStyleData.verticalAlignLength();
        if (verticalAlignLength.hasValue()) {
            m_rareComputedStyleData.ensureVerticalAlignLength()
                ->changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                        windowSize.width(), windowSize.height(),
                                        this);
        }

        OutlineData* outline = m_rareComputedStyleData.outline();
        if (outline) {
            outline->checkComputed(curFontSize, rootFontSize, font, windowSize,
                                   this);
        }

        BorderRadiusData* borderRadius = m_rareComputedStyleData.borderRadius();
        if (borderRadius) {
            borderRadius->m_topLeftHorizontal.changeToFixedIfNeeded(
                curFontSize, rootFontSize, font, windowSize.width(),
                windowSize.height(), this);
            borderRadius->m_topLeftVertical.changeToFixedIfNeeded(
                curFontSize, rootFontSize, font, windowSize.width(),
                windowSize.height(), this);
            borderRadius->m_topRightHorizontal.changeToFixedIfNeeded(
                curFontSize, rootFontSize, font, windowSize.width(),
                windowSize.height(), this);
            borderRadius->m_topRightVertical.changeToFixedIfNeeded(
                curFontSize, rootFontSize, font, windowSize.width(),
                windowSize.height(), this);
            borderRadius->m_bottomRightHorizontal.changeToFixedIfNeeded(
                curFontSize, rootFontSize, font, windowSize.width(),
                windowSize.height(), this);
            borderRadius->m_bottomRightVertical.changeToFixedIfNeeded(
                curFontSize, rootFontSize, font, windowSize.width(),
                windowSize.height(), this);
            borderRadius->m_bottomLeftHorizontal.changeToFixedIfNeeded(
                curFontSize, rootFontSize, font, windowSize.width(),
                windowSize.height(), this);
            borderRadius->m_bottomLeftVertical.changeToFixedIfNeeded(
                curFontSize, rootFontSize, font, windowSize.width(),
                windowSize.height(), this);
        }

        StyleTransformDataGroup* transforms =
            m_rareComputedStyleData.transforms();
        if (transforms) {
            size_t sz = transforms->size();
            for (size_t i = 0; i < sz; i++) {
                StyleTransformData& std = transforms->at(i);
                if (std.type() !=
                    StyleTransformData::OperationType::Translate) {
                    continue;
                }
                std.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                          windowSize, this);
            }
        }

        StyleTransformOrigin* origin =
            m_rareComputedStyleData.transformOrigin();
        if (origin && origin->originValue()) {
            origin->originValue()->getXAxis().changeToFixedIfNeeded(
                curFontSize, rootFontSize, font, windowSize.width(),
                windowSize.height(), this);
            origin->originValue()->getYAxis().changeToFixedIfNeeded(
                curFontSize, rootFontSize, font, windowSize.width(),
                windowSize.height(), this);
            origin->originValue()->getZAxis().changeToFixedIfNeeded(
                curFontSize, rootFontSize, font, windowSize.width(),
                windowSize.height(), this);
        }

        StyleBackgroundData* background = m_rareComputedStyleData.background();
        if (background) {
            background->checkComputed(curFontSize, rootFontSize, font,
                                      windowSize, this);
        }

        BorderData* border = m_rareComputedStyleData.border();
        if (border) {
            BorderData* b = border;
            b->checkComputed(curFontSize, rootFontSize, font, windowSize, this);
            if (!b->top().hasBorderColor()) {
                b->top().setColor(color());
            }
            if (!b->bottom().hasBorderColor()) {
                b->bottom().setColor(color());
            }
            if (!b->left().hasBorderColor()) {
                b->left().setColor(color());
            }
            if (!b->right().hasBorderColor()) {
                b->right().setColor(color());
            }
        }

        LengthData* padding = m_rareComputedStyleData.padding();
        if (padding) {
            padding->checkComputed(curFontSize, rootFontSize, font, windowSize,
                                   this);
        }

        LengthData* margin = m_rareComputedStyleData.margin();
        if (margin) {
            margin->checkComputed(curFontSize, rootFontSize, font, windowSize,
                                  this);
        }

        LengthData* offset = m_rareComputedStyleData.offset();
        if (offset) {
            offset->checkComputed(curFontSize, rootFontSize, font, windowSize,
                                  this);
        }

#define TO_FIXED(name, name2)                                           \
    Nullable<Length> name = m_rareComputedStyleData.name();             \
    if (name.hasValue()) {                                              \
        m_rareComputedStyleData.ensure##name2()->changeToFixedIfNeeded( \
            curFontSize, rootFontSize, font, windowSize.width(),        \
            windowSize.height(), this);                                 \
    }

        TO_FIXED(x, X);
        TO_FIXED(y, Y);
        TO_FIXED(cx, CX);
        TO_FIXED(cy, CY);
        TO_FIXED(rx, RX);
        TO_FIXED(ry, RY);
        TO_FIXED(gridRowGap, GridRowGap);
        TO_FIXED(gridColumnGap, GridColumnGap);

#undef TO_FIXED
    }

    if (textShadow()) {
        for (auto& shadow :
             m_inheritedStyles.m_rareData->m_textShadowDataList) {
            if (!shadow.offsetX().isComputed()) {
                auto v = shadow.offsetX();
                v.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                        windowSize.width(), windowSize.height(),
                                        this);
                shadow.setOffsetX(v);
            }
            if (!shadow.offsetY().isComputed()) {
                auto v = shadow.offsetY();
                v.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                        windowSize.width(), windowSize.height(),
                                        this);
                shadow.setOffsetY(v);
            }
            if (!shadow.radius().isComputed()) {
                auto v = shadow.radius();
                v.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                        windowSize.width(), windowSize.height(),
                                        this);
                shadow.setRadius(v);
            }
        }
    }

    if (boxShadow()) {
        for (auto& shadow : (*m_rareComputedStyleData.boxShadow())) {
            if (!shadow.offsetX().isComputed()) {
                auto v = shadow.offsetX();
                v.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                        windowSize.width(), windowSize.height(),
                                        this);
                shadow.setOffsetX(v);
            }
            if (!shadow.offsetY().isComputed()) {
                auto v = shadow.offsetY();
                v.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                        windowSize.width(), windowSize.height(),
                                        this);
                shadow.setOffsetY(v);
            }
            if (!shadow.radius().isComputed()) {
                auto v = shadow.radius();
                v.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                        windowSize.width(), windowSize.height(),
                                        this);
                shadow.setRadius(v);
            }
            if (!shadow.spreadDistance().isComputed()) {
                auto v = shadow.spreadDistance();
                v.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                        windowSize.width(), windowSize.height(),
                                        this);
                shadow.setSpreadDistance(v);
            }
        }
    }

    RectData* rect = m_rareComputedStyleData.clip();
    if (rect) {
        if (!rect->top().isComputed()) {
            rect->top().changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                              windowSize.width(),
                                              windowSize.height(), this);
        }

        if (!rect->right().isComputed()) {
            rect->right().changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                                windowSize.width(),
                                                windowSize.height(), this);
        }

        if (!rect->bottom().isComputed()) {
            rect->bottom().changeToFixedIfNeeded(curFontSize, rootFontSize,
                                                 font, windowSize.width(),
                                                 windowSize.height(), this);
        }

        if (!rect->left().isComputed()) {
            rect->left().changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                               windowSize.width(),
                                               windowSize.height(), this);
        }
    }

    GCVector<GridLength>* columns =
        m_rareComputedStyleData.gridTemplateColumns();

    if (columns) {
        for (size_t i = 0; i < columns->size(); i++) {
            if ((*columns)[i].isLength() &&
                !(*columns)[i].length().isComputed()) {
                (*columns)[i].mutableLength().changeToFixedIfNeeded(
                    curFontSize, rootFontSize, font, windowSize.width(),
                    windowSize.height(), this);
            }
        }
    }

    GCVector<GridLength>* rows = m_rareComputedStyleData.gridTemplateRows();

    if (rows) {
        for (size_t i = 0; i < rows->size(); i++) {
            if ((*rows)[i].isLength() && !(*rows)[i].length().isComputed()) {
                (*rows)[i].mutableLength().changeToFixedIfNeeded(
                    curFontSize, rootFontSize, font, windowSize.width(),
                    windowSize.height(), this);
            }
        }
    }
}

bool needsToApplyTransition(ComputedStyle* newStyle, const bool* damagedKeys)
{
    STARFISH_ASSERT(newStyle->display() != NoneDisplayValue);

    StyleTransitionData* data = newStyle->transition();
    for (size_t i = 0; i < data->size(); i++) {
        if (!data->duration(i).toTimeValue()) {
            continue;
        }
        CSSStyleValuePair::KeyKind property = data->property(i);
        bool isPropertyAll = property == CSSStyleValuePair::All;

        // e.g. BackgroundColor = BackgroundColor | Background | All
        RETURN_NEED_TRANSITION(CSSStyleValuePair::BackgroundColor,
                               CSSStyleValuePair::Background);
        RETURN_NEED_TRANSITION(CSSStyleValuePair::BackgroundPositionX,
                               CSSStyleValuePair::BackgroundPosition,
                               CSSStyleValuePair::Background);
        RETURN_NEED_TRANSITION(CSSStyleValuePair::BackgroundPositionY,
                               CSSStyleValuePair::BackgroundPosition,
                               CSSStyleValuePair::Background);
        RETURN_NEED_TRANSITION(CSSStyleValuePair::BackgroundSize,
                               CSSStyleValuePair::Background);
        RETURN_NEED_TRANSITION(CSSStyleValuePair::BorderBottomColor,
                               CSSStyleValuePair::BorderColor,
                               CSSStyleValuePair::BorderBottom);
        RETURN_NEED_TRANSITION(CSSStyleValuePair::BorderLeftColor,
                               CSSStyleValuePair::BorderColor,
                               CSSStyleValuePair::BorderLeft);
        RETURN_NEED_TRANSITION(CSSStyleValuePair::BorderTopColor,
                               CSSStyleValuePair::BorderColor,
                               CSSStyleValuePair::BorderTop);
        RETURN_NEED_TRANSITION(CSSStyleValuePair::Color);
        RETURN_NEED_TRANSITION(CSSStyleValuePair::CaretColor);
        RETURN_NEED_TRANSITION(CSSStyleValuePair::Height);
#ifndef NDEBUG
        RETURN_NEED_TRANSITION(CSSStyleValuePair::MarginBottom,
                               CSSStyleValuePair::Margin);
        RETURN_NEED_TRANSITION(CSSStyleValuePair::MarginLeft,
                               CSSStyleValuePair::Margin);
        RETURN_NEED_TRANSITION(CSSStyleValuePair::MarginRight,
                               CSSStyleValuePair::Margin);
        RETURN_NEED_TRANSITION(CSSStyleValuePair::MarginTop,
                               CSSStyleValuePair::Margin);
#endif
        RETURN_NEED_TRANSITION(CSSStyleValuePair::MaxHeight);
        RETURN_NEED_TRANSITION(CSSStyleValuePair::MaxWidth);
        RETURN_NEED_TRANSITION(CSSStyleValuePair::MinHeight);
        RETURN_NEED_TRANSITION(CSSStyleValuePair::MinWidth);
        RETURN_NEED_TRANSITION(CSSStyleValuePair::Opacity);
        RETURN_NEED_TRANSITION(CSSStyleValuePair::OutlineColor);
        RETURN_NEED_TRANSITION(CSSStyleValuePair::TextDecorationColor,
                               CSSStyleValuePair::TextDecoration);
        RETURN_NEED_TRANSITION(CSSStyleValuePair::Transform);
        RETURN_NEED_TRANSITION(CSSStyleValuePair::Width);
    }

    return false;
}

void applyTransition(Element* element, ComputedStyle* oldStyle, Frame* oldFrame,
                     ComputedStyle* newStyle, const bool* damagedKeys)
{
    STARFISH_ASSERT(newStyle->display() != NoneDisplayValue);
    STARFISH_ASSERT(oldFrame);

    AnimationExecutor* executor = element->document()->animationExecutor();
    StyleTransitionData* data = newStyle->transition();
    for (size_t i = 0; i < data->size(); i++) {
        if (!data->duration(i).toTimeValue()) {
            continue;
        }
        CSSStyleValuePair::KeyKind property = data->property(i);
        bool isPropertyAll = property == CSSStyleValuePair::All;

        auto duration = data->duration(i).toTimeValue();
        auto delay = data->delay(i).toTimeValue();
        auto timingFunction = data->timingFunction(i);

        if (NEED_TRANSITION(CSSStyleValuePair::BackgroundColor,
                            CSSStyleValuePair::Background)) {
            Unit::Color oldColor = oldStyle->backgroundColor();
            executor->registerAnimation(new ColorAnimationTask(
                element, CSSStyleValuePair::BackgroundColor,
                AnimatedValue(oldColor),
                AnimatedValue(newStyle->backgroundColor()), duration, delay,
                timingFunction));
            newStyle->setBackgroundColor(oldColor);
        }
        if (NEED_TRANSITION(CSSStyleValuePair::BackgroundPositionX,
                            CSSStyleValuePair::BackgroundPosition,
                            CSSStyleValuePair::Background)) {
            if (!oldStyle->hasBlockLikeDisplay() ||
                !newStyle->hasBlockLikeDisplay()) {
                // TODO Inline Element
                continue;
            }
            FrameBox* oldPaintingBox = oldFrame->asFrameBox();
            size_t layerSize = newStyle->backgroundLayerSize();
            GCVector<LengthAnimationTask*> tasks;
            for (size_t i = 0; i < layerSize; i++) {
                AnimatedValue pos1, pos2;
                if (AnimationUtil::backgroundPosXToAnimatedValue(
                        oldStyle, newStyle, oldPaintingBox, element, pos1, pos2,
                        i)) {
                    // NOTE Do not register animation directly here
                    // Because registerAnimation can effect next layer style
                    tasks.push_back(new LengthAnimationTask(
                        element, CSSStyleValuePair::BackgroundPositionX, pos1,
                        pos2, duration, delay, timingFunction, (void*)i));
                }
            }
            for (size_t i = 0; i < tasks.size(); i++) {
                executor->registerAnimation(tasks[i]);
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::BackgroundPositionY,
                            CSSStyleValuePair::BackgroundPosition,
                            CSSStyleValuePair::Background)) {
            if (!oldStyle->hasBlockLikeDisplay() ||
                !newStyle->hasBlockLikeDisplay()) {
                // TODO Inline Element
                continue;
            }
            FrameBox* oldPaintingBox = oldFrame->asFrameBox();
            size_t layerSize = newStyle->backgroundLayerSize();
            GCVector<LengthAnimationTask*> tasks;
            for (size_t i = 0; i < layerSize; i++) {
                AnimatedValue pos1, pos2;
                if (AnimationUtil::backgroundPosYToAnimatedValue(
                        oldStyle, newStyle, oldPaintingBox, element, pos1, pos2,
                        i)) {
                    // NOTE Do not register animation directly here
                    // Because registerAnimation can effect next layer style
                    tasks.push_back(new LengthAnimationTask(
                        element, CSSStyleValuePair::BackgroundPositionY, pos1,
                        pos2, duration, delay, timingFunction, (void*)i));
                }
            }
            for (size_t i = 0; i < tasks.size(); i++) {
                executor->registerAnimation(tasks[i]);
            }
        }
        // NOTE background-size should come after background-position
        if (NEED_TRANSITION(CSSStyleValuePair::BackgroundSize,
                            CSSStyleValuePair::Background)) {
            if (!oldStyle->hasBlockLikeDisplay() ||
                !newStyle->hasBlockLikeDisplay()) {
                // TODO Inline Element
                continue;
            }
            FrameBox* oldPaintingBox = oldFrame->asFrameBox();
            size_t layerSize = newStyle->backgroundLayerSize();
            GCVector<LengthSizeAnimationTask*> tasks;
            for (size_t i = 0; i < layerSize; i++) {
                AnimatedValue size1, size2;
                if (AnimationUtil::backgroundSizeToAnimatedValue(
                        oldStyle, newStyle, oldPaintingBox, element, size1,
                        size2, i)) {
                    // NOTE Do not register animation directly here
                    // Because registerAnimation can effect next layer style
                    tasks.push_back(new LengthSizeAnimationTask(
                        element, CSSStyleValuePair::BackgroundSize, size1,
                        size2, duration, delay, timingFunction, (void*)i));
                }
            }
            for (size_t i = 0; i < tasks.size(); i++) {
                executor->registerAnimation(tasks[i]);
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::BorderBottomColor,
                            CSSStyleValuePair::BorderColor,
                            CSSStyleValuePair::BorderBottom)) {
            Unit::Color oldColor = oldStyle->border().bottom().color();
            executor->registerAnimation(new ColorAnimationTask(
                element, CSSStyleValuePair::BorderBottomColor,
                AnimatedValue(oldColor),
                AnimatedValue(newStyle->border().bottom().color()), duration,
                delay, timingFunction));
            newStyle->setBorderBottomColor(oldColor);
        }
        if (NEED_TRANSITION(CSSStyleValuePair::BorderLeftColor,
                            CSSStyleValuePair::BorderColor,
                            CSSStyleValuePair::BorderLeft)) {
            Unit::Color oldColor = oldStyle->border().left().color();
            executor->registerAnimation(new ColorAnimationTask(
                element, CSSStyleValuePair::BorderLeftColor,
                AnimatedValue(oldColor),
                AnimatedValue(newStyle->border().left().color()), duration,
                delay, timingFunction));
            newStyle->setBorderLeftColor(oldColor);
        }
        if (NEED_TRANSITION(CSSStyleValuePair::BorderRightColor,
                            CSSStyleValuePair::BorderColor,
                            CSSStyleValuePair::BorderRight)) {
            Unit::Color oldColor = oldStyle->border().right().color();
            executor->registerAnimation(new ColorAnimationTask(
                element, CSSStyleValuePair::BorderRightColor,
                AnimatedValue(oldColor),
                AnimatedValue(newStyle->border().right().color()), duration,
                delay, timingFunction));
            newStyle->setBorderRightColor(oldColor);
        }
        if (NEED_TRANSITION(CSSStyleValuePair::BorderTopColor,
                            CSSStyleValuePair::BorderColor,
                            CSSStyleValuePair::BorderTop)) {
            Unit::Color oldColor = oldStyle->border().top().color();
            executor->registerAnimation(new ColorAnimationTask(
                element, CSSStyleValuePair::BorderTopColor,
                AnimatedValue(oldColor),
                AnimatedValue(newStyle->border().top().color()), duration,
                delay, timingFunction));
            newStyle->setBorderTopColor(oldColor);
        }
        if (NEED_TRANSITION(CSSStyleValuePair::Color)) {
            Unit::Color oldColor = oldStyle->color();
            executor->registerAnimation(new ColorAnimationTask(
                element, CSSStyleValuePair::Color, AnimatedValue(oldColor),
                AnimatedValue(newStyle->color()), duration, delay,
                timingFunction));
            newStyle->setColor(oldColor);
        }
        if (NEED_TRANSITION(CSSStyleValuePair::CaretColor)) {
            Unit::Color oldColor = oldStyle->caretColor();
            executor->registerAnimation(new ColorAnimationTask(
                element, CSSStyleValuePair::CaretColor, AnimatedValue(oldColor),
                AnimatedValue(newStyle->caretColor()), duration, delay,
                timingFunction));
            newStyle->setColor(oldColor);
        }
        if (NEED_TRANSITION(CSSStyleValuePair::Height)) {
            if (oldStyle->height().isDefinite(true) &&
                oldStyle->hasBlockLikeDisplay() &&
                newStyle->height().isDefinite(true) &&
                newStyle->hasBlockLikeDisplay()) {
                LayoutUnit currentHeight = oldFrame->asFrameBox()->height();
                if (oldStyle->boxSizing() ==
                    BoxSizingValue::ContentBoxBoxSizingValue) {
                    currentHeight = oldFrame->asFrameBox()->contentHeight();
                }
                executor->registerAnimation(
                    new LengthAnimationTask(element, CSSStyleValuePair::Height,
                                            AnimatedValue(currentHeight),
                                            AnimatedValue(newStyle->height()),
                                            duration, delay, timingFunction));
            }
        }
#ifndef NDEBUG
        if (NEED_TRANSITION(CSSStyleValuePair::MarginBottom,
                            CSSStyleValuePair::Margin)) {
            AnimatedValue v1, v2;
            if (AnimationUtil::marginBottomToAnimatedValue(oldStyle, newStyle,
                                                           element, v1, v2)) {
                executor->registerAnimation(new LengthAnimationTask(
                    element, CSSStyleValuePair::MarginBottom, v1, v2, duration,
                    delay, timingFunction));
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::MarginLeft,
                            CSSStyleValuePair::Margin)) {
            AnimatedValue v1, v2;
            if (AnimationUtil::marginLeftToAnimatedValue(oldStyle, newStyle,
                                                         element, v1, v2)) {
                executor->registerAnimation(new LengthAnimationTask(
                    element, CSSStyleValuePair::MarginLeft, v1, v2, duration,
                    delay, timingFunction));
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::MarginRight,
                            CSSStyleValuePair::Margin)) {
            AnimatedValue v1, v2;
            if (AnimationUtil::marginRightToAnimatedValue(oldStyle, newStyle,
                                                          element, v1, v2)) {
                executor->registerAnimation(new LengthAnimationTask(
                    element, CSSStyleValuePair::MarginRight, v1, v2, duration,
                    delay, timingFunction));
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::MarginTop,
                            CSSStyleValuePair::Margin)) {
            AnimatedValue v1, v2;
            if (AnimationUtil::marginTopToAnimatedValue(oldStyle, newStyle,
                                                        element, v1, v2)) {
                executor->registerAnimation(new LengthAnimationTask(
                    element, CSSStyleValuePair::MarginTop, v1, v2, duration,
                    delay, timingFunction));
            }
        }
#endif
        if (NEED_TRANSITION(CSSStyleValuePair::MaxHeight)) {
            if (oldStyle->maxHeight().isDefinite(true) &&
                oldStyle->hasBlockLikeDisplay() &&
                newStyle->maxHeight().isDefinite(true) &&
                newStyle->hasBlockLikeDisplay()) {
                FrameBox* cb = containingBlock(oldFrame);
                LayoutUnit currentValue = oldStyle->maxHeight().specifiedValue(
                    cb->contentHeight(), oldFrame);

                executor->registerAnimation(new LengthAnimationTask(
                    element, CSSStyleValuePair::MaxHeight,
                    AnimatedValue(currentValue),
                    AnimatedValue(newStyle->maxHeight()), duration, delay,
                    timingFunction));
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::MaxWidth)) {
            if (oldStyle->maxWidth().isDefinite(true) &&
                oldStyle->hasBlockLikeDisplay() &&
                newStyle->maxWidth().isDefinite(true) &&
                newStyle->hasBlockLikeDisplay()) {
                FrameBox* cb = containingBlock(oldFrame);
                LayoutUnit currentValue = oldStyle->maxWidth().specifiedValue(
                    cb->contentWidth(), oldFrame);

                executor->registerAnimation(new LengthAnimationTask(
                    element, CSSStyleValuePair::MaxWidth,
                    AnimatedValue(currentValue),
                    AnimatedValue(newStyle->maxWidth()), duration, delay,
                    timingFunction));
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::MinHeight)) {
            if (oldStyle->minHeight().isDefinite(true) &&
                oldStyle->hasBlockLikeDisplay() &&
                newStyle->minHeight().isDefinite(true) &&
                newStyle->hasBlockLikeDisplay()) {
                FrameBox* cb = containingBlock(oldFrame);
                LayoutUnit currentValue = oldStyle->minHeight().specifiedValue(
                    cb->contentHeight(), oldFrame);

                executor->registerAnimation(new LengthAnimationTask(
                    element, CSSStyleValuePair::MinHeight,
                    AnimatedValue(currentValue),
                    AnimatedValue(newStyle->minHeight()), duration, delay,
                    timingFunction));
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::MinWidth)) {
            if (oldStyle->minWidth().isDefinite(true) &&
                oldStyle->hasBlockLikeDisplay() &&
                newStyle->minWidth().isDefinite(true) &&
                newStyle->hasBlockLikeDisplay()) {
                FrameBox* cb = containingBlock(oldFrame);
                LayoutUnit currentValue = oldStyle->minWidth().specifiedValue(
                    cb->contentWidth(), oldFrame);

                executor->registerAnimation(new LengthAnimationTask(
                    element, CSSStyleValuePair::MinWidth,
                    AnimatedValue(currentValue),
                    AnimatedValue(newStyle->minWidth()), duration, delay,
                    timingFunction));
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::Opacity)) {
            executor->registerAnimation(new OpacityAnimationTask(
                element, AnimatedValue(oldStyle->opacity()),
                AnimatedValue(newStyle->opacity()), duration, delay,
                timingFunction));
            newStyle->setOpacity(oldStyle->opacity());
        }
        if (NEED_TRANSITION(CSSStyleValuePair::OutlineColor)) {
            Unit::Color oldColor = oldStyle->outlineColor();
            executor->registerAnimation(
                new ColorAnimationTask(element, CSSStyleValuePair::OutlineColor,
                                       AnimatedValue(oldColor),
                                       AnimatedValue(newStyle->outlineColor()),
                                       duration, delay, timingFunction));
            newStyle->setOutlineColor(oldColor);
        }
        if (NEED_TRANSITION(CSSStyleValuePair::TextDecorationColor,
                            CSSStyleValuePair::TextDecoration)) {
            Unit::Color oldColor = oldStyle->textDecorationColor();
            executor->registerAnimation(new ColorAnimationTask(
                element, CSSStyleValuePair::TextDecorationColor,
                AnimatedValue(oldColor),
                AnimatedValue(newStyle->textDecorationColor()), duration, delay,
                timingFunction));
            newStyle->setTextDecorationColor(oldColor);
        }
        if (NEED_TRANSITION(CSSStyleValuePair::Transform)) {
            if (oldFrame->isTransformable()) {
                STARFISH_ASSERT(oldFrame->isFrameBox());
                FrameBox* box = oldFrame->asFrameBox();
                SkMatrix matrixBefore = oldStyle->transformsToMatrix(
                    box->width(), box->height(), box, true);

                TransformAnimationTask* task = new TransformAnimationTask(
                    element, AnimatedValue(matrixBefore), duration, delay,
                    timingFunction);
                executor->registerAnimation(task);
            }
        }
        if (NEED_TRANSITION(CSSStyleValuePair::Width)) {
            if (oldStyle->width().isDefinite(true) &&
                oldStyle->hasBlockLikeDisplay() &&
                newStyle->width().isDefinite(true) &&
                newStyle->hasBlockLikeDisplay()) {
                LayoutUnit currentWidth = oldFrame->asFrameBox()->width();
                if (oldStyle->boxSizing() ==
                    BoxSizingValue::ContentBoxBoxSizingValue) {
                    currentWidth = oldFrame->asFrameBox()->contentWidth();
                }
                executor->registerAnimation(
                    new LengthAnimationTask(element, CSSStyleValuePair::Width,
                                            AnimatedValue(currentWidth),
                                            AnimatedValue(newStyle->width()),
                                            duration, delay, timingFunction));
            }
        }
    }
}

ComputedStyleDamage compareStyle(ComputedStyle* oldStyle,
                                 ComputedStyle* newStyle, bool* damagedKeys)
{
    ComputedStyleDamage damage = ComputedStyleDamage::ComputedStyleDamageNone;
    if (newStyle->m_inheritedStyles.m_color !=
        oldStyle->m_inheritedStyles.m_color) {
        damagedKeys[CSSStyleValuePair::KeyKind::Color] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->m_inheritedStyles.m_direction !=
        oldStyle->m_inheritedStyles.m_direction) {
        damagedKeys[CSSStyleValuePair::KeyKind::Direction] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
    }

    if (newStyle->m_inheritedStyles.m_whiteSpace !=
        oldStyle->m_inheritedStyles.m_whiteSpace) {
        damagedKeys[CSSStyleValuePair::KeyKind::WhiteSpace] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->fontSize() != oldStyle->fontSize()) {
        damagedKeys[CSSStyleValuePair::KeyKind::FontSize] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_inheritedStyles.m_fontStyle !=
        oldStyle->m_inheritedStyles.m_fontStyle) {
        damagedKeys[CSSStyleValuePair::KeyKind::FontStyle] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_inheritedStyles.m_fontFamilyDatas[0].m_length !=
        oldStyle->m_inheritedStyles.m_fontFamilyDatas[0].m_length) {
        damagedKeys[CSSStyleValuePair::KeyKind::FontFamily] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    } else {
        size_t len = newStyle->m_inheritedStyles.m_fontFamilyDatas[0].m_length;
        for (size_t i = 0; i < len; i++) {
            if (!newStyle->m_inheritedStyles.m_fontFamilyDatas[i + 1]
                     .m_familyName->equals(
                         oldStyle->m_inheritedStyles.m_fontFamilyDatas[i + 1]
                             .m_familyName)) {
                damagedKeys[CSSStyleValuePair::KeyKind::FontFamily] = true;
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamageInherited |
                    ComputedStyleDamage::ComputedStyleDamageLayout | damage);
                break;
            }
        }
    }

    if (newStyle->m_inheritedStyles.m_fontWeight !=
        oldStyle->m_inheritedStyles.m_fontWeight) {
        damagedKeys[CSSStyleValuePair::KeyKind::FontWeight] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->fontKerning() != oldStyle->fontKerning()) {
        damagedKeys[CSSStyleValuePair::KeyKind::FontKerning] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_inheritedStyles.m_wordWrap !=
        oldStyle->m_inheritedStyles.m_wordWrap) {
        damagedKeys[CSSStyleValuePair::KeyKind::WordWrap] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->letterSpacing() != oldStyle->letterSpacing()) {
        damagedKeys[CSSStyleValuePair::KeyKind::LetterSpacing] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->lineHeight() != oldStyle->lineHeight()) {
        damagedKeys[CSSStyleValuePair::KeyKind::LineHeight] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->imageRendering() != oldStyle->imageRendering()) {
        damagedKeys[CSSStyleValuePair::KeyKind::ImageRendering] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->textIndent() != oldStyle->textIndent()) {
        damagedKeys[CSSStyleValuePair::KeyKind::TextIndent] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->textTransform() != oldStyle->textTransform()) {
        damagedKeys[CSSStyleValuePair::KeyKind::TextTransform] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->textOverflow() != oldStyle->textOverflow()) {
        damagedKeys[CSSStyleValuePair::KeyKind::TextOverflow] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->m_inheritedStyles.m_textAlign !=
        oldStyle->m_inheritedStyles.m_textAlign) {
        damagedKeys[CSSStyleValuePair::KeyKind::TextAlign] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_inheritedStyles.m_visibility !=
        oldStyle->m_inheritedStyles.m_visibility) {
        damagedKeys[CSSStyleValuePair::KeyKind::Visibility] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);

        if (newStyle->m_inheritedStyles.m_visibility ==
                VisibilityValue::CollapseVisibilityValue ||
            oldStyle->m_inheritedStyles.m_visibility ==
                VisibilityValue::CollapseVisibilityValue) {
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamageLayout | damage);
        }
    }

    if (newStyle->m_inheritedStyles.m_borderCollapse !=
        oldStyle->m_inheritedStyles.m_borderCollapse) {
        damagedKeys[CSSStyleValuePair::KeyKind::BorderCollapse] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->horizontalBorderSpacing() !=
        oldStyle->horizontalBorderSpacing()) {
        damagedKeys[CSSStyleValuePair::KeyKind::BorderSpacing] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->verticalBorderSpacing() !=
        oldStyle->verticalBorderSpacing()) {
        damagedKeys[CSSStyleValuePair::KeyKind::BorderSpacing] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_inheritedStyles.m_captionSide !=
        oldStyle->m_inheritedStyles.m_captionSide) {
        damagedKeys[CSSStyleValuePair::KeyKind::CaptionSide] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_inheritedStyles.m_emptyCells !=
        oldStyle->m_inheritedStyles.m_emptyCells) {
        damagedKeys[CSSStyleValuePair::KeyKind::EmptyCells] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->fill() != oldStyle->fill()) {
        damagedKeys[CSSStyleValuePair::KeyKind::Fill] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->fillRule() != oldStyle->fillRule()) {
        damagedKeys[CSSStyleValuePair::KeyKind::FillRule] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->fillOpacity() != oldStyle->fillOpacity()) {
        damagedKeys[CSSStyleValuePair::KeyKind::FillOpacity] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->stroke() != oldStyle->stroke()) {
        damagedKeys[CSSStyleValuePair::KeyKind::Stroke] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->strokeWidth() != oldStyle->strokeWidth()) {
        damagedKeys[CSSStyleValuePair::KeyKind::StrokeWidth] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->m_originalDisplay != oldStyle->m_originalDisplay) {
        damagedKeys[CSSStyleValuePair::KeyKind::Display] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
    }

    if (newStyle->m_position != oldStyle->m_position) {
        damagedKeys[CSSStyleValuePair::KeyKind::Position] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
    }

    if (newStyle->m_float != oldStyle->m_float) {
        damagedKeys[CSSStyleValuePair::KeyKind::Float] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
    }

    if (newStyle->m_clear != oldStyle->m_clear) {
        damagedKeys[CSSStyleValuePair::KeyKind::Clear] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->width() != oldStyle->width()) {
        damagedKeys[CSSStyleValuePair::KeyKind::Width] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->minWidth() != oldStyle->minWidth()) {
        damagedKeys[CSSStyleValuePair::KeyKind::MinWidth] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->maxWidth() != oldStyle->maxWidth()) {
        damagedKeys[CSSStyleValuePair::KeyKind::MaxWidth] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->height() != oldStyle->height()) {
        damagedKeys[CSSStyleValuePair::KeyKind::Height] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->minHeight() != oldStyle->minHeight()) {
        damagedKeys[CSSStyleValuePair::KeyKind::MinHeight] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->maxHeight() != oldStyle->maxHeight()) {
        damagedKeys[CSSStyleValuePair::KeyKind::MaxHeight] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (LengthData::damaged(
            oldStyle->margin(), newStyle->margin(),
            damagedKeys[CSSStyleValuePair::KeyKind::MarginTop],
            damagedKeys[CSSStyleValuePair::KeyKind::MarginRight],
            damagedKeys[CSSStyleValuePair::KeyKind::MarginBottom],
            damagedKeys[CSSStyleValuePair::KeyKind::MarginLeft])) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (LengthData::damaged(
            oldStyle->padding(), newStyle->padding(),
            damagedKeys[CSSStyleValuePair::KeyKind::PaddingTop],
            damagedKeys[CSSStyleValuePair::KeyKind::PaddingRight],
            damagedKeys[CSSStyleValuePair::KeyKind::PaddingBottom],
            damagedKeys[CSSStyleValuePair::KeyKind::PaddingLeft])) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (LengthData::damaged(oldStyle->offset(), newStyle->offset(),
                            damagedKeys[CSSStyleValuePair::KeyKind::Top],
                            damagedKeys[CSSStyleValuePair::KeyKind::Right],
                            damagedKeys[CSSStyleValuePair::KeyKind::Bottom],
                            damagedKeys[CSSStyleValuePair::KeyKind::Left])) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    {
        BorderData oldBorder = oldStyle->border();
        BorderData newBorder = newStyle->border();
        BorderValue& oldBorderTop = oldBorder.top();
        BorderValue& oldBorderRight = oldBorder.right();
        BorderValue& oldBorderBottom = oldBorder.bottom();
        BorderValue& oldBorderLeft = oldBorder.left();
        BorderValue& newBorderTop = newBorder.top();
        BorderValue& newBorderRight = newBorder.right();
        BorderValue& newBorderBottom = newBorder.bottom();
        BorderValue& newBorderLeft = newBorder.left();
        bool borderDamage = false;
        if (oldBorderTop.hasBorderColor() != newBorderTop.hasBorderColor() ||
            oldBorderTop.color() != newBorderTop.color()) {
            damagedKeys[CSSStyleValuePair::KeyKind::BorderTopColor] = true;
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamagePainting | damage);
        }
        if (oldBorderTop.width() != newBorderTop.width()) {
            damagedKeys[CSSStyleValuePair::KeyKind::BorderTopWidth] = true;
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamageLayout | damage);
        }
        if (oldBorderTop.style() != newBorderTop.style()) {
            damagedKeys[CSSStyleValuePair::KeyKind::BorderTopStyle] = true;
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamageLayout | damage);
        }
        if (oldBorderRight.hasBorderColor() !=
                newBorderRight.hasBorderColor() ||
            oldBorderRight.color() != newBorderRight.color()) {
            damagedKeys[CSSStyleValuePair::KeyKind::BorderRightColor] = true;
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamagePainting | damage);
        }
        if (oldBorderRight.width() != newBorderRight.width()) {
            damagedKeys[CSSStyleValuePair::KeyKind::BorderRightWidth] = true;
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamageLayout | damage);
        }
        if (oldBorderRight.style() != newBorderRight.style()) {
            damagedKeys[CSSStyleValuePair::KeyKind::BorderRightStyle] = true;
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamageLayout | damage);
        }
        if (oldBorderBottom.hasBorderColor() !=
                newBorderBottom.hasBorderColor() ||
            oldBorderBottom.color() != newBorderBottom.color()) {
            damagedKeys[CSSStyleValuePair::KeyKind::BorderBottomColor] = true;
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamagePainting | damage);
        }
        if (oldBorderBottom.width() != newBorderBottom.width()) {
            damagedKeys[CSSStyleValuePair::KeyKind::BorderBottomWidth] = true;
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamageLayout | damage);
        }
        if (oldBorderBottom.style() != newBorderBottom.style()) {
            damagedKeys[CSSStyleValuePair::KeyKind::BorderBottomStyle] = true;
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamageLayout | damage);
        }
        if (oldBorderLeft.hasBorderColor() != newBorderLeft.hasBorderColor() ||
            oldBorderLeft.color() != newBorderLeft.color()) {
            damagedKeys[CSSStyleValuePair::KeyKind::BorderLeftColor] = true;
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamagePainting | damage);
        }
        if (oldBorderLeft.width() != newBorderLeft.width()) {
            damagedKeys[CSSStyleValuePair::KeyKind::BorderLeftWidth] = true;
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamageLayout | damage);
        }
        if (oldBorderLeft.style() != newBorderLeft.style()) {
            damagedKeys[CSSStyleValuePair::KeyKind::BorderLeftStyle] = true;
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamageLayout | damage);
        }
    }

    if (newStyle->border().image().url() != oldStyle->border().image().url()) {
        damagedKeys[CSSStyleValuePair::KeyKind::BorderImageSource] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }
    if (newStyle->border().image().slices() !=
            oldStyle->border().image().slices() ||
        newStyle->border().image().sliceFill() !=
            oldStyle->border().image().sliceFill()) {
        damagedKeys[CSSStyleValuePair::KeyKind::BorderImageSlice] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }
    if (newStyle->border().image().widths() !=
        oldStyle->border().image().widths()) {
        damagedKeys[CSSStyleValuePair::KeyKind::BorderImageWidth] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }
    if (newStyle->border().image().outsets() !=
        oldStyle->border().image().outsets()) {
        damagedKeys[CSSStyleValuePair::KeyKind::BorderImageOutset] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }
    if (newStyle->border().image().repeatX() !=
            oldStyle->border().image().repeatX() ||
        newStyle->border().image().repeatY() !=
            oldStyle->border().image().repeatY()) {
        damagedKeys[CSSStyleValuePair::KeyKind::BorderImageRepeat] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->m_unicodeBidi != oldStyle->m_unicodeBidi) {
        damagedKeys[CSSStyleValuePair::KeyKind::UnicodeBidi] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_verticalAlign != oldStyle->m_verticalAlign) {
        damagedKeys[CSSStyleValuePair::KeyKind::VerticalAlign] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->verticalAlignLength() != oldStyle->verticalAlignLength()) {
        damagedKeys[CSSStyleValuePair::KeyKind::VerticalAlign] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_overflowX != oldStyle->m_overflowX &&
        newStyle->m_overflowY != oldStyle->m_overflowY) {
        damagedKeys[CSSStyleValuePair::KeyKind::OverflowX] = true;
        damagedKeys[CSSStyleValuePair::KeyKind::OverflowY] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    } else if (newStyle->m_overflowX != oldStyle->m_overflowX) {
        if (oldStyle->m_overflowX == OverflowValue::AutoOverflow &&
            newStyle->m_overflowX == OverflowValue::VisibleOverflow &&
            oldStyle->m_overflowY != OverflowValue::VisibleOverflow) {
        } else {
            damagedKeys[CSSStyleValuePair::KeyKind::OverflowX] = true;
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamageLayout | damage);
        }
    } else if (newStyle->m_overflowY != oldStyle->m_overflowY) {
        if (oldStyle->m_overflowY == OverflowValue::AutoOverflow &&
            newStyle->m_overflowY == OverflowValue::VisibleOverflow &&
            oldStyle->m_overflowX != OverflowValue::VisibleOverflow) {
        } else {
            damagedKeys[CSSStyleValuePair::KeyKind::OverflowY] = true;
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamageLayout | damage);
        }
    }

    if (newStyle->m_tableLayout != oldStyle->m_tableLayout) {
        damagedKeys[CSSStyleValuePair::KeyKind::TableLayout] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    // NOTE.
    // text-decoration is not inherited.
    // but it influence its child boxes, within it's inline formatting context
    // and is further propagated to any in-flow block-level boxes that split the
    // inline (see section 9.2.1.1).
    // https://www.w3.org/TR/CSS2/text.html#propdef-text-decoration
    if (newStyle->textDecorationColor() != oldStyle->textDecorationColor()) {
        damagedKeys[CSSStyleValuePair::KeyKind::TextDecorationColor] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    ValueList* oldTextDecorationLine = oldStyle->textDecorationLine();
    ValueList* newTextDecorationLine = newStyle->textDecorationLine();
    if (oldTextDecorationLine && oldTextDecorationLine->size() == 0) {
        oldTextDecorationLine = nullptr;
    }
    if (newTextDecorationLine && newTextDecorationLine->size() == 0) {
        newTextDecorationLine = nullptr;
    }

    if (oldTextDecorationLine == nullptr && newTextDecorationLine == nullptr) {
    } else if ((oldTextDecorationLine != nullptr &&
                newTextDecorationLine == nullptr) ||
               (oldTextDecorationLine == nullptr &&
                newTextDecorationLine != nullptr)) {
        damagedKeys[CSSStyleValuePair::KeyKind::TextDecorationLine] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    } else if (!oldTextDecorationLine->equalsTextDecorationLine(
                   newTextDecorationLine)) {
        damagedKeys[CSSStyleValuePair::KeyKind::TextDecorationLine] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->textDecorationStyle() != oldStyle->textDecorationStyle()) {
        damagedKeys[CSSStyleValuePair::KeyKind::TextDecorationStyle] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->textUnderlinePosition() !=
        oldStyle->textUnderlinePosition()) {
        damagedKeys[CSSStyleValuePair::KeyKind::TextUnderlinePosition] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->resize() != oldStyle->resize()) {
        damagedKeys[CSSStyleValuePair::KeyKind::Resize] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    float newOpacity = newStyle->opacity();
    float oldOpacity = oldStyle->opacity();

    if (newOpacity != oldOpacity) {
        damagedKeys[CSSStyleValuePair::KeyKind::Opacity] = true;
        if (newOpacity < 1 && oldOpacity < 1) {
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamagePainting | damage);
        } else {
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
        }
    }

    // FIXME changing z-index not always cause tree-rebuild
    if (newStyle->zIndex() != oldStyle->zIndex()) {
        damagedKeys[CSSStyleValuePair::KeyKind::ZIndex] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
    }

    if (newStyle->backgroundColor() != oldStyle->backgroundColor()) {
        damagedKeys[CSSStyleValuePair::KeyKind::BackgroundColor] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (StyleBackgroundData::damaged(
            oldStyle->background(), newStyle->background(),
            damagedKeys[CSSStyleValuePair::KeyKind::BackgroundAttachment],
            damagedKeys[CSSStyleValuePair::KeyKind::BackgroundClip],
            damagedKeys[CSSStyleValuePair::KeyKind::BackgroundImage],
            damagedKeys[CSSStyleValuePair::KeyKind::BackgroundOrigin],
            damagedKeys[CSSStyleValuePair::KeyKind::BackgroundSize],
            damagedKeys[CSSStyleValuePair::KeyKind::BackgroundRepeatX],
            damagedKeys[CSSStyleValuePair::KeyKind::BackgroundRepeatY],
            damagedKeys[CSSStyleValuePair::KeyKind::BackgroundPositionX],
            damagedKeys[CSSStyleValuePair::KeyKind::BackgroundPositionY])) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    // TODO
    if (/*compare transform-3d is not same*/ false) {
        // damage = (ComputedStyleDamage)(
        //     ComputedStyleDamage::ComputedStyleDamageComputeStackingContextProperties
        //     | damage);
    }

    // TODO
    if (/*compare transform-3d perspective is not same*/ false) {
        // damage = (ComputedStyleDamage)(
        //     ComputedStyleDamage::ComputedStyleDamageComputeStackingContextProperties
        //     | damage);
    }

    StyleTransformDataGroup* oldTransforms =
        oldStyle->hasRareComputeStyleData()
            ? oldStyle->rareComputedStyleData()->transforms()
            : nullptr;
    if (oldTransforms && oldTransforms->size() == 0) {
        oldTransforms = nullptr;
    }
    StyleTransformDataGroup* newTransforms =
        newStyle->hasRareComputeStyleData()
            ? newStyle->rareComputedStyleData()->transforms()
            : nullptr;
    if (newTransforms && newTransforms->size() == 0) {
        newTransforms = nullptr;
    }
    bool oldComplex =
        oldTransforms ? oldTransforms->hasComplexTransform() : false;
    bool newComplex =
        newTransforms ? newTransforms->hasComplexTransform() : false;
    if (oldComplex != newComplex) {
        damagedKeys[CSSStyleValuePair::KeyKind::Transform] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::
                ComputedStyleDamageComputeStackingContextProperties |
            damage);
    }

    if (newTransforms == nullptr && oldTransforms == nullptr) {
    } else if (newTransforms == nullptr || oldTransforms == nullptr) {
        damagedKeys[CSSStyleValuePair::KeyKind::Transform] = true;
        // FIXME if element has transform, we should re-layout for building
        // stacking-context
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::
                ComputedStyleDamageComputeStackingContextProperties |
            damage);
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    } else {
        if (*newTransforms != *oldTransforms) {
            damagedKeys[CSSStyleValuePair::KeyKind::Transform] = true;
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::
                    ComputedStyleDamageComputeStackingContextProperties |
                damage);
        }
    }

    StyleTransformOrigin* oldTransformOrigin =
        oldStyle->hasRareComputeStyleData()
            ? oldStyle->rareComputedStyleData()->transformOrigin()
            : nullptr;
    StyleTransformOrigin* newTransformOrigin =
        newStyle->hasRareComputeStyleData()
            ? newStyle->rareComputedStyleData()->transformOrigin()
            : nullptr;

    if (newTransformOrigin == nullptr && oldTransformOrigin == nullptr) {
    } else if (newTransformOrigin == nullptr || oldTransformOrigin == nullptr) {
        damagedKeys[CSSStyleValuePair::KeyKind::TransformOrigin] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::
                ComputedStyleDamageComputeStackingContextProperties |
            damage);
    } else {
        if (*newTransformOrigin != *oldTransformOrigin) {
            damagedKeys[CSSStyleValuePair::KeyKind::TransformOrigin] = true;
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::
                    ComputedStyleDamageComputeStackingContextProperties |
                damage);
        }
    }

    if (newStyle->m_boxSizing != oldStyle->m_boxSizing) {
        damagedKeys[CSSStyleValuePair::KeyKind::BoxSizing] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_flexDirection != oldStyle->m_flexDirection) {
        damagedKeys[CSSStyleValuePair::KeyKind::FlexDirection] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_flexWrap != oldStyle->m_flexWrap) {
        damagedKeys[CSSStyleValuePair::KeyKind::FlexWrap] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->order() != oldStyle->order()) {
        damagedKeys[CSSStyleValuePair::KeyKind::Order] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_justifyContent != oldStyle->m_justifyContent) {
        damagedKeys[CSSStyleValuePair::KeyKind::JustifyContent] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_alignItems != oldStyle->m_alignItems) {
        damagedKeys[CSSStyleValuePair::KeyKind::AlignItems] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_alignSelf != oldStyle->m_alignSelf) {
        damagedKeys[CSSStyleValuePair::KeyKind::AlignSelf] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_alignContent != oldStyle->m_alignContent) {
        damagedKeys[CSSStyleValuePair::KeyKind::AlignContent] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->flexGrow() != oldStyle->flexGrow()) {
        damagedKeys[CSSStyleValuePair::KeyKind::FlexGrow] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->flexShrink() != oldStyle->flexShrink()) {
        damagedKeys[CSSStyleValuePair::KeyKind::FlexShrink] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->flexBasis() != oldStyle->flexBasis()) {
        damagedKeys[CSSStyleValuePair::KeyKind::FlexBasis] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    // The style for the 'content' property is computed when we build the frame
    // tree if it is needed.

    ContentDataGroup* oldContent =
        oldStyle->hasRareComputeStyleData()
            ? oldStyle->rareComputedStyleData()->content()
            : nullptr;
    ContentDataGroup* newContent =
        newStyle->hasRareComputeStyleData()
            ? newStyle->rareComputedStyleData()->content()
            : nullptr;

    if (newContent == nullptr && oldContent == nullptr) {
    } else if (newContent == nullptr || oldContent == nullptr) {
        damagedKeys[CSSStyleValuePair::KeyKind::Content] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
    } else {
        if (*oldContent != *newContent) {
            damagedKeys[CSSStyleValuePair::KeyKind::Content] = true;
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
        }
    }

    CounterBaseList* newCReset = newStyle->counterReset();
    CounterBaseList* oldCReset = oldStyle->counterReset();
    if (newCReset != oldCReset &&
        (!newCReset || !oldCReset || *newCReset != *oldCReset)) {
        damagedKeys[CSSStyleValuePair::CounterReset] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
    }

    CounterBaseList* newCInc = newStyle->counterIncrement();
    CounterBaseList* oldCInc = oldStyle->counterIncrement();
    if (newCInc != oldCInc && (!newCInc || !oldCInc || *newCInc != *oldCInc)) {
        damagedKeys[CSSStyleValuePair::CounterIncrement] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
    }

    if (newStyle->outlineColor() != oldStyle->outlineColor()) {
        damagedKeys[CSSStyleValuePair::KeyKind::OutlineColor] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->outlineStyle() != oldStyle->outlineStyle()) {
        damagedKeys[CSSStyleValuePair::KeyKind::OutlineStyle] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->outlineWidth() != oldStyle->outlineWidth()) {
        damagedKeys[CSSStyleValuePair::KeyKind::OutlineWidth] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->outlineOffset() != oldStyle->outlineOffset()) {
        damagedKeys[CSSStyleValuePair::KeyKind::OutlineOffset] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->borderRadius() != oldStyle->borderRadius()) {
        // TODO seprate this
        damagedKeys[CSSStyleValuePair::KeyKind::BorderTopLeftRadius] = true;
        damagedKeys[CSSStyleValuePair::KeyKind::BorderTopRightRadius] = true;
        damagedKeys[CSSStyleValuePair::KeyKind::BorderBottomLeftRadius] = true;
        damagedKeys[CSSStyleValuePair::KeyKind::BorderBottomRightRadius] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    ShadowDataList* oldShadow = oldStyle->textShadow();
    ShadowDataList* newShadow = newStyle->textShadow();

    if (oldShadow == nullptr && newShadow == nullptr) {
    } else if ((oldShadow != nullptr && newShadow == nullptr) ||
               (oldShadow == nullptr && newShadow != nullptr)) {
        damagedKeys[CSSStyleValuePair::KeyKind::TextShadow] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    } else if (*oldShadow != *newShadow) {
        damagedKeys[CSSStyleValuePair::KeyKind::TextShadow] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    oldShadow = oldStyle->boxShadow();
    newShadow = newStyle->boxShadow();

    if (oldShadow == nullptr && newShadow == nullptr) {
    } else if ((oldShadow != nullptr && newShadow == nullptr) ||
               (oldShadow == nullptr && newShadow != nullptr)) {
        damagedKeys[CSSStyleValuePair::KeyKind::BoxShadow] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    } else if (*oldShadow != *newShadow) {
        damagedKeys[CSSStyleValuePair::KeyKind::BoxShadow] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->listStyleData() != oldStyle->listStyleData()) {
        damagedKeys[CSSStyleValuePair::KeyKind::ListStyleType] = true;
        damagedKeys[CSSStyleValuePair::KeyKind::ListStyleImage] = true;
        damagedKeys[CSSStyleValuePair::KeyKind::ListStylePosition] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
    }

    auto oldClip = oldStyle->clip();
    auto newClip = newStyle->clip();
    if (oldClip == nullptr && newClip == nullptr) {
    } else if (oldClip == nullptr || newClip == nullptr) {
        damagedKeys[CSSStyleValuePair::KeyKind::Clip] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    } else if (*newStyle->clip() != *oldStyle->clip()) {
        damagedKeys[CSSStyleValuePair::KeyKind::Clip] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->userSelect() != oldStyle->userSelect()) {
        damagedKeys[CSSStyleValuePair::KeyKind::UserSelect] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->gridTemplateColumns() != oldStyle->gridTemplateColumns()) {
        damagedKeys[CSSStyleValuePair::KeyKind::GridTemplateColumns] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->gridTemplateRows() != oldStyle->gridTemplateRows()) {
        damagedKeys[CSSStyleValuePair::KeyKind::GridTemplateRows] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->caretColor() != oldStyle->caretColor()) {
        damagedKeys[CSSStyleValuePair::KeyKind::CaretColor] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->hyphens() != oldStyle->hyphens()) {
        damagedKeys[CSSStyleValuePair::KeyKind::Hyphens] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->lineBreak() != oldStyle->lineBreak()) {
        damagedKeys[CSSStyleValuePair::KeyKind::LineBreak] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->wordBreak() != oldStyle->wordBreak()) {
        damagedKeys[CSSStyleValuePair::KeyKind::WordBreak] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->gridRowGap() != oldStyle->gridRowGap()) {
        damagedKeys[CSSStyleValuePair::KeyKind::GridRowGap] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->gridColumnGap() != oldStyle->gridColumnGap()) {
        damagedKeys[CSSStyleValuePair::KeyKind::GridColumnGap] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->gridTemplateAreas() != oldStyle->gridTemplateAreas()) {
        damagedKeys[CSSStyleValuePair::KeyKind::GridTemplateAreas] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->boxDecorationBreak() != oldStyle->boxDecorationBreak()) {
        damagedKeys[CSSStyleValuePair::KeyKind::BoxDecorationBreak] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
    }

    return damage;
}

inline double deg2rad(float degree)
{
    return degree * M_PI / 180;
}

SkMatrix ComputedStyle::transformsToMatrix(LayoutUnit containerWidth,
                                           LayoutUnit containerHeight, Frame* f,
                                           bool isTransformable)
{
    SkMatrix matrix;
    matrix.reset();

    if (!hasRareComputeStyleData()) {
        return matrix;
    }

    StyleTransformDataGroup* transforms = m_rareComputedStyleData.transforms();

    if (!transforms || !isTransformable) {
        return matrix;
    }

    for (size_t i = 0; i < transforms->size(); i++) {
        StyleTransformData t = transforms->at(i);
        if (t.type() == StyleTransformData::Matrix) {
            MatrixTransform* m = t.matrix();
            // [ a c e ]
            // [ b d f ]
            // [ x x x ]
            matrix.set(0, m->a());
            matrix.set(1, m->c());
            matrix.set(2, m->e());
            matrix.set(3, m->b());
            matrix.set(4, m->d());
            matrix.set(5, m->f());
        } else if (t.type() == StyleTransformData::InternalMatrix) {
            matrix = t.internalMatrix()->matrix();
        } else if (t.type() == StyleTransformData::Scale) {
            ScaleTransform* m = t.scale();
            matrix.preScale(m->x(), m->y());
        } else if (t.type() == StyleTransformData::Rotate) {
            RotateTransform* m = t.rotate();
            matrix.preRotate(m->angle());
        } else if (t.type() == StyleTransformData::Skew) {
            SkewTransform* m = t.skew();
            matrix.preSkew(tan(deg2rad(m->angleX())),
                           tan(deg2rad(m->angleY())));
        } else if (t.type() == StyleTransformData::Translate) {
            TranslateTransform* m = t.translate();
            matrix.preTranslate(m->tx().specifiedValue(containerWidth, f),
                                m->ty().specifiedValue(containerHeight, f));
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }
    // printf("[%8.4f %8.4f %8.4f][%8.4f %8.4f %8.4f][%8.4f %8.4f %8.4f]\n",
    //         matrix.get(0), matrix.get(1), matrix.get(2), matrix.get(3),
    //         matrix.get(4), matrix.get(5),
    //              matrix.get(6), matrix.get(7), matrix.get(8));
    return matrix;
}

ComputedStyle* ComputedStyle::cachedPseudoStyle(
    StyleResolver::PseudoElementType pseudoType)
{
    GCVector<ComputedStyle*>* styles = cachedPseudoStyles();
    if (!styles) {
        return nullptr;
    }

    auto it = std::find_if(styles->begin(), styles->end(),
                           [&pseudoType](ComputedStyle* pseudoStyle) {
                               return pseudoStyle->pseudoType() == pseudoType;
                           });

    if (it != styles->end()) {
        return *it;
    } else {
        return nullptr;
    }
}

ComputedStyle* ComputedStyle::addCachedPseudoStyle(ComputedStyle* pseudoStyle)
{
    if (!pseudoStyle) {
        return nullptr;
    }

    STARFISH_ASSERT(pseudoStyle->pseudoType() >
                    StyleResolver::PseudoElementType::PseudoElementNone);

    m_rareComputedStyleData.ensureCachedPsuedoStyles()->push_back(pseudoStyle);
    return pseudoStyle;
}

void ComputedStyle::removeCachedPseudoStyle(
    StyleResolver::PseudoElementType pid)
{
    GCVector<ComputedStyle*>* styles = cachedPseudoStyles();
    if (!styles) {
        return;
    }

    styles->erase(std::remove_if(styles->begin(), styles->end(),
                                 [&pid](ComputedStyle* pseudoStyle) {
                                     return pseudoStyle->pseudoType() == pid;
                                 }),
                  styles->end());
}

ComputedStyle* ComputedStyle::pseudoStyle(
    Element* containerElement, StyleResolver::PseudoElementType pseudoType,
    ComputedStyle* stickyInheritFrom, ComputedStyle* oldPseudoStyleIfHas)
{
    if (!seenPseudoElement(pseudoType)) {
        return nullptr;
    }

    ComputedStyle* cs = cachedPseudoStyle(pseudoType);
    if (!cs) {
        cs = FrameTreeBuilder::pseudoStyleForElementInternal(
            containerElement, pseudoType,
            stickyInheritFrom ? stickyInheritFrom : this, oldPseudoStyleIfHas);
        addCachedPseudoStyle(cs);

        m_styleDamageSource = (StyleResolver::StyleDamageSource)(
            m_styleDamageSource | cs->m_styleDamageSource);
    }
    return cs;
}

bool ComputedStyle::isFourSideBorderStyleValueSolid()
{
    if (!hasRareComputeStyleData()) {
        return false;
    }

    BorderData* border = m_rareComputedStyleData.border();
    if (border) {
        BorderData* b = border;
        return (b->top().style() == BorderStyleValue::SolidBorderStyleValue) &&
               (b->bottom().style() ==
                BorderStyleValue::SolidBorderStyleValue) &&
               (b->left().style() == BorderStyleValue::SolidBorderStyleValue) &&
               (b->right().style() == BorderStyleValue::SolidBorderStyleValue);
    }

    return false;
}
#undef _DAMAGED_KEYS
#undef NEED_TRANSITION
#undef RETURN_NEED_TRANSITION
}
