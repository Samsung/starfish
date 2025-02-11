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
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/dom/Document.h"

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

void SVGAnimateElement::beginElement()
{
    beginElementAt(0);
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

    AnimationKeyframes* animationKeyframes = new AnimationKeyframes();

    // TODO: If a list of values is used, the animation will apply the
    // values in order over the course of the animation. If a list of
    // ‘values’ is specified, any ‘from’, ‘to’ and ‘by’ attribute values are
    // ignored.

    // Parse from and to value.
    CSSStyleValuePair from;
    CSSStyleValuePair to;
    if (!parseFrom(keyKind, from) || !parseTo(keyKind, to)) {
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

    // Parse calcMode.
    SVGAnimationCalcMode calcMode;
    if (!parseCalcMode(calcMode)) {
        // Default values is Linear.
        // can proceed using the default value.
        calcMode = SVGAnimationCalcMode::Linear;
    }
    CubicBezierEaseType easeType =
        svgAnimationCalcModeToCubicBezierEaseType(calcMode);
    animationKeyframes->setTimingFunction(
        CubicBezier::createCubicBezier(easeType));

    // Add keyframes using from and to value to animationKeyframes.
    AddAnimationKeyframe(keyKind, animationKeyframes, from, to);

    Optional<Element*> maybeTargetElement = targetElement();
    if (!maybeTargetElement) {
        STARFISH_LOG_WARN("Invalid animation target element.");
        return;
    }
    Element* targetElement = maybeTargetElement.value();

    // Apply animation for svg.
    m_animationKeyframes = animationKeyframes;
    AnimationApplier applier(targetElement, AnimationType::SVGAnimation,
                             targetElement->style());
    if (!applier.applySVGAnimateElement(this)) {
        m_animationKeyframes = nullptr;
        STARFISH_LOG_WARN("Failed to apply animation.");
        return;
    }

    webView()->updateActiveAnimationExecutorRegistration(
        document()->animationExecutor());
    setNeedsStyleRecalcForAnimation();

    document()->animationExecutor()->fireSVGAnimateBeginEvent(this);
}

void SVGAnimateElement::AddAnimationKeyframe(
    CSSStyleValuePair::KeyKind keyKind, AnimationKeyframes* animationKeyframes,
    const CSSStyleValuePair& from, const CSSStyleValuePair& to)
{
    AnimationKeyframe* fromKeyframe = new AnimationKeyframe();
    fromKeyframe->setKeyframeSelector(0.0);
    fromKeyframe->addProperty(keyKind, from);
    fromKeyframe->setDuration(animationKeyframes->duration());
    fromKeyframe->setTimingFunction(animationKeyframes->timingFunction());
    animationKeyframes->animationKeyframeList().push_back(fromKeyframe);

    AnimationKeyframe* toKeyframe = new AnimationKeyframe();
    toKeyframe->setKeyframeSelector(1.0);
    toKeyframe->addProperty(keyKind, to);
    toKeyframe->setDuration(animationKeyframes->duration());
    toKeyframe->setTimingFunction(animationKeyframes->timingFunction());
    animationKeyframes->animationKeyframeList().push_back(toKeyframe);
}

} // namespace Starfish
