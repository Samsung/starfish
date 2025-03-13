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
        if (parseType(type)) {
            if (!m_type.hasValue() || m_type.value() != type) {
                m_type = type;
            }
        }
    }
}

void SVGAnimateTransformElement::beginElementAt(float offset)
{
    SVGAnimationElement::beginElementAt(offset);
}

bool SVGAnimateTransformElement::parseType(TransformType& type)
{
    Optional<String*> mabyType =
        getAttribute(starfish()->staticStrings()->m_type);
    if (!mabyType) {
        return false;
    }
    String* typeStr = mabyType.value();

    if (typeStr->equals("translate")) {
        type = TransformType::Translate;
        return true;
    } else if (typeStr->equals("scale")) {
        type = TransformType::Scale;
        return true;
    } else if (typeStr->equals("rotate")) {
        type = TransformType::Rotate;
        return true;
    } else if (typeStr->equals("skewX")) {
        type = TransformType::SkewX;
        return true;
    } else if (typeStr->equals("skewY")) {
        type = TransformType::SkewY;
        return true;
    }
    return false;
}

bool SVGAnimateTransformElement::parseFrom(CSSStyleValuePair::KeyKind keyKind,
                                           const String* fromValue,
                                           CSSStyleValuePair& from)
{
    if (!m_type.hasValue()) {
        return false;
    }
    String* transformValue = nullptr;
    if (!toCSSTransfromValue(m_type.value(), const_cast<String*>(fromValue),
                             &transformValue)) {
        return false;
    }
    return parseFromAndToInternal(CSSStyleValuePair::KeyKind::Transform,
                                  transformValue, from);
}
bool SVGAnimateTransformElement::parseTo(CSSStyleValuePair::KeyKind keyKind,
                                         const String* toValue,
                                         CSSStyleValuePair& to)
{
    if (!m_type.hasValue()) {
        return false;
    }

    String* transformValue = nullptr;
    if (!toCSSTransfromValue(m_type.value(), const_cast<String*>(toValue),
                             &transformValue)) {
        return false;
    }
    return parseFromAndToInternal(CSSStyleValuePair::KeyKind::Transform,
                                  transformValue, to);
}

bool SVGAnimateTransformElement::toCSSTransfromValue(const TransformType type,
                                                     String* nubmer,
                                                     String** transformValue)
{
    StaticStrings* ss = starfish()->staticStrings();
    StringBuilder builder;
    switch (type) {
    case TransformType::Translate:
        builder.appendString("translate(");
        builder.appendString(nubmer);
        builder.appendChar(')');
        *transformValue = builder.finalize();
        return true;
    case TransformType::Scale:
        builder.appendString("scale(");
        builder.appendString(nubmer);
        builder.appendChar(')');
        *transformValue = builder.finalize();
        return true;
    case TransformType::Rotate:
        builder.appendString("rotate(");
        builder.appendString(nubmer);
        builder.appendString("deg)");
        *transformValue = builder.finalize();
        return true;
    case TransformType::SkewX:
        builder.appendString("skewX(");
        builder.appendString(nubmer);
        builder.appendString("deg)");
        *transformValue = builder.finalize();
        return true;
    case TransformType::SkewY:
        builder.appendString("skewX(");
        builder.appendString(nubmer);
        builder.appendString("deg)");
        *transformValue = builder.finalize();
        return true;
    }
    return false;
}

} // namespace Starfish
