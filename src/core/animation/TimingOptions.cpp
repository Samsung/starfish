/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/animation/Animation.h"
#include "core/animation/CubicBezier.h"
#include "core/animation/TimingOptions.h"
#include "core/dom/DOMException.h"
#include "core/dom/Element.h"
#include "core/style/ComputedStyle.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"

namespace Starfish {

static size_t gAnimationCount = 0;

TimingOutput::TimingOutput()
    : m_startDelay(0)
    , m_endDelay(0)
    , m_fill(AnimationFillModeValue::AnimationFillModeNoneValue)
    , m_iterationStart(0.0)
    , m_iterationCount(1.0)
    , m_iterationDuration(0.0)
    , m_direction(AnimationDirectionValue::AnimationDirectionNormalValue)
    , m_easing(new CubicBezier(0.25, 0.1, 0.25, 1))
{
}

bool TimingOptions::makeTimingOptions(Element* element,
                                      KeyframeAnimationOptions& options)
{
    ComputedStyle* style = element->style();
    if (!style) {
        return false;
    }

    TimingOutput output;
    AtomicString name =
        AtomicString::createAtomicString(element->starfish(), "AnimationId");
    StringBuilder id;
    id.appendString(name.string());
    id.appendString(String::fromInt64(gAnimationCount++));
    options.setId(id.finalize());

    setFillMode(output, options);

    if (!setIterationStart(element, options, output)) {
        return false;
    }

    if (!setIterationCount(element, options, output)) {
        return false;
    }

    if (!setIterationDuration(element, options, output)) {
        return false;
    }

    setDirection(output, options);

    if (!setTimingFunction(element, options, output)) {
        return false;
    }

    StyleAnimationData* animation = style->animation();
    size_t animationNameSize = animation ? animation->animationNameSize() : 0;

    // TimingOutput to StyleAnimationData
    style->setAnimationName(options.id(), animationNameSize);
    style->setAnimationIterationCount(output.m_iterationCount,
                                      animationNameSize);
    style->setAnimationDelay(CSSTime(output.m_startDelay), animationNameSize);
    style->setAnimationDirection(output.m_direction, animationNameSize);
    style->setAnimationDuration(CSSTime(output.m_iterationDuration),
                                animationNameSize);
    style->setAnimationFillMode(output.m_fill, animationNameSize);
    style->setAnimationTimingFunction(output.m_easing, animationNameSize);
    // TODO: handle items like m_endDelay and m_iterationStart

    return true;
}

bool TimingOptions::setIterationStart(Element* element,
                                      KeyframeAnimationOptions& options,
                                      TimingOutput& output)
{
    if (std::isnan(options.iterationStart()) || options.iterationStart() < 0) {
        throw new DOMException(element->executionContext(),
                               DOMException::Code::INVALID_NODE_TYPE_ERR,
                               "iterationStart must be non-negative.");
        return false;
    }

    output.m_iterationStart = options.iterationStart();
    return true;
}
bool TimingOptions::setIterationCount(Element* element,
                                      KeyframeAnimationOptions& options,
                                      TimingOutput& output)
{
    if (std::isnan(options.iterations()) || options.iterations() < 0) {
        throw new DOMException(element->executionContext(),
                               DOMException::Code::INVALID_NODE_TYPE_ERR,
                               "iterationCount must be non-negative.");
        return false;
    }

    output.m_iterationCount = options.iterations();
    return true;
}

bool TimingOptions::setIterationDuration(Element* element,
                                         KeyframeAnimationOptions& options,
                                         TimingOutput& output)
{
    static const char* errMsg = "duration must be non-negative or auto.";

    // TODO : handle this as restricted double
    // if (options.duration().isUnrestricted())
    {
        // it represents the number of milliseconds.
        double d = options.duration();
        if (std::isnan(d) || d < 0) {
            throw new DOMException(element->executionContext(),
                                   DOMException::Code::INVALID_NODE_TYPE_ERR,
                                   errMsg);
            return false;
        }
        output.m_iterationDuration = d;
        return true;
    }

    // TODO : handle this as DOMString
    /*if (iteration_duration.GetAsString() != "auto") {
        throw new DOMException(element->executionContext(),
    DOMException::Code::INVALID_NODE_TYPE_ERR, errMsg);
        return false;
    }
    */

    output.m_iterationDuration = 0.0;
    return true;
}

void TimingOptions::setDirection(TimingOutput& output,
                                 KeyframeAnimationOptions& options)
{
    if (options.direction()->equals("reverse")) {
        output.m_direction =
            AnimationDirectionValue::AnimationDirectionReverseValue;
    } else if (options.direction()->equals("alternate")) {
        output.m_direction =
            AnimationDirectionValue::AnimationDirectionAlternateValue;
    } else if (options.direction()->equals("alternate-reverse")) {
        output.m_direction =
            AnimationDirectionValue::AnimationDirectionAlternateReverseValue;
    } else {
        output.m_direction =
            AnimationDirectionValue::AnimationDirectionNormalValue;
    }
}

void TimingOptions::setFillMode(TimingOutput& output,
                                KeyframeAnimationOptions& options)
{
    if (options.fill()->equals("none")) {
        output.m_fill = AnimationFillModeValue::AnimationFillModeNoneValue;
    } else if (options.fill()->equals("forwards")) {
        output.m_fill = AnimationFillModeValue::AnimationFillModeForwardsValue;
    } else if (options.fill()->equals("backwards")) {
        output.m_fill = AnimationFillModeValue::AnimationFillModeBackwardsValue;
    } else if (options.fill()->equals("both")) {
        output.m_fill = AnimationFillModeValue::AnimationFillModeBothValue;
    } else {
        output.m_fill = AnimationFillModeValue::AnimationFillModeNoneValue;
    }
}

bool TimingOptions::setTimingFunction(Element* element,
                                      KeyframeAnimationOptions& options,
                                      TimingOutput& output)
{
    CSSTokenVector layers;
    if (!CSSPropertyParser::parseLayers(
            options.easing()->toUTF8NonGCString().c_str(),
            options.easing()->length(), layers)) {
        return false;
    }

    if (layers.size() != 1) {
        return false;
    }

    CSSStyleValuePair value;
    CSSTokenVector tokens;
    CSSStyleDeclaration::tokenizeCSSValue(tokens, layers[0].data(),
                                          layers[0].length());
    if (!value.updateValueLayerAnimationTimingFunction(tokens)) {
        return false;
    }

    if (value.valueKind() ==
        CSSStyleValuePair::ValueKind::TimingFunctionValueKind) {
        output.m_easing =
            ComputedStyle::knownTimingFunction(value.timingFunctionValue());
    } else if (value.valueKind() ==
               CSSStyleValuePair::ValueKind::TimingFunctionPointerKind) {
        output.m_easing = value.timingFunctionPointerValue();
    }
    return true;
}
}
