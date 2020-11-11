/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#include <SkMatrix.h>

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/animation/AnimationTask.h"
#include "core/animation/util/AnimationUtil.h"
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
#include "core/style/FilterFunctions.h"
#include "core/style/WillChangeData.h"
#include "core/style/ComputedStyle.h"
#include "core/style/AncestorSelectorFilter.h"

#include "platform/window/PlatformWindow.h"
#include "platform/loader/ResourceLoader.h"

namespace Starfish {

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
        GC_set_bit(
            obj_bitmap,
            GC_WORD_OFFSET(ComputedStyle::InheritedStylesRareData, m_fill));
        GC_set_bit(
            obj_bitmap,
            GC_WORD_OFFSET(ComputedStyle::InheritedStylesRareData, m_stroke));
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

bool ComputedStyle::hasFilter()
{
    if (!m_rareComputedStyleData.m_styles.size()) {
        return false;
    }

    return m_rareComputedStyleData.filter() != nullptr &&
           m_rareComputedStyleData.filter()->size();
}

bool ComputedStyle::hasAvailableFilter()
{
    if (!hasFilter()) {
        return false;
    }

    for (auto filter : *(m_rareComputedStyleData.filter())) {
        switch (filter->type()) {
        case FilterFunctionType::BlurFilterFunctionType:
            if (!(static_cast<BlurFilterFunction*>(filter)
                      ->standardDeviation()
                      .isZero())) {
                return true;
            }
            break;
        case FilterFunctionType::DropShadowFilterFunctionType:
        case FilterFunctionType::HueRotateFilterFunctionType:
        case FilterFunctionType::BrightnessFilterFunctionType:
        case FilterFunctionType::ContrastFilterFunctionType:
        case FilterFunctionType::GrayScaleFilterFunctionType:
        case FilterFunctionType::InvertFilterFunctionType:
        case FilterFunctionType::OpacityFilterFunctionType:
        case FilterFunctionType::SaturateFilterFunctionType:
        case FilterFunctionType::SVGUrlFilterFunctionType:
        default:
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            break;
        }
    }
    return false;
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

StyleTransformDataGroup* ComputedStyle::transforms(NULLABLE Frame* frame)
{
    if (hasRareComputeStyleData() == false) {
        return nullptr;
    }

    // https://www.w3.org/TR/css-transforms-1/#transformable-element
    if (frame != nullptr && frame->isTransformable() == false) {
        return nullptr;
    }

    StyleTransformDataGroup* transforms = m_rareComputedStyleData.transforms();
    if (transforms) {
        return transforms;
    }

    return nullptr;
}

TimingFunction* ComputedStyle::knownTimingFunction(TimingFunctionValue v)
{
    switch (v) {
    case TimingFunctionValue::TimingFunctionEaseValue:
        return new CubicBezier(0.25, 0.1, 0.25, 1);
    case TimingFunctionValue::TimingFunctionLinearValue:
        return new CubicBezier(0.25, 0.25, 0.75, 0.75);
    case TimingFunctionValue::TimingFunctionEaseInValue:
        return new CubicBezier(0.42, 0, 1, 1);
    case TimingFunctionValue::TimingFunctionEaseOutValue:
        return new CubicBezier(0.0, 0.0, 0.58, 1.0);
    case TimingFunctionValue::TimingFunctionEaseInOutValue:
        return new CubicBezier(0.42, 0.0, 0.58, 1.0);
    case TimingFunctionValue::TimingFunctionStepStartValue:
        return new Steps(1, false);
    case TimingFunctionValue::TimingFunctionStepEndValue:
        return new Steps(1, true);
    }
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
}

class BackgroundImageResourceClient : public ResourceClient {
public:
    BackgroundImageResourceClient(Resource* res, Node* node)
        : ResourceClient(res)
        , m_node(node)
    {
    }
    virtual void didLoadFinished()
    {
        m_node->setNeedsPainting();
        if (m_node->isHTMLHtmlElement() || m_node->isHTMLBodyElement()) {
            if (m_node->document()->rootElement()) {
                m_node->document()->rootElement()->setNeedsPainting();
            } else {
                m_node->document()->setNeedsPainting();
            }
        }
    }

    Node* m_node;
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
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
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
                    if (a != b) {
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
    WebView* wv = consumer->webView();
    if (g_enablePixelTest) {
        String* str = String::fromUTF8("StarfishAhem");
        m_font = fs->loadFont(&str, 1, fixedFontSize, style, fontWeight,
                              fixedLetterSpacing);
    } else {
        bool regressionEnable =
            wv->startUpFlag() & StarfishStartUpFlag::enableRegressionTest;

        if (regressionEnable) {
            String** familyNameArray =
                (String**)(&m_inheritedStyles.m_fontFamilyDatas[1]);
            size_t familyNameArraySize =
                m_inheritedStyles.m_fontFamilyDatas[0].m_length;

            for (size_t i = 0; i < familyNameArraySize; i++) {
                if (familyNameArray[i]->contains("ahem", false) ||
                    familyNameArray[i]->contains("CanvasTest", false)) {
                    regressionEnable = false;
                }
            }
        }

        if (regressionEnable) {
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
                    new BackgroundImageResourceClient(res, consumer));

                RequestData* reqData = new RequestData();
                reqData->m_url = u;
                reqData->m_referrer =
                    new ReferrerURL(consumer->document()->documentURI());
                reqData->m_destination = RequestDestination::Image;
#ifdef STARFISH_ENABLE_TEST
                WebView* wv = consumer->webView();
                bool enableRegressionTest =
                    wv->startUpFlag() &
                    StarfishStartUpFlag::enableRegressionTest;
                reqData->m_syncLevel =
                    (g_enablePixelTest || enableRegressionTest)
                        ? RequestSyncLevel::AlwaysSync
                        : RequestSyncLevel::SyncIfAlreadyLoaded;
#else
                reqData->m_syncLevel = RequestSyncLevel::SyncIfAlreadyLoaded;
#endif
                res->request(reqData, true);
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
                new BackgroundImageResourceClient(res, consumer));

            RequestData* reqData = new RequestData();
            reqData->m_url = u;
            reqData->m_referrer =
                new ReferrerURL(consumer->document()->documentURI());
            reqData->m_destination = RequestDestination::Image;
#ifdef STARFISH_ENABLE_TEST
            WebView* wv = consumer->webView();
            bool enableRegressionTest =
                wv->startUpFlag() & StarfishStartUpFlag::enableRegressionTest;

            reqData->m_syncLevel = (g_enablePixelTest || enableRegressionTest)
                                       ? RequestSyncLevel::AlwaysSync
                                       : RequestSyncLevel::SyncIfAlreadyLoaded;
#else
            reqData->m_syncLevel = RequestSyncLevel::SyncIfAlreadyLoaded;
#endif
            res->request(reqData, true);
            setBorderImageResource(res);
        }
    }
}

void ComputedStyle::loadListStyleImage(
    Node* consumer,
    ComputedStyle* prevComputedStyleValueForReferenceLoadedResources)
{
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
                new BackgroundImageResourceClient(res, consumer));

            RequestData* reqData = new RequestData();
            reqData->m_url = u;
            reqData->m_referrer =
                new ReferrerURL(consumer->document()->documentURI());
            reqData->m_destination = RequestDestination::Image;
#ifdef STARFISH_ENABLE_TEST
            WebView* wv = consumer->webView();
            bool enableRegressionTest =
                wv->startUpFlag() & StarfishStartUpFlag::enableRegressionTest;
            reqData->m_syncLevel = (g_enablePixelTest || enableRegressionTest)
                                       ? RequestSyncLevel::AlwaysSync
                                       : RequestSyncLevel::SyncIfAlreadyLoaded;
#else
            reqData->m_syncLevel = RequestSyncLevel::SyncIfAlreadyLoaded;
#endif
            res->request(reqData, true);
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

        if (current &&
            (current->isHTMLInputElement() || current->isHTMLButtonElement())) {
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
        s->updateCurrentColorToFixedColorIfNeeds(color());
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
    Length rootFontSize = Length(
        Length::Fixed, current->document()->webView()->defaultFontSize());
    HTMLHtmlElement* root = current->document()->rootElement();
    if (root && root->style()) {
        rootFontSize = root->style()->fontSize();
    }

    changeFontPercentToFixedIfNeeded(curFontSize, rootFontSize, font(),
                                     current);

    // When <center><table>...</table></center>, table is placed in the middle
    // of the parent block, but the content of the table is not affected by
    // <center>. To do so, Blink seems resets the text-align.
    if (m_inheritedStyles.m_textAlign ==
        TextAlignValue::StarfishCenterTextAlignValue) {
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

        FilterFunctions* filter = m_rareComputedStyleData.filter();
        if (filter) {
            filter->checkComputed(curFontSize, rootFontSize, font, windowSize,
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

    GCVector<GridTrackSize>* columns =
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

    GCVector<GridTrackSize>* rows = m_rareComputedStyleData.gridTemplateRows();

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

ComputedStyleDamage compareStyle(ComputedStyle* oldStyle,
                                 ComputedStyle* newStyle, bool* damagedKeys)
{
    STARFISH_ASSERT(oldStyle != nullptr);
    STARFISH_ASSERT(newStyle != nullptr);
    STARFISH_ASSERT(damagedKeys != nullptr);

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
            if (newStyle->m_inheritedStyles.m_fontFamilyDatas[i + 1]
                    .m_familyName !=
                oldStyle->m_inheritedStyles.m_fontFamilyDatas[i + 1]
                    .m_familyName) {
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

    if (*newStyle->fill() != *oldStyle->fill()) {
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

    if (*newStyle->stroke() != *oldStyle->stroke()) {
        damagedKeys[CSSStyleValuePair::KeyKind::Stroke] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->strokeOpacity() != oldStyle->strokeOpacity()) {
        damagedKeys[CSSStyleValuePair::KeyKind::StrokeOpacity] = true;
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

    if (*newStyle->stopColor() != *oldStyle->stopColor()) {
        damagedKeys[CSSStyleValuePair::KeyKind::StopColor] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->stopOpacity() != oldStyle->stopOpacity()) {
        damagedKeys[CSSStyleValuePair::KeyKind::StopOpacity] = true;
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

    {
        auto oldMargin = oldStyle->rareComputedStyleData()->margin();
        auto newMargin = newStyle->rareComputedStyleData()->margin();

        if (oldMargin || newMargin) {
            if (LengthData::damaged(
                    oldMargin ? *oldMargin : LengthData(),
                    newMargin ? *newMargin : LengthData(),
                    damagedKeys[CSSStyleValuePair::KeyKind::MarginTop],
                    damagedKeys[CSSStyleValuePair::KeyKind::MarginRight],
                    damagedKeys[CSSStyleValuePair::KeyKind::MarginBottom],
                    damagedKeys[CSSStyleValuePair::KeyKind::MarginLeft])) {
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamageLayout | damage);
            }
        }
    }

    {
        auto oldPadding = oldStyle->rareComputedStyleData()->padding();
        auto newPadding = newStyle->rareComputedStyleData()->padding();

        if (oldPadding || newPadding) {
            if (LengthData::damaged(
                    oldPadding ? *oldPadding : LengthData(),
                    newPadding ? *newPadding : LengthData(),
                    damagedKeys[CSSStyleValuePair::KeyKind::PaddingTop],
                    damagedKeys[CSSStyleValuePair::KeyKind::PaddingRight],
                    damagedKeys[CSSStyleValuePair::KeyKind::PaddingBottom],
                    damagedKeys[CSSStyleValuePair::KeyKind::PaddingLeft])) {
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamageLayout | damage);
            }
        }
    }

    {
        auto oldOffset = oldStyle->rareComputedStyleData()->offset();
        auto newOffset = newStyle->rareComputedStyleData()->offset();

        if (oldOffset || newOffset) {
            if (LengthData::damaged(
                    oldOffset ? *oldOffset : LengthData(),
                    newOffset ? *newOffset : LengthData(),
                    damagedKeys[CSSStyleValuePair::KeyKind::Top],
                    damagedKeys[CSSStyleValuePair::KeyKind::Right],
                    damagedKeys[CSSStyleValuePair::KeyKind::Bottom],
                    damagedKeys[CSSStyleValuePair::KeyKind::Left])) {
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamageLayout | damage);
            }
        }
    }

    {
        auto oldBorderData = oldStyle->rareComputedStyleData()->border();
        auto newBorderData = newStyle->rareComputedStyleData()->border();

        if (oldBorderData || newBorderData) {
            BorderData oldBorder =
                oldBorderData ? *oldBorderData
                              : BorderData(BorderData::InitiallyZeroValue);
            BorderData newBorder =
                newBorderData ? *newBorderData
                              : BorderData(BorderData::InitiallyZeroValue);

            BorderValue& oldBorderTop = oldBorder.top();
            BorderValue& oldBorderRight = oldBorder.right();
            BorderValue& oldBorderBottom = oldBorder.bottom();
            BorderValue& oldBorderLeft = oldBorder.left();
            BorderValue& newBorderTop = newBorder.top();
            BorderValue& newBorderRight = newBorder.right();
            BorderValue& newBorderBottom = newBorder.bottom();
            BorderValue& newBorderLeft = newBorder.left();
            bool borderDamage = false;
            if (oldBorderTop.hasBorderColor() !=
                    newBorderTop.hasBorderColor() ||
                oldBorderTop.color() != newBorderTop.color()) {
                damagedKeys[CSSStyleValuePair::KeyKind::BorderTopColor] = true;
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamagePainting | damage);
            }
            if (oldBorderTop.width() != newBorderTop.width()) {
                damagedKeys[CSSStyleValuePair::KeyKind::BorderTopWidth] = true;
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamageLayout | damage);
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamagePainting | damage);
            }
            if (oldBorderTop.style() != newBorderTop.style()) {
                damagedKeys[CSSStyleValuePair::KeyKind::BorderTopStyle] = true;
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamageLayout | damage);
            }
            if (oldBorderRight.hasBorderColor() !=
                    newBorderRight.hasBorderColor() ||
                oldBorderRight.color() != newBorderRight.color()) {
                damagedKeys[CSSStyleValuePair::KeyKind::BorderRightColor] =
                    true;
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamagePainting | damage);
            }
            if (oldBorderRight.width() != newBorderRight.width()) {
                damagedKeys[CSSStyleValuePair::KeyKind::BorderRightWidth] =
                    true;
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamageLayout | damage);
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamagePainting | damage);
            }
            if (oldBorderRight.style() != newBorderRight.style()) {
                damagedKeys[CSSStyleValuePair::KeyKind::BorderRightStyle] =
                    true;
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamageLayout | damage);
            }
            if (oldBorderBottom.hasBorderColor() !=
                    newBorderBottom.hasBorderColor() ||
                oldBorderBottom.color() != newBorderBottom.color()) {
                damagedKeys[CSSStyleValuePair::KeyKind::BorderBottomColor] =
                    true;
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamagePainting | damage);
            }
            if (oldBorderBottom.width() != newBorderBottom.width()) {
                damagedKeys[CSSStyleValuePair::KeyKind::BorderBottomWidth] =
                    true;
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamageLayout | damage);
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamagePainting | damage);
            }
            if (oldBorderBottom.style() != newBorderBottom.style()) {
                damagedKeys[CSSStyleValuePair::KeyKind::BorderBottomStyle] =
                    true;
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamageLayout | damage);
            }
            if (oldBorderLeft.hasBorderColor() !=
                    newBorderLeft.hasBorderColor() ||
                oldBorderLeft.color() != newBorderLeft.color()) {
                damagedKeys[CSSStyleValuePair::KeyKind::BorderLeftColor] = true;
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamagePainting | damage);
            }
            if (oldBorderLeft.width() != newBorderLeft.width()) {
                damagedKeys[CSSStyleValuePair::KeyKind::BorderLeftWidth] = true;
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamageLayout | damage);
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamagePainting | damage);
            }
            if (oldBorderLeft.style() != newBorderLeft.style()) {
                damagedKeys[CSSStyleValuePair::KeyKind::BorderLeftStyle] = true;
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamageLayout | damage);
            }

            if (newBorder.image().url() != oldBorder.image().url()) {
                damagedKeys[CSSStyleValuePair::KeyKind::BorderImageSource] =
                    true;
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamagePainting | damage);
            }
            if (newBorder.image().slices() != oldBorder.image().slices() ||
                newBorder.image().sliceFill() !=
                    oldBorder.image().sliceFill()) {
                damagedKeys[CSSStyleValuePair::KeyKind::BorderImageSlice] =
                    true;
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamagePainting | damage);
            }
            if (newBorder.image().widths() != oldBorder.image().widths()) {
                damagedKeys[CSSStyleValuePair::KeyKind::BorderImageWidth] =
                    true;
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamagePainting | damage);
            }
            if (newBorder.image().outsets() != oldBorder.image().outsets()) {
                damagedKeys[CSSStyleValuePair::KeyKind::BorderImageOutset] =
                    true;
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamagePainting | damage);
            }
            if (newBorder.image().repeatX() != oldBorder.image().repeatX() ||
                newBorder.image().repeatY() != oldBorder.image().repeatY()) {
                damagedKeys[CSSStyleValuePair::KeyKind::BorderImageRepeat] =
                    true;
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamagePainting | damage);
            }
        }
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
        if ((newOpacity == 0 && oldOpacity != 0) ||
            (newOpacity != 0 && oldOpacity == 0)) {
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamagePainting | damage);
        }
        if (newOpacity < 1 && oldOpacity < 1) {
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::
                    ComputedStyleDamageComputeStackingContextProperties |
                damage);
        } else {
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::
                    ComputedStyleDamageEstablishesStackingContext |
                damage);
        }
    }

    if (newStyle->zIndex() != oldStyle->zIndex()) {
        damagedKeys[CSSStyleValuePair::KeyKind::ZIndex] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageEstablishesStackingContext |
            damage);
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    {
        auto oldBackground = oldStyle->rareComputedStyleData()->background();
        auto newBackground = newStyle->rareComputedStyleData()->background();

        if (oldBackground || newBackground) {
            auto oldColor =
                oldBackground ? oldBackground->color() : Unit::Color();
            auto newColor =
                newBackground ? newBackground->color() : Unit::Color();

            if (oldColor != newColor) {
                damagedKeys[CSSStyleValuePair::KeyKind::BackgroundColor] = true;
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamagePainting | damage);
            }

            if (StyleBackgroundData::damaged(
                    oldBackground, newBackground,
                    damagedKeys
                        [CSSStyleValuePair::KeyKind::BackgroundAttachment],
                    damagedKeys[CSSStyleValuePair::KeyKind::BackgroundClip],
                    damagedKeys[CSSStyleValuePair::KeyKind::BackgroundImage],
                    damagedKeys[CSSStyleValuePair::KeyKind::BackgroundOrigin],
                    damagedKeys[CSSStyleValuePair::KeyKind::BackgroundSize],
                    damagedKeys[CSSStyleValuePair::KeyKind::BackgroundRepeatX],
                    damagedKeys[CSSStyleValuePair::KeyKind::BackgroundRepeatY],
                    damagedKeys
                        [CSSStyleValuePair::KeyKind::BackgroundPositionX],
                    damagedKeys
                        [CSSStyleValuePair::KeyKind::BackgroundPositionY])) {
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamagePainting | damage);
            }
        }
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
    StyleTransformDataGroup* newTransforms =
        newStyle->hasRareComputeStyleData()
            ? newStyle->rareComputedStyleData()->transforms()
            : nullptr;
    bool oldComplex = oldTransforms ? (oldTransforms->hasComplexTransform() ||
                                       oldTransforms->has3DTransform())
                                    : false;
    bool newComplex = newTransforms ? (newTransforms->hasComplexTransform() ||
                                       newTransforms->has3DTransform())
                                    : false;
    if (oldTransforms && oldTransforms->size() == 0) {
        oldTransforms = nullptr;
    }
    if (newTransforms && newTransforms->size() == 0) {
        newTransforms = nullptr;
    }
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
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageEstablishesStackingContext |
            damage);
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

    StyleAnimationData* oldAnimation =
        oldStyle->hasRareComputeStyleData() ? oldStyle->animation() : nullptr;
    StyleAnimationData* newAnimation =
        newStyle->hasRareComputeStyleData() ? newStyle->animation() : nullptr;

    if (!oldAnimation && !newAnimation) {
    } else if (!oldAnimation || !newAnimation) {
        damagedKeys[CSSStyleValuePair::KeyKind::Animation] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited | damage);
    }

    String* oldAnimationName = oldStyle->hasRareComputeStyleData()
                                   ? oldStyle->animationName()
                                   : String::emptyString;
    String* newAnimationName = newStyle->hasRareComputeStyleData()
                                   ? newStyle->animationName()
                                   : String::emptyString;
    if (!oldAnimationName->equals(newAnimationName)) {
        damagedKeys[CSSStyleValuePair::KeyKind::AnimationName] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited | damage);
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

    if (newStyle->appearance() != oldStyle->appearance()) {
        damagedKeys[CSSStyleValuePair::KeyKind::Appearance] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    FilterFunctions* newFilter = newStyle->filter();
    FilterFunctions* oldFilter = oldStyle->filter();
    if (newFilter != oldFilter) {
        if ((!newFilter || !oldFilter) || !oldFilter->compare(newFilter)) {
            damagedKeys[CSSStyleValuePair::KeyKind::Filter] = true;
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamagePainting |
                ComputedStyleDamage::
                    ComputedStyleDamageComputeStackingContextProperties |
                damage);
            if (newStyle->hasAvailableFilter() !=
                oldStyle->hasAvailableFilter()) {
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::
                        ComputedStyleDamageEstablishesStackingContext |
                    damage);
            }
        }
    }

    {
        auto newCustomProperty = newStyle->customProperty();
        auto oldCustomProperty = oldStyle->customProperty();

        if (newCustomProperty.size() != oldCustomProperty.size()) {
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamagePainting | damage);
        } else {
            bool changedCustomProperty = false;
            for (auto newValue : newCustomProperty) {
                for (auto oldValue : oldCustomProperty) {
                    if (oldValue != newValue) {
                        changedCustomProperty = true;
                    }
                }
            }

            if (changedCustomProperty) {
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamagePainting | damage);
            }
        }
    }

    return damage;
}

inline double deg2rad(float degree)
{
    return degree * M_PI / 180;
}

SkMatrix ComputedStyle::transformToMatrix(StyleTransformDataGroup* transforms,
                                          LayoutUnit containerWidth,
                                          LayoutUnit containerHeight, Frame* f)
{
    STARFISH_ASSERT(transforms != nullptr);
    STARFISH_ASSERT(f != nullptr);
    STARFISH_ASSERT(f->isTransformable() == true);

    SkMatrix matrix;
    matrix.reset();

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
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    }
    // printf("[%8.4f %8.4f %8.4f][%8.4f %8.4f %8.4f][%8.4f %8.4f %8.4f]\n",
    //         matrix.get(0), matrix.get(1), matrix.get(2), matrix.get(3),
    //         matrix.get(4), matrix.get(5),
    //              matrix.get(6), matrix.get(7), matrix.get(8));
    return matrix;
}

SkMatrix ComputedStyle::transformsToMatrix(LayoutUnit containerWidth,
                                           LayoutUnit containerHeight, Frame* f,
                                           bool isTransformable)
{
    STARFISH_ASSERT(f != nullptr);

    if (hasRareComputeStyleData() == false) {
        return SkMatrix::I();
    }

    StyleTransformDataGroup* transforms = m_rareComputedStyleData.transforms();

    if (transforms == nullptr || isTransformable == false) {
        return SkMatrix::I();
    }

    return transformToMatrix(transforms, containerWidth, containerHeight, f);
}

ComputedStyle* ComputedStyle::cachedPseudoStyle(PseudoElementType pseudoType)
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
                    PseudoElementType::PseudoElementNone);

    m_rareComputedStyleData.ensureCachedPsuedoStyles()->push_back(pseudoStyle);
    return pseudoStyle;
}

void ComputedStyle::removeCachedPseudoStyle(PseudoElementType pid)
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

ComputedStyle* ComputedStyle::pseudoStyle(Element* containerElement,
                                          PseudoElementType pseudoType,
                                          ComputedStyle* stickyInheritFrom,
                                          ComputedStyle* oldPseudoStyleIfHas,
                                          Nullable<StyleResolveContext*> ctx)
{
    if (!seenPseudoElement(pseudoType)) {
        return nullptr;
    }

    ComputedStyle* cs = cachedPseudoStyle(pseudoType);
    if (!cs) {
        cs = ComputedStyle::pseudoStyleForElementInternal(
            containerElement, pseudoType,
            stickyInheritFrom ? stickyInheritFrom : this, oldPseudoStyleIfHas,
            ctx);
        addCachedPseudoStyle(cs);

        m_styleDamageSource = (StyleResolver::StyleDamageSource)(
            m_styleDamageSource | cs->m_styleDamageSource);
    }
    return cs;
}

ComputedStyle* ComputedStyle::pseudoStyleForElementInternal(
    Node* parent, PseudoElementType pseudoId, ComputedStyle* parentStyle,
    ComputedStyle* oldPseudoStyleIfHas, Nullable<StyleResolveContext*> ctx)
{
    STARFISH_ASSERT(pseudoId != PseudoElementType::PseudoElementNone);
    STARFISH_ASSERT(parentStyle);

    ComputedStyle* style;
    if (ctx) {
        style = new (ctx->allocateComputedStyle()) ComputedStyle(parentStyle);
    } else {
        style = new ComputedStyle(parentStyle);
    }

    StyleResolveContext* resolveContext;

    if (ctx) {
        resolveContext = ctx.value();
    } else {
        resolveContext = new (alloca(sizeof(StyleResolveContext)))
            StyleResolveContext(parent->document());
        VectorWithInlineStorage<16, Node*, std::allocator<Node*>> tree;

        Node* n = parent->parentNode();
        while (n) {
            tree.push_back(n);
            n = n->parentNode();
        }

        for (size_t i = tree.size(); i > 0; i--) {
            resolveContext->m_ancestorSelectorFilter->pushNode(tree[i - 1]);
        }
    }

    parent->document()->styleResolver().matchAllRules(
        *resolveContext, parent->asElement(), style, parentStyle, pseudoId);
    Length fontSize = style->fontSize();
    fontSize.changeToFixedIfNeeded(
        parentStyle->fontSize(),
        parent->document()->rootElement()->style()->fontSize(),
        parentStyle->font(), parent->window()->innerWidth(),
        parent->window()->innerHeight(), style);
    style->setFontSize(fontSize);

    // TODO: Set the proper style according to the type of pseudo-elements
    if (pseudoId == PseudoElementType::PseudoElementFirstLetter) {
        style->setDisplay(DisplayValue::InlineDisplayValue);
        style->setPosition(PositionValue::StaticPositionValue);
    }
    style->loadResources(parent, oldPseudoStyleIfHas);
    style->arrangeStyleValues(parentStyle, parent);

    if (!ctx) {
        resolveContext->~StyleResolveContext();
    }

    return style;
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
