/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "SVGLength.h"
#include "SVGElement.h"
#include "core/style/ComputedStyle.h"
#include "core/style/CalcData.h"
#include "core/layout/FrameBox.h"
#include "core/layout/svg/FrameSVGSVGBox.h"
#include "core/page/BrowsingContext.h"
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"

namespace StarFish {

SVGLength::SVGLength(SVGElement* sourceElement, QualifiedName targetAttribute)
    : ScriptWrappable(this)
    , m_sourceElement(sourceElement)
    , m_targetAttribute(targetAttribute)
{
}

ScriptBindingInstance* SVGLength::scriptBindingInstance()
{
    return m_sourceElement->scriptBindingInstance();
}

unsigned short SVGLength::unitType()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return 1;
}

static Nullable<Length> valueToLength(CSSStyleValuePair::ValueKind kind,
                                      CSSStyleValuePair::ValueData data)
{
    if (kind == CSSStyleValuePair::ValueKind::Auto) {
        return Length();
    } else if (kind == CSSStyleValuePair::ValueKind::Length) {
        return data.m_length.toLength();
    } else if (kind == CSSStyleValuePair::ValueKind::Percentage) {
        return Length(Length::Percent, data.m_floatValue);
    } else if (kind == CSSStyleValuePair::ValueKind::Number) {
        return Length(Length::Fixed, data.m_floatValue);
    } else if (kind == CSSStyleValuePair::ValueKind::CalcValueKind) {
        CalcValueType type = data.m_calc->type();
        if (type.isLength() || type.isPercentage()) {
            return Length(data.m_calc);
        } else {
            return Nullable<Length>();
        }
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return Nullable<Length>();
    }
}

float SVGLength::value()
{
    m_sourceElement->document()->browsingContext()->layoutIfNeeds();

    String* attrValue = m_sourceElement->getAttributeOrEmpty(m_targetAttribute);
    Length len;
    if (attrValue->length()) {
        auto s = attrValue->toUTF8NonGCString();
        CSSStyleValuePair pair;
        if (CSSPropertyParser::parseLength(
                s.data(), CSSPropertyParser::AllowPercent |
                              CSSPropertyParser::AllowWithoutUnit,
                &pair)) {
            Nullable<Length> value =
                valueToLength(pair.valueKind(), pair.value());
            if (value.hasValue()) {
                len = value.getValue();
            }
        }
    }

    if (len.isSpecified()) {
        FrameBox* cb =
            m_sourceElement->frame()
                ? m_sourceElement->frame()->layoutParent()->asFrameBox()
                : nullptr;
        FrameBox* svgBox = nullptr;
        if (cb) {
            svgBox = cb;
            while (true) {
                if (svgBox->isFrameReplaced() &&
                    svgBox->asFrameReplaced()->isFrameSVGSVGBox()) {
                    break;
                }
                svgBox = svgBox->layoutParent()->asFrameBox();
            }
        }

        LayoutUnit result = len.specifiedValue(
            cb ? (float)cb->contentWidth() : 0.f, m_sourceElement);
        if (svgBox) {
            result = result * ((FrameSVGSVGBox*)svgBox)->svgScale();
        }
        return result;
    }
    return 0;
}

void SVGLength::setValue(float v)
{
    if (std::isnan(v) || std::isinf(v)) {
        throw new DOMException(m_sourceElement->document(),
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "The provided float value is non-finite");
    }

    m_sourceElement->setAttribute(m_targetAttribute, String::fromFloat(v));
}
}
