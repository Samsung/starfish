/*
 * Copyright (c) 2025-present Samsung Electronics Co., Ltd
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

#include "SVGAnimateTransformElement.h"

#include "Starfish.h"
#include "StaticStrings.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/dom/Document.h"
#include "core/style/CSSProperty.h"

namespace Starfish {

SVGAnimateTransformElement::SVGAnimateTransformElement(
    Document* document, const QualifiedName& qname)
    : SVGAnimationElement(document, qname)
{
}

void* SVGAnimateTransformElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGAnimateTransformElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGAnimateTransformElement)] = { 0 };
        SVGAnimateTransformElement::fillGCDescriptor(desc);
        descr =
            GC_make_descriptor(desc, GC_WORD_LEN(SVGAnimateTransformElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGAnimateTransformElement::didAttributeChanged(QualifiedName name,
                                                     Optional<String*> old,
                                                     String* value,
                                                     bool attributeCreated,
                                                     bool attributeRemoved)
{
    SVGAnimationElement::didAttributeChanged(name, old, value, attributeCreated,
                                             attributeRemoved);

    StaticStrings* ss = starfish()->staticStrings();

    if (ss->m_type == name) {
        TransformType type;
        if (parseType(value, type)) {
            if (!m_type.hasValue() || m_type.value() != type) {
                m_type = type;
            }
        } else {
            // TODO: Use default value 'translate' when type is not specified.
            STARFISH_UNIMPLEMENTED();
        }
    }

    if (m_attributeName.hasValue() && m_type.hasValue()) {
        if (ss->m_from == name) {
            CSSStyleValuePair from;
            if (parseFromTo(m_attributeName.value(), value, from)) {
                if (!m_from.hasValue() || m_from.value() != from) {
                    m_from = from;
                }
            }
        } else if (ss->m_to == name) {
            CSSStyleValuePair to;
            if (parseFromTo(m_attributeName.value(), value, to)) {
                if (!m_to.hasValue() || m_to.value() != to) {
                    m_to = to;
                }
            }
        } else if (ss->m_values == name) {
            GCVector<CSSStyleValuePair> values;
            if (parseValues(m_attributeName.value(), value, values)) {
                if (!m_values.hasValue() ||
                    m_values.value().size() != values.size() ||
                    !std::equal(m_values.value().begin(),
                                m_values.value().end(), values.begin())) {
                    m_values = std::move(values);
                }
            }
        }
    }
}

void SVGAnimateTransformElement::beginElementAt(float offset)
{
    if (!m_attributeName.hasValue()) {
        STARFISH_UNIMPLEMENTED("Handle invalid animation name.");
        return;
    }

    if (!m_type.hasValue()) {
        STARFISH_UNIMPLEMENTED("Handle invalid type.");
        return;
    }

    // TODO: Support values attribute.
    beginElementAtInternal(offset, m_attributeName.value(), m_from, m_to,
                           nullptr);
}

bool SVGAnimateTransformElement::parseType(const String* typeValue,
                                           TransformType& type)
{
    if (typeValue->equals("translate")) {
        type = TransformType::Translate;
        return true;
    } else if (typeValue->equals("scale")) {
        type = TransformType::Scale;
        return true;
    } else if (typeValue->equals("rotate")) {
        type = TransformType::Rotate;
        return true;
    } else if (typeValue->equals("skewX")) {
        type = TransformType::SkewX;
        return true;
    } else if (typeValue->equals("skewY")) {
        type = TransformType::SkewY;
        return true;
    }
    return false;
}

bool SVGAnimateTransformElement::parseFromTo(CSSStyleValuePair::KeyKind keyKind,
                                             const String* value,
                                             CSSStyleValuePair& output)
{
    String* transformValue = nullptr;
    if (!toCSSTransformValue(m_type.value(), const_cast<String*>(value),
                             &transformValue)) {
        return false;
    }

    CSSStyleValuePair temp;
    bool ret = transformValue->peekUTF8Buffer(
        [](const char* buffer, size_t len, void* data) -> size_t {
            CSSStyleValuePair* pair = static_cast<CSSStyleValuePair*>(data);
            CSSTokenVector tokens;
            CSSStyleDeclaration::tokenizeCSSValue(tokens, buffer, len);
            if (!pair->updateValueTransform(tokens, true,
                                            Separator::SpaceSeparator)) {
                return 0;
            }
            pair->setKeyKind(CSSStyleValuePair::KeyKind::Transform);
            return 1;
        },
        &temp);

    if (ret) {
        output = temp;
    }
    return ret;
}

bool SVGAnimateTransformElement::toCSSTransformValue(const TransformType type,
                                                     String* nubmer,
                                                     String** transformValue)
{
    StaticStrings* ss = starfish()->staticStrings();
    StringBuilder builder;
    switch (type) {
    case TransformType::Translate:
        builder.appendString("translate(");
        builder.appendString(nubmer->trim());
        builder.appendChar(')');
        *transformValue = builder.finalize();
        return true;
    case TransformType::Scale:
        builder.appendString("scale(");
        builder.appendString(nubmer->trim());
        builder.appendChar(')');
        *transformValue = builder.finalize();
        return true;
    case TransformType::Rotate: {
        // <rotate-angle> [<cx> <cy>].
        GCVector<StringView> tokens;
        StringUtils::wordTokenizer(nubmer, tokens);
        if (tokens.size() != 1 && tokens.size() != 3) {
            return false;
        }

        builder.appendString("rotate(");
        for (size_t i = 0; i < tokens.size(); i++) {
            if (i != 0) {
                builder.appendChar(' ');
            }
            builder.appendString(tokens[i]);
            if (i == 0) {
                builder.appendString("deg");
            }
        }
        builder.appendChar(')');
        *transformValue = builder.finalize();
    }
        return true;
    case TransformType::SkewX:
        builder.appendString("skewX(");
        builder.appendString(nubmer->trim());
        builder.appendString("deg)");
        *transformValue = builder.finalize();
        return true;
    case TransformType::SkewY:
        builder.appendString("skewX(");
        builder.appendString(nubmer->trim());
        builder.appendString("deg)");
        *transformValue = builder.finalize();
        return true;
    }
    return false;
}

} // namespace Starfish
