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
#include "core/layout/Frame.h"
#include "core/layout/FrameBlockBox.h"
#include "core/page/Window.h"
#include "core/style/ComputedStyle.h"
#include "platform/window/PlatformWindow.h"

namespace StarFish {

bool ComputedStyle::hasTransforms(Frame* frame)
{
    return transforms(frame) != nullptr;
}

StyleTransformDataGroup* ComputedStyle::transforms(Frame* frame)
{
    // https://www.w3.org/TR/css-transforms-1/#transformable-element
    if (frame->isFrameInline() || frame->isInlineNonReplacedBox()) {
        return nullptr;
    }
    return m_transforms;
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

void ComputedStyle::loadResources(
    Node* consumer,
    ComputedStyle* prevComputedStyleValueForReferenceLoadedResources)
{
    StarFish* sf = consumer->starFish();
    float fontSize = m_inheritedStyles.m_fontSize.fixed();

    char style = m_inheritedStyles.m_fontStyle;
    char fontWeight = 4;

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

#ifdef STARFISH_ENABLE_TEST
    if (g_enablePixelTest) {
        m_font = sf->fetchFont(String::fromUTF8("Ahem"), fontSize, style,
                               fontWeight);
    } else {
        if (sf->startUpFlag() & StarFishStartUpFlag::enableRegressionTest) {
            m_font = sf->fetchFont(String::fromUTF8("SamsungOne"), fontSize,
                                   style, fontWeight);
        } else {
            m_font =
                sf->fetchFont(String::emptyString, fontSize, style, fontWeight);
        }
    }
#else
    m_font = sf->fetchFont(String::emptyString, fontSize, style, fontWeight);
#endif

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
                    ->bgImageResource(bgIndex) &&
                *(prevComputedStyleValueForReferenceLoadedResources
                      ->background()
                      ->bgImageResource(bgIndex)
                      ->url()) == *u) {
                consumer->document()
                    ->resourceLoader()
                    .notifyImageResourceActiveState(
                        prevComputedStyleValueForReferenceLoadedResources
                            ->background()
                            ->bgImageResource());
                setBackgroundImageResource(
                    prevComputedStyleValueForReferenceLoadedResources
                        ->background()
                        ->bgImageResource(),
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
                              SyncIfAlreadyLoaded);
#else
                res->request(
                    Resource::ResourceRequestSyncLevel::SyncIfAlreadyLoaded);
#endif
            }
        }
        bgIndex++;
    }

    if (!borderImageSource()->equals(String::emptyString)) {
        ResourceURL* u =
            new ResourceURL(borderImageSource(),
                            consumer->document()->documentURI()->baseURI());

        if (prevComputedStyleValueForReferenceLoadedResources &&
            prevComputedStyleValueForReferenceLoadedResources
                ->hasBorderImageData() &&
            prevComputedStyleValueForReferenceLoadedResources->surround()
                ->border.image()
                .imageResource() &&
            *(prevComputedStyleValueForReferenceLoadedResources->surround()
                  ->border.image()
                  .imageResource()
                  ->url()) == *u) {
            consumer->document()
                ->resourceLoader()
                .notifyImageResourceActiveState(
                    prevComputedStyleValueForReferenceLoadedResources
                        ->surround()
                        ->border.image()
                        .imageResource());
            ImageResource* res =
                prevComputedStyleValueForReferenceLoadedResources->surround()
                    ->border.image()
                    .imageResource();
            setBorderImageResource(res);
        } else {
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
                    : Resource::ResourceRequestSyncLevel::SyncIfAlreadyLoaded);
#else
            res->request(
                Resource::ResourceRequestSyncLevel::SyncIfAlreadyLoaded);
#endif
            setBorderImageResource(res);
        }

        setBorderLeftStyle(BorderStyleValue::SolidBorderStyleValue);
        setBorderTopStyle(BorderStyleValue::SolidBorderStyleValue);
        setBorderRightStyle(BorderStyleValue::SolidBorderStyleValue);
        setBorderBottomStyle(BorderStyleValue::SolidBorderStyleValue);
    }
}

void ComputedStyle::arrangeStyleValues(ComputedStyle* parentStyle,
                                       Node* current)
{
    if (current != nullptr && current->isHTMLHtmlElement()) {
        if (m_display == DisplayValue::InlineDisplayValue ||
            m_display == DisplayValue::InlineBlockDisplayValue) {
            m_display = DisplayValue::BlockDisplayValue;
        }
    }
    // 9.7 Relationships between 'display', 'position', and 'float'
    m_originalDisplay = m_display;
    if (m_originalDisplay != DisplayValue::NoneDisplayValue) {
        if (position() == AbsolutePositionValue) {
            m_display = DisplayValue::BlockDisplayValue;
        } else if (m_float != FloatValue::NoneFloatValue) {
            m_display = DisplayValue::BlockDisplayValue;
        }
    }

    if (lineHeight().isPercent()) {
        if (lineHeight().percent() == -100) {
        } else {
            // The computed value of the property is this percentage multiplied
            // by the element's computed font size. Negative values are illegal.
            setLineHeight(Length(Length::Fixed,
                                 lineHeight().percent() * fontSize().fixed()));
        }
    }

    // Convert all non-computed Lengths to computed Length
    STARFISH_ASSERT(m_inheritedStyles.m_fontSize.isFixed());
    Length baseFontSize = fontSize();
    m_inheritedStyles.m_letterSpacing.changeToFixedIfNeeded(baseFontSize,
                                                            font());
    m_inheritedStyles.m_lineHeight.changeToFixedIfNeeded(baseFontSize, font());
    m_inheritedStyles.m_horizontalBorderSpacing.changeToFixedIfNeeded(
        baseFontSize, font());
    m_inheritedStyles.m_verticalBorderSpacing.changeToFixedIfNeeded(
        baseFontSize, font());
    m_width.changeToFixedIfNeeded(baseFontSize, font());
    m_minWidth.changeToFixedIfNeeded(baseFontSize, font());
    m_maxWidth.changeToFixedIfNeeded(baseFontSize, font());
    m_height.changeToFixedIfNeeded(baseFontSize, font());
    m_minHeight.changeToFixedIfNeeded(baseFontSize, font());
    m_maxHeight.changeToFixedIfNeeded(baseFontSize, font());
    m_verticalAlignLength.changeToFixedIfNeeded(baseFontSize, font());
    if (m_transforms) {
        size_t sz = m_transforms->size();
        for (size_t i = 0; i < sz; i++) {
            StyleTransformData std = m_transforms->at(i);
            if (std.type() != StyleTransformData::OperationType::Translate) {
                continue;
            }
            std.changeToFixedIfNeeded(baseFontSize, font());
        }
    }

    if (m_surround) {
        m_surround->margin.checkComputed(baseFontSize, font());

        m_surround->padding.checkComputed(baseFontSize, font());

        m_surround->offset.checkComputed(baseFontSize, font());

        m_surround->border.checkComputed(baseFontSize, font());

        if (hasBorderStyle() && !hasBorderColor()) {
            // If an element's border color is not specified with a border
            // property,
            // user agents must use the value of the element's 'color' property
            // as the computed value for the border color.
            setBorderTopColor(m_inheritedStyles.m_color);
            setBorderRightColor(m_inheritedStyles.m_color);
            setBorderBottomColor(m_inheritedStyles.m_color);
            setBorderLeftColor(m_inheritedStyles.m_color);
        }
    }

    if (m_background) {
        m_background->checkComputed(baseFontSize, font(), color());
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
        }
        newStyle->setHeight(from);
    } else if (newStyle->transitionProperty() ==
               TransitionPropertyValue::TransitionPropertyAllValue) {
    }
}

ComputedStyleDamage compareStyle(ComputedStyle* oldStyle,
                                 ComputedStyle* newStyle)
{
    ComputedStyleDamage damage = ComputedStyleDamage::ComputedStyleDamageNone;
    if (memcmp(&oldStyle->m_inheritedStyles, &newStyle->m_inheritedStyles,
               sizeof(ComputedStyle::InheritedStyles)) != 0) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited | damage);
    }

    if (newStyle->m_inheritedStyles.m_color !=
        oldStyle->m_inheritedStyles.m_color) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->m_inheritedStyles.m_direction !=
        oldStyle->m_inheritedStyles.m_direction) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
    }

    if (newStyle->m_inheritedStyles.m_whiteSpace !=
        oldStyle->m_inheritedStyles.m_whiteSpace) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_inheritedStyles.m_fontSize !=
        oldStyle->m_inheritedStyles.m_fontSize) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_inheritedStyles.m_fontStyle !=
        oldStyle->m_inheritedStyles.m_fontStyle) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_inheritedStyles.m_fontWeight !=
        oldStyle->m_inheritedStyles.m_fontWeight) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_inheritedStyles.m_letterSpacing !=
        oldStyle->m_inheritedStyles.m_letterSpacing) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_inheritedStyles.m_lineHeight !=
        oldStyle->m_inheritedStyles.m_lineHeight) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_inheritedStyles.m_textAlign !=
        oldStyle->m_inheritedStyles.m_textAlign) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_inheritedStyles.m_visibility !=
        oldStyle->m_inheritedStyles.m_visibility) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->m_inheritedStyles.m_borderCollapse !=
        oldStyle->m_inheritedStyles.m_borderCollapse) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_inheritedStyles.m_horizontalBorderSpacing !=
        oldStyle->m_inheritedStyles.m_horizontalBorderSpacing) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_inheritedStyles.m_verticalBorderSpacing !=
        oldStyle->m_inheritedStyles.m_verticalBorderSpacing) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_inheritedStyles.m_captionSide !=
        oldStyle->m_inheritedStyles.m_captionSide) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
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

    if (newStyle->m_minWidth != oldStyle->m_minWidth) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_maxWidth != oldStyle->m_maxWidth) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_height != oldStyle->m_height) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_minHeight != oldStyle->m_minHeight) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_maxHeight != oldStyle->m_maxHeight) {
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

    if (newStyle->m_verticalAlignLength != oldStyle->m_verticalAlignLength) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_overflow != oldStyle->m_overflow) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
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
    // we store text-decoration values in FrameBlockBox
    // so we need to rebuild frame from this point
    // https://www.w3.org/TR/CSS2/text.html#propdef-text-decoration
    if (newStyle->m_textDecoration != oldStyle->m_textDecoration) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
    }

    if (newStyle->m_opacity != oldStyle->m_opacity) {
        if ((newStyle->m_opacity) < 1 && (oldStyle->m_opacity < 1)) {
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamageComposite | damage);
        } else {
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
        }
    }

    // FIXME changing z-index not always cause tree-rebuild
    if (newStyle->m_zIndex != oldStyle->m_zIndex) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
    }

    if (newStyle->m_background == nullptr &&
        oldStyle->m_background == nullptr) {
    } else if (newStyle->m_background == nullptr ||
               oldStyle->m_background == nullptr) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    } else if (*newStyle->m_background != *oldStyle->m_background) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->m_surround == nullptr && oldStyle->m_surround == nullptr) {
    } else if (newStyle->m_surround == nullptr ||
               oldStyle->m_surround == nullptr) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    } else if (*newStyle->m_surround != *oldStyle->m_surround) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }

    if (newStyle->m_transforms == nullptr &&
        oldStyle->m_transforms == nullptr) {
    } else if (newStyle->m_transforms == nullptr ||
               oldStyle->m_transforms == nullptr) {
        // if element has transform, we should re-layout for building
        // stacking-context
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageLayout | damage);
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    } else {
        if (*newStyle->m_transforms != *oldStyle->m_transforms) {
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamageComposite | damage);
        }
    }

    if (newStyle->m_transformOrigin == nullptr &&
        oldStyle->m_transformOrigin == nullptr) {
    } else if (newStyle->m_transformOrigin == nullptr ||
               oldStyle->m_transformOrigin == nullptr) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageComposite | damage);
    } else {
        if (!(*newStyle->m_transformOrigin == *oldStyle->m_transformOrigin)) {
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamageComposite | damage);
        }
    }

    // The style for the 'content' property is computed when we build the frame
    // tree if it is needed.
    if (newStyle->m_content != oldStyle->m_content) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
    }

    return damage;
}

inline double deg2rad(float degree)
{
    return degree * 3.14159265358979323846 / 180;
}

SkMatrix ComputedStyle::transformsToMatrix(LayoutUnit containerWidth,
                                           LayoutUnit containerHeight,
                                           bool isTransformable)
{
    SkMatrix matrix;
    matrix.reset();
    if (!m_transforms || !isTransformable) {
        return matrix;
    }
    for (size_t i = 0; i < m_transforms->size(); i++) {
        StyleTransformData t = m_transforms->at(i);
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
            matrix.preTranslate(m->tx().specifiedValue(containerWidth),
                                m->ty().specifiedValue(containerHeight));
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }
    return matrix;
}

ComputedStyle* ComputedStyle::cachedPseudoStyle(
    StyleResolver::PseudoElementType pid)
{
    if (pseudoType() != StyleResolver::PseudoElementType::PseudoElementNone) {
        return nullptr;
    }

    auto it =
        std::find_if(m_cachedPseudoStyles.begin(), m_cachedPseudoStyles.end(),
                     [&pid](ComputedStyle* pseudoStyle) {
                         return pseudoStyle->pseudoType() == pid;
                     });

    if (it != m_cachedPseudoStyles.end()) {
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

    m_cachedPseudoStyles.push_back(pseudoStyle);
    return pseudoStyle;
}

void ComputedStyle::removeCachedPseudoStyle(
    StyleResolver::PseudoElementType pid)
{
    m_cachedPseudoStyles.erase(
        std::remove_if(m_cachedPseudoStyles.begin(), m_cachedPseudoStyles.end(),
                       [&pid](ComputedStyle* pseudoStyle) {
                           return pseudoStyle->pseudoType() == pid;
                       }),
        m_cachedPseudoStyles.end());
}
}
