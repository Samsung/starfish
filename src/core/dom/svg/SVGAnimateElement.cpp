/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"

#include "SVGAnimateElement.h"

#include "core/animation/AnimatedValue.h"
#include "core/animation/AnimationApplier.h"
#include "core/animation/AnimationTask.h"
#include "core/animation/AnimationExecutor.h"
#include "core/animation/CubicBezier.h"
#include "core/animation/Steps.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/dom/Document.h"
#include "core/style/CSSProperty.h"

namespace Starfish {

SVGAnimateElement::SVGAnimateElement(Document* document,
                                     const QualifiedName& qname)
    : SVGAnimationElement(document, qname)
{
    STARFISH_UNIMPLEMENTED();
}

void* SVGAnimateElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGAnimateElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGAnimateElement)] = { 0 };
        SVGAnimateElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGAnimateElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGAnimateElement::beginElementAt(float offset)
{
    // TODO: Apply offset to animation.

    window()->webView()->layoutIfNeeded(false);

    // Parse target keyKind from attribute name.
    CSSStyleValuePair::KeyKind keyKind;
    if (!parseAttributeName(keyKind)) {
        STARFISH_LOG_WARN("Invalid attribute name.");
        return;
    }

    // Prevent runtime crashes when animating unsupported properties.
    if (!CSSPropertyHelper::isAnimatableProperty(keyKind)) {
        STARFISH_LOG_WARN("Not supported animatable property.");
        return;
    }

    AnimationKeyframes* animationKeyframes = new AnimationKeyframes();

    // If a list of values is used, the animation will apply the
    // values in order over the course of the animation. If a list of
    // ‘values’ is specified, any ‘from’, ‘to’ and ‘by’ attribute values are
    // ignored.
    // TODO: 'by' attribute.
    GCVector<CSSStyleValuePair> values;
    if (hasValues()) {
        if (!parseValues(keyKind, values)) {
            STARFISH_LOG_WARN("Invalid values attribute.");
            return;
        }
    } else if (!parseFrom(keyKind, values) || !parseTo(keyKind, values)) {
        STARFISH_LOG_WARN("Invalid from, to attributes.");
        return;
    }

    // Parse duration.
    CSSTime duration;
    if (!parseDur(duration)) {
        STARFISH_LOG_WARN("Invalid dur attribute.");
        return;
    }
    animationKeyframes->setDuration(duration);

    // Parse fill.
    SVGAnimationFill fill;
    if (!parseFill(fill)) {
        // Default values is remove.
        // can proceed using the default value.
        fill = SVGAnimationFill::Remove;
    }
    AnimationFillModeValue fillMode =
        svgAnimationFillToAnimationFillModeValue(fill);
    animationKeyframes->setFillMode(fillMode);

    // Parse repeatCount.
    float repeatCount;
    if (!parseRepeatCount(repeatCount)) {
        // Default values is 1.
        // can proceed using the default value.
        repeatCount = 1.0f;
    }
    animationKeyframes->setIterationCount(repeatCount);

    // Parse calcMode.
    SVGAnimationCalcMode calcMode = SVGAnimationCalcMode::Linear;
    if (!parseCalcMode(calcMode)) {
        // Default values is Linear.
        // can proceed using the default value.
    }

    // TODO: Apply keyTimes.

    // Parse KeySplines. ignore it if calcMode is not spline.
    Optional<GCVector<TimingFunction*>> maybeKeySplines;
    if (calcMode == SVGAnimationCalcMode::Spline) {
        GCVector<TimingFunction*> keySplines;
        if (!parseKeySplines(keySplines)) {
            STARFISH_LOG_WARN("Invalid KeySplines.");
            return;
        }
        if (keySplines.size() != values.size() - 1) {
            // Fallback guarantee: An animation is to occur, but it should not
            // cause any changes.
            // FIXME: If you think of a better way, please replace it.
            // FIXME: In this case, improve it so that only minimal rendering
            // occurs.
            if (!convertFallbackValues(keyKind, values)) {
                return;
            }
            calcMode = SVGAnimationCalcMode::Linear; // fallback to linear.
        } else {
            maybeKeySplines = keySplines;
        }
    }

    CubicBezierEaseType easeType =
        svgAnimationCalcModeToCubicBezierEaseType(calcMode);
    if (easeType != CubicBezierEaseType::Custom) {
        animationKeyframes->setTimingFunction(
            CubicBezier::createCubicBezier(easeType));
    }

    // Add keyframes using values to animationKeyframes.
    AddAnimationKeyframe(keyKind, animationKeyframes, values, maybeKeySplines);

    Optional<Element*> maybeTargetElement = targetElement();
    if (!maybeTargetElement) {
        STARFISH_LOG_WARN("Invalid animation target element.");
        return;
    }
    Element* targetElement = maybeTargetElement.value();

    // Apply animation for svg.
    m_animationKeyframes = animationKeyframes;
    AnimationApplier applier(targetElement, AnimationType::SVGAnimation,
                             targetElement->style(), this);
    if (!applier.applySVGAnimateElement()) {
        m_animationKeyframes = nullptr;
        STARFISH_LOG_WARN("Failed to apply animation.");
        return;
    }

    webView()->updateActiveAnimationExecutorRegistration(
        document()->animationExecutor());
    setNeedsStyleRecalcForAnimation();

    m_declarations->clear();
}

void SVGAnimateElement::AddAnimationKeyframe(
    CSSStyleValuePair::KeyKind keyKind, AnimationKeyframes* animationKeyframes,
    const GCVector<CSSStyleValuePair>& values,
    Optional<GCVector<TimingFunction*>> maybeKeySplines)
{
    for (size_t i = 0; i < values.size(); ++i) {
        AnimationKeyframe* keyframe = new AnimationKeyframe();
        double offset = 100.0 / (values.size() - 1);
        double keyframeSelector = (offset * i) / 100;
        keyframe->setKeyframeSelector(keyframeSelector);
        keyframe->addProperty(keyKind, values[i]);
        keyframe->setDuration(animationKeyframes->duration());
        if (maybeKeySplines.hasValue() && i < values.size() - 1) {
            keyframe->setTimingFunction(maybeKeySplines.getValue()[i]);
        } else {
            keyframe->setTimingFunction(animationKeyframes->timingFunction());
        }
        animationKeyframes->animationKeyframeList().push_back(keyframe);
    }
}

} // namespace Starfish
