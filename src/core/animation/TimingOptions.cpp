/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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
#include "Starfish.h"

#include "core/animation/Animation.h"
#include "core/animation/TimingOptions.h"
#include "core/dom/DOMException.h"
#include "core/dom/Element.h"
#include "core/style/ComputedStyle.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"

namespace Starfish {

static size_t gAnimationCount = 0;

TimingFunction* TimingOptions::validate(ExecutionContext* context,
                                        const KeyframeAnimationOptions& options)
{
    if (!(options.duration() >= 0) || !(options.iterationStart() >= 0) ||
        !(options.iterations() >= 0)) {
        throw new DOMException(
            context, DOMException::SCRIPT_TYPE_ERR,
            "Duration, iterationStart and iterations must be non-negative.");
    }
    auto easing = parseEasing(options.easing());
    if (!easing) {
        throw new DOMException(context, DOMException::SCRIPT_TYPE_ERR,
                               "Invalid animation easing");
    }
    return easing.value();
}

static AnimationDirectionValue animationDirection(String* direction)
{
    if (direction->equals("reverse")) {
        return AnimationDirectionValue::Reverse;
    } else if (direction->equals("alternate")) {
        return AnimationDirectionValue::Alternate;
    } else if (direction->equals("alternate-reverse")) {
        return AnimationDirectionValue::AlternateReverse;
    } else {
        return AnimationDirectionValue::Normal;
    }
}

static AnimationFillModeValue animationFillMode(String* fill)
{
    if (fill->equals("forwards")) {
        return AnimationFillModeValue::Forwards;
    } else if (fill->equals("backwards")) {
        return AnimationFillModeValue::Backwards;
    } else if (fill->equals("both")) {
        return AnimationFillModeValue::Both;
    } else {
        return AnimationFillModeValue::None;
    }
}

void TimingOptions::apply(Element* element, KeyframeAnimationOptions& options,
                          TimingFunction* easing)
{
    ComputedStyle* style = element->style();

    AtomicString name =
        AtomicString::createAtomicString(element->starfish(), "AnimationId");
    StringBuilder id;
    id.appendString(name.string());
    id.appendString(String::fromInt64(gAnimationCount++));
    options.setId(id.finalize());

    StyleAnimationData* styleAnimationData = style->animation();
    size_t animationNameSize =
        styleAnimationData ? styleAnimationData->animationNameSize() : 0;

    style->setAnimationName(options.id(), animationNameSize);
    style->setAnimationIterationCount(options.iterations(), animationNameSize);
    style->setAnimationDelay(CSSTime(options.delay()), animationNameSize);
    style->setAnimationDirection(animationDirection(options.direction()),
                                 animationNameSize);
    style->setAnimationDuration(CSSTime(options.duration()), animationNameSize);
    style->setAnimationFillMode(animationFillMode(options.fill()),
                                animationNameSize);
    style->setAnimationTimingFunction(easing, animationNameSize);
    // TODO: apply endDelay and iterationStart to playback.
}

Optional<TimingFunction*> TimingOptions::parseEasing(String* easing)
{
    auto utf8 = easing->toUTF8NonGCString();
    CSSTokenVector layers;
    if (!CSSPropertyParser::parseLayers(utf8.data(), utf8.size(), layers)) {
        return nullptr;
    }

    if (layers.size() != 1) {
        return nullptr;
    }

    CSSStyleValuePair value;
    CSSTokenVector tokens;
    CSSStyleDeclaration::tokenizeCSSValue(tokens, layers[0].data(),
                                          layers[0].length());
    if (!value.updateValueLayerAnimationTimingFunction(tokens)) {
        return nullptr;
    }

    if (value.valueKind() ==
        CSSStyleValuePair::ValueKind::TimingFunctionValueKind) {
        return ComputedStyle::knownTimingFunction(value.timingFunctionValue());
    }
    return value.timingFunctionPointerValue();
}
} // namespace Starfish
