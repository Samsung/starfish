/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/animation/Animation.h"
#include "core/dom/Node.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBlockBox.h"
#include "core/page/Window.h"
#include "core/style/ComputedStyle.h"
#include "platform/window/PlatformWindow.h"

namespace StarFish {

FontFamilyData g_initialFontFamilyDatas[2] = {
    1, String::createASCIIStringWithNoGC(STARFISH_DEFAULT_FONT_FAMILY)
};

void* RareComputedStyleData::operator new(size_t size)
{
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
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(
            ComputedStyle::InheritedStylesRareData)] = { 0 };
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(ComputedStyle::InheritedStylesRareData,
                                  m_textShadowDataList));
        descr = GC_make_descriptor(
            obj_bitmap, GC_WORD_LEN(ComputedStyle::InheritedStylesRareData));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void* ComputedStyle::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(ComputedStyle)] = { 0 };
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(ComputedStyle, m_inheritedStyles.m_rareData));
        GC_set_bit(
            obj_bitmap,
            GC_WORD_OFFSET(ComputedStyle, m_inheritedStyles.m_fontFamilyDatas));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(ComputedStyle, m_font));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(ComputedStyle, m_width));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(ComputedStyle, m_height));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(ComputedStyle, m_rareComputedStyleData));
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

StyleTransformDataGroup* ComputedStyle::transforms(Frame* frame)
{
    if (!hasRareComputeStyleData()) {
        return nullptr;
    }

    // https://www.w3.org/TR/css-transforms-1/#transformable-element
    if (frame && (frame->isFrameInline() || frame->isInlineNonReplacedBox())) {
        return nullptr;
    }

    StyleTransformDataGroup* transforms = m_rareComputedStyleData->transforms();
    if (transforms) {
        return transforms;
    }

    return nullptr;
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

void ComputedStyle::loadFont(Node* consumer)
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
            parentStyle->fontWeight() == this->fontWeight()) {
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

#ifdef STARFISH_ENABLE_TEST
    StarFish* sf = consumer->starFish();
    if (g_enablePixelTest) {
        String* str = String::fromUTF8("Ahem");
        m_font = fs->loadFont(&str, 1, fixedFontSize, style, fontWeight);
    } else {
        if (sf->startUpFlag() & StarFishStartUpFlag::enableRegressionTest) {
            String* str = String::fromUTF8("SamsungOne");
            m_font = fs->loadFont(&str, 1, fixedFontSize, style, fontWeight);
        } else {
            if (canUseParentFont) {
                m_font = parentNodeFont;
            } else {
                m_font = fs->loadFont(
                    (String**)&m_inheritedStyles.m_fontFamilyDatas[1],
                    m_inheritedStyles.m_fontFamilyDatas[0].m_length,
                    fixedFontSize, style, fontWeight);
            }
        }
    }
#else
    if (canUseParentFont) {
        m_font = parentNodeFont;
    } else {
        m_font = fs->loadFont((String**)&m_inheritedStyles.m_fontFamilyDatas[1],
                              m_inheritedStyles.m_fontFamilyDatas[0].m_length,
                              fixedFontSize, style, fontWeight);
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
        if (!backgroundImage(bgIndex)->equals(String::emptyString)) {
            ResourceURL* u =
                new ResourceURL(backgroundImage(bgIndex),
                                consumer->document()->documentURI()->baseURI());

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
        ResourceURL* u =
            new ResourceURL(border.image().url(),
                            consumer->document()->documentURI()->baseURI());

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

        setBorderLeftStyle(BorderStyleValue::SolidBorderStyleValue);
        setBorderTopStyle(BorderStyleValue::SolidBorderStyleValue);
        setBorderRightStyle(BorderStyleValue::SolidBorderStyleValue);
        setBorderBottomStyle(BorderStyleValue::SolidBorderStyleValue);
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
}

void ComputedStyle::changeFontPercentToFixedIfNeeded(Length curFontSize,
                                                     Length rootFontSize,
                                                     Font* font, Node* current)
{
    Window* w = current->window();
    LayoutSize windowSize(w->innerWidth(), w->innerHeight());
    if (!letterSpacing().isComputed()) {
        auto v = letterSpacing();
        v.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                windowSize.width(), windowSize.height(), this);
        setLetterSpacing(v);
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

    m_width.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                  windowSize.width(), windowSize.height(),
                                  this);
    m_height.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                   windowSize.width(), windowSize.height(),
                                   this);
    if (hasRareComputeStyleData()) {
        Nullable<Length> minWidth = m_rareComputedStyleData->minWidth();
        if (minWidth.hasValue()) {
            m_rareComputedStyleData->ensureMinWidth()->changeToFixedIfNeeded(
                curFontSize, rootFontSize, font, windowSize.width(),
                windowSize.height(), this);
        }
        Nullable<Length> maxWidth = m_rareComputedStyleData->maxWidth();
        if (maxWidth.hasValue()) {
            m_rareComputedStyleData->ensureMaxWidth()->changeToFixedIfNeeded(
                curFontSize, rootFontSize, font, windowSize.width(),
                windowSize.height(), this);
        }

        Nullable<Length> minHeight = m_rareComputedStyleData->minHeight();
        if (minHeight.hasValue()) {
            m_rareComputedStyleData->ensureMinHeight()->changeToFixedIfNeeded(
                curFontSize, rootFontSize, font, windowSize.width(),
                windowSize.height(), this);
        }

        Nullable<Length> maxHeight = m_rareComputedStyleData->maxHeight();
        if (maxHeight.hasValue()) {
            m_rareComputedStyleData->ensureMaxHeight()->changeToFixedIfNeeded(
                curFontSize, rootFontSize, font, windowSize.width(),
                windowSize.height(), this);
        }

        Nullable<Length> verticalAlignLength =
            m_rareComputedStyleData->verticalAlignLength();
        if (verticalAlignLength.hasValue()) {
            m_rareComputedStyleData->ensureVerticalAlignLength()
                ->changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                        windowSize.width(), windowSize.height(),
                                        this);
        }

        OutlineData* outline = m_rareComputedStyleData->outline();
        if (outline) {
            outline->checkComputed(curFontSize, rootFontSize, font, windowSize,
                                   this);
        }

        BorderRadiusData* borderRadius =
            m_rareComputedStyleData->borderRadius();
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
            m_rareComputedStyleData->transforms();
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
            m_rareComputedStyleData->transformOrigin();
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

        StyleBackgroundData* background = m_rareComputedStyleData->background();
        if (background) {
            background->checkComputed(curFontSize, rootFontSize, font,
                                      windowSize, this);
        }

        Nullable<BorderData> border = m_rareComputedStyleData->border();
        if (border.hasValue()) {
            BorderData* b = m_rareComputedStyleData->ensureBorder();
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

        Nullable<LengthData> padding = m_rareComputedStyleData->padding();
        if (padding.hasValue()) {
            m_rareComputedStyleData->ensurePadding()->checkComputed(
                curFontSize, rootFontSize, font, windowSize, this);
        }

        Nullable<LengthData> margin = m_rareComputedStyleData->margin();
        if (margin.hasValue()) {
            m_rareComputedStyleData->ensureMargin()->checkComputed(
                curFontSize, rootFontSize, font, windowSize, this);
        }

        Nullable<LengthData> offset = m_rareComputedStyleData->offset();
        if (offset.hasValue()) {
            m_rareComputedStyleData->ensureOffset()->checkComputed(
                curFontSize, rootFontSize, font, windowSize, this);
        }
    }

    if (textShadow().size()) {
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
        }
    }
}

void applyTransition(Element* element, ComputedStyle* oldStyle,
                     ComputedStyle* newStyle)
{
    AnimationExecutor* executor = element->document()->animationExecutor();
    if (newStyle->transitionProperty() ==
        TransitionPropertyValue::TransitionPropertyWidthValue) {
        Length from = oldStyle->width();
        Length to = newStyle->width();

        // TODO this check is wrong. we should use BoundRect for this
        if (from.isFixed() && to.isFixed()) {
            executor->registerAnimation(new LengthAnimationTask(
                element, CSSStyleValuePair::KeyKind::Width, AnimatedValue(from),
                AnimatedValue(to), newStyle->transitionDuration().value(), 0,
                new CubicBeizer(0.25, 0.1, 0.25, 1)));
            // keep current computed style
            newStyle->setWidth(from);
        }
    } else if (newStyle->transitionProperty() ==
               TransitionPropertyValue::TransitionPropertyHeightValue) {
        Length from = oldStyle->height();
        Length to = newStyle->height();

        // TODO this check is wrong. we should use BoundRect for this
        if (from.isFixed() && to.isFixed()) {
            executor->registerAnimation(new LengthAnimationTask(
                element, CSSStyleValuePair::KeyKind::Height,
                AnimatedValue(from), AnimatedValue(to),
                newStyle->transitionDuration().value(), 0,
                new CubicBeizer(0.25, 0.1, 0.25, 1)));
            // keep current computed style
            newStyle->setHeight(from);
        }
    } else if (newStyle->transitionProperty() ==
               TransitionPropertyValue::TransitionPropertyAllValue) {
    }
}

ComputedStyleDamage compareStyle(ComputedStyle* oldStyle,
                                 ComputedStyle* newStyle)
{
    ComputedStyleDamage damage = ComputedStyleDamage::ComputedStyleDamageNone;
    if (newStyle->m_inheritedStyles.m_color !=
        oldStyle->m_inheritedStyles.m_color) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->m_inheritedStyles.m_direction !=
        oldStyle->m_inheritedStyles.m_direction) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
    }

    if (newStyle->m_inheritedStyles.m_whiteSpace !=
        oldStyle->m_inheritedStyles.m_whiteSpace) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->fontSize() != oldStyle->fontSize()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_inheritedStyles.m_fontStyle !=
        oldStyle->m_inheritedStyles.m_fontStyle) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_inheritedStyles.m_fontFamilyDatas[0].m_length !=
        oldStyle->m_inheritedStyles.m_fontFamilyDatas[0].m_length) {
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
                damage = (ComputedStyleDamage)(
                    ComputedStyleDamage::ComputedStyleDamageInherited |
                    ComputedStyleDamage::ComputedStyleDamageLayout | damage);
                break;
            }
        }
    }

    if (newStyle->m_inheritedStyles.m_fontWeight !=
        oldStyle->m_inheritedStyles.m_fontWeight) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_inheritedStyles.m_wordWrap !=
        oldStyle->m_inheritedStyles.m_wordWrap) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->letterSpacing() != oldStyle->letterSpacing()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->lineHeight() != oldStyle->lineHeight()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->textIndent() != oldStyle->textIndent()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->textTransform() != oldStyle->textTransform()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_inheritedStyles.m_textAlign !=
        oldStyle->m_inheritedStyles.m_textAlign) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_inheritedStyles.m_visibility !=
        oldStyle->m_inheritedStyles.m_visibility) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->m_inheritedStyles.m_borderCollapse !=
        oldStyle->m_inheritedStyles.m_borderCollapse) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->horizontalBorderSpacing() !=
        oldStyle->horizontalBorderSpacing()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->verticalBorderSpacing() !=
        oldStyle->verticalBorderSpacing()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_inheritedStyles.m_captionSide !=
        oldStyle->m_inheritedStyles.m_captionSide) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_inheritedStyles.m_emptyCells !=
        oldStyle->m_inheritedStyles.m_emptyCells) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->fill() != oldStyle->fill()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->fillRule() != oldStyle->fillRule()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->fillOpacity() != oldStyle->fillOpacity()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->stroke() != oldStyle->stroke()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->strokeWidth() != oldStyle->strokeWidth()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->m_display != oldStyle->m_display) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
    }

    if (newStyle->m_originalDisplay != oldStyle->m_originalDisplay) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
    }

    if (newStyle->m_position != oldStyle->m_position) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
    }

    if (newStyle->m_float != oldStyle->m_float) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
    }

    if (newStyle->m_clear != oldStyle->m_clear) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_width != oldStyle->m_width) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->minWidth() != oldStyle->minWidth()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->maxWidth() != oldStyle->maxWidth()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_height != oldStyle->m_height) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->minHeight() != oldStyle->minHeight()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->maxHeight() != oldStyle->maxHeight()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->margin() != oldStyle->margin()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->padding() != oldStyle->padding()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->offset() != oldStyle->offset()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->border() != oldStyle->border()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_unicodeBidi != oldStyle->m_unicodeBidi) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_verticalAlign != oldStyle->m_verticalAlign) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->verticalAlignLength() != oldStyle->verticalAlignLength()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_overflowX != oldStyle->m_overflowX &&
        newStyle->m_overflowY != oldStyle->m_overflowY) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    } else if (newStyle->m_overflowX != oldStyle->m_overflowX) {
        if (oldStyle->m_overflowX == OverflowValue::AutoOverflow &&
            newStyle->m_overflowX == OverflowValue::VisibleOverflow &&
            oldStyle->m_overflowY != OverflowValue::VisibleOverflow) {
        } else {
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamageLayout | damage);
        }
    } else if (newStyle->m_overflowY != oldStyle->m_overflowY) {
        if (oldStyle->m_overflowY == OverflowValue::AutoOverflow &&
            newStyle->m_overflowY == OverflowValue::VisibleOverflow &&
            oldStyle->m_overflowX != OverflowValue::VisibleOverflow) {
        } else {
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamageLayout | damage);
        }
    }

    if (newStyle->m_tableLayout != oldStyle->m_tableLayout) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    // NOTE.
    // text-decoration is not inherited.
    // but it influence its child boxes, within it's inline formatting context
    // and is further propagated to any in-flow block-level boxes that split the
    // inline (see section 9.2.1.1).
    // https://www.w3.org/TR/CSS2/text.html#propdef-text-decoration
    if (newStyle->m_textDecoration != oldStyle->m_textDecoration) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    float newOpacity = newStyle->opacity();
    float oldOpacity = oldStyle->opacity();

    if (newOpacity != oldOpacity) {
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
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
    }

    StyleBackgroundData* oldBackground =
        oldStyle->hasRareComputeStyleData()
            ? oldStyle->rareComputedStyleData()->background()
            : nullptr;
    StyleBackgroundData* newBackground =
        newStyle->hasRareComputeStyleData()
            ? newStyle->rareComputedStyleData()->background()
            : nullptr;

    if (newBackground == nullptr && oldBackground == nullptr) {
    } else if (newBackground == nullptr || oldBackground == nullptr) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    } else if (*newBackground != *oldBackground) {
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
    StyleTransformDataGroup* newTransforms =
        newStyle->hasRareComputeStyleData()
            ? newStyle->rareComputedStyleData()->transforms()
            : nullptr;

    bool oldComplex =
        oldTransforms ? oldTransforms->hasComplexTransform() : false;
    bool newComplex =
        newTransforms ? newTransforms->hasComplexTransform() : false;
    if (oldComplex != newComplex) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::
                ComputedStyleDamageComputeStackingContextProperties |
            damage);
    }

    if (newTransforms == nullptr && oldTransforms == nullptr) {
    } else if (newTransforms == nullptr || oldTransforms == nullptr) {
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
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::
                ComputedStyleDamageComputeStackingContextProperties |
            damage);
    } else {
        if (*newTransformOrigin != *oldTransformOrigin) {
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::
                    ComputedStyleDamageComputeStackingContextProperties |
                damage);
        }
    }

    if (newStyle->m_boxSizing != oldStyle->m_boxSizing) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_flexDirection != oldStyle->m_flexDirection) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_flexWrap != oldStyle->m_flexWrap) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->order() != oldStyle->order()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_justifyContent != oldStyle->m_justifyContent) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_alignItems != oldStyle->m_alignItems) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_alignSelf != oldStyle->m_alignSelf) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_alignContent != oldStyle->m_alignContent) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->flexGrow() != oldStyle->flexGrow()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->flexShrink() != oldStyle->flexShrink()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->flexBasis() != oldStyle->flexBasis()) {
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
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
    } else {
        if (*oldContent != *newContent) {
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
        }
    }

    if (newStyle->outlineColor() != oldStyle->outlineColor()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->outlineStyle() != oldStyle->outlineStyle()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->outlineWidth() != oldStyle->outlineWidth()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->outlineOffset() != oldStyle->outlineOffset()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->borderRadius() != oldStyle->borderRadius()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->textShadow() != oldStyle->textShadow()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    return damage;
}

inline double deg2rad(float degree)
{
    return degree * 3.14159265358979323846 / 180;
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

    StyleTransformDataGroup* transforms = m_rareComputedStyleData->transforms();

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
    StyleResolver::PseudoElementType pid)
{
    if (pseudoType() != StyleResolver::PseudoElementType::PseudoElementNone) {
        return nullptr;
    }

    GCVector<ComputedStyle*>* styles = cachedPseudoStyles();
    if (!styles) {
        return nullptr;
    }

    auto it = std::find_if(styles->begin(), styles->end(),
                           [&pid](ComputedStyle* pseudoStyle) {
                               return pseudoStyle->pseudoType() == pid;
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

    setRareComputedStyleDataIfNeeded();
    m_rareComputedStyleData->ensureCachedPsuedoStyles()->push_back(pseudoStyle);
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

bool ComputedStyle::isFourSideBorderStyleValueSolid()
{
    if (!m_rareComputedStyleData) {
        return false;
    }

    Nullable<BorderData> border = m_rareComputedStyleData->border();
    if (border.hasValue()) {
        BorderData b = border.getValue();
        return (b.top().style() == BorderStyleValue::SolidBorderStyleValue) &&
               (b.bottom().style() ==
                BorderStyleValue::SolidBorderStyleValue) &&
               (b.left().style() == BorderStyleValue::SolidBorderStyleValue) &&
               (b.right().style() == BorderStyleValue::SolidBorderStyleValue);
    }

    return false;
}
}
