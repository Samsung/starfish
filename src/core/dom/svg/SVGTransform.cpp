/*
 * Copyright (c) 2021-present Samsung Electronics Co., Ltd
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
#include "SVGElement.h"
#include "core/dom/svg/SVGTransform.h"
#include "core/dom/DOMException.h"
#include "core/style/CSSParser.h"
#include "core/style/CalcData.h"

namespace Starfish {

SVGTransform::SVGTransform(SVGElement* sourceElement,
                           QualifiedName targetAttribute,
                           CSSTransformFunction value)
    : ScriptWrappable(this)
    , m_sourceElement(sourceElement)
    , m_targetAttribute(targetAttribute)
    , m_value(value)
    , m_readOnly(false)
{
    m_matrixObject = new DOMMatrix(m_sourceElement->executionContext());
    m_matrixComparisonTarget =
        new DOMMatrix(m_sourceElement->executionContext());

    updateMatrixByValue();
}

ScriptBindingInstance* SVGTransform::scriptBindingInstance()
{
    return m_sourceElement->scriptBindingInstance();
}

void* SVGTransform::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGTransform));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGTransform)] = { 0 };
        SVGElement::fillGCDescriptor(desc);

        GC_set_bit(desc, GC_WORD_OFFSET(SVGTransform, m_value));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGTransform, m_matrixObject));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(SVGTransform, m_matrixComparisonTarget));

        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGTransform));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

unsigned short SVGTransform::type()
{
    if (m_matrixObject->matrix() != m_matrixComparisonTarget->matrix()) {
        setMatrix(m_matrixObject);
    }

    switch (m_value.kind()) {
    case CSSTransformFunction::Kind::Matrix:
        return SVG_TRANSFORM_MATRIX;
    case CSSTransformFunction::Kind::Translate:
        return SVG_TRANSFORM_TRANSLATE;
    case CSSTransformFunction::Kind::Scale:
        return SVG_TRANSFORM_SCALE;
    case CSSTransformFunction::Kind::Rotate:
        return SVG_TRANSFORM_ROTATE;
    case CSSTransformFunction::Kind::SkewX:
        return SVG_TRANSFORM_SKEWX;
    case CSSTransformFunction::Kind::SkewY:
        return SVG_TRANSFORM_SKEWY;
    default:
        return SVG_TRANSFORM_UNKOWN;
    }
    return SVG_TRANSFORM_UNKOWN;
}

DOMMatrix* SVGTransform::matrix()
{
    if (m_matrixObject->matrix() != m_matrixComparisonTarget->matrix()) {
        setMatrix(m_matrixObject);
    }

    return m_matrixObject;
}

float SVGTransform::angle()
{
    if (m_matrixObject->matrix() != m_matrixComparisonTarget->matrix()) {
        setMatrix(m_matrixObject);
    }

    if (m_value.kind() == CSSTransformFunction::Kind::Rotate ||
        m_value.kind() == CSSTransformFunction::Kind::SkewX ||
        m_value.kind() == CSSTransformFunction::Kind::SkewY) {
        return m_value.values()->at(0).angleValue().value();
    }
    return 0;
}

void SVGTransform::setMatrix(DOMMatrixReadOnly* matrix)
{
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return;
    }

    if (m_matrixObject == nullptr) {
        m_matrixObject = new DOMMatrix(m_sourceElement->executionContext());
    }

    ValueList* values = new ValueList(Separator::SpaceSeparator);
    if (matrix->is2D()) {
        values->emplace_back(CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::Number, (float)matrix->a()));
        values->emplace_back(CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::Number, (float)matrix->b()));
        values->emplace_back(CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::Number, (float)matrix->c()));
        values->emplace_back(CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::Number, (float)matrix->d()));
        values->emplace_back(CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::Number, (float)matrix->e()));
        values->emplace_back(CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::Number, (float)matrix->f()));
        m_value =
            CSSTransformFunction(CSSTransformFunction::Kind::Matrix, values);
    } else {
        values->emplace_back(CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::Number, (float)matrix->m11()));
        values->emplace_back(CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::Number, (float)matrix->m12()));
        values->emplace_back(CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::Number, (float)matrix->m13()));
        values->emplace_back(CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::Number, (float)matrix->m14()));
        values->emplace_back(CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::Number, (float)matrix->m21()));
        values->emplace_back(CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::Number, (float)matrix->m22()));
        values->emplace_back(CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::Number, (float)matrix->m23()));
        values->emplace_back(CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::Number, (float)matrix->m24()));
        values->emplace_back(CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::Number, (float)matrix->m31()));
        values->emplace_back(CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::Number, (float)matrix->m32()));
        values->emplace_back(CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::Number, (float)matrix->m33()));
        values->emplace_back(CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::Number, (float)matrix->m34()));
        values->emplace_back(CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::Number, (float)matrix->m41()));
        values->emplace_back(CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::Number, (float)matrix->m42()));
        values->emplace_back(CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::Number, (float)matrix->m43()));
        values->emplace_back(CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::Number, (float)matrix->m44()));
        m_value =
            CSSTransformFunction(CSSTransformFunction::Kind::Matrix3D, values);
    }
    updateMatrixByValue();

    // need to attribute update
    // It will be implemented with implementing SVGTransformList
}

void SVGTransform::setTranslate(float tx, float ty)
{
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return;
    }

    if (std::isnan(tx) || std::isinf(tx) || std::isnan(ty) || std::isinf(ty)) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "The provided float value is non-finite");
    }

    if (m_matrixObject == nullptr) {
        m_matrixObject = new DOMMatrix(m_sourceElement->executionContext());
    }

    ValueList* values = new ValueList(Separator::SpaceSeparator);
    values->emplace_back(
        CSSStyleValuePair(CSSStyleValuePair::Length, CSSLength(tx)));
    values->emplace_back(
        CSSStyleValuePair(CSSStyleValuePair::Length, CSSLength(ty)));
    m_value =
        CSSTransformFunction(CSSTransformFunction::Kind::Translate, values);

    updateMatrixByValue();

    // need to update attribute
    // It will be implemented with implementing SVGTransformList
}

void SVGTransform::setScale(float sx, float sy)
{
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return;
    }

    if (std::isnan(sx) || std::isinf(sx) || std::isnan(sy) || std::isinf(sy)) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "The provided float value is non-finite");
    }

    if (m_matrixObject == nullptr) {
        m_matrixObject = new DOMMatrix(m_sourceElement->executionContext());
    }

    ValueList* values = new ValueList(Separator::SpaceSeparator);
    values->emplace_back(CSSStyleValuePair(CSSStyleValuePair::Number, sx));
    values->emplace_back(CSSStyleValuePair(CSSStyleValuePair::Number, sy));
    m_value = CSSTransformFunction(CSSTransformFunction::Kind::Scale, values);

    updateMatrixByValue();

    // need to update attribute
    // It will be implemented with implementing SVGTransformList
}

void SVGTransform::setRotate(float angle, float cx, float cy)
{
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return;
    }

    if (std::isnan(angle) || std::isinf(angle) || std::isnan(cx) ||
        std::isinf(cx) || std::isnan(cy) || std::isinf(cy)) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "The provided float value is non-finite");
    }

    if (m_matrixObject == nullptr) {
        m_matrixObject = new DOMMatrix(m_sourceElement->executionContext());
    }

    ValueList* values = new ValueList(Separator::SpaceSeparator);
    values->emplace_back(
        CSSStyleValuePair(CSSStyleValuePair::Angle, CSSAngle(angle)));
    if (cx != 0 && cy != 0) {
        values->emplace_back(
            CSSStyleValuePair(CSSStyleValuePair::Length, CSSLength(cx)));
        values->emplace_back(
            CSSStyleValuePair(CSSStyleValuePair::Length, CSSLength(cy)));
    }
    m_value = CSSTransformFunction(CSSTransformFunction::Kind::Rotate, values);

    updateMatrixByValue();

    // need to update attribute
    // It will be implemented with implementing SVGTransformList
}

void SVGTransform::setSkewX(float angle)
{
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return;
    }

    if (std::isnan(angle) || std::isinf(angle)) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "The provided float value is non-finite");
    }

    if (m_matrixObject == nullptr) {
        m_matrixObject = new DOMMatrix(m_sourceElement->executionContext());
    }

    ValueList* values = new ValueList(Separator::SpaceSeparator);
    values->emplace_back(
        CSSStyleValuePair(CSSStyleValuePair::Angle, CSSAngle(angle)));
    m_value = CSSTransformFunction(CSSTransformFunction::Kind::SkewX, values);

    updateMatrixByValue();

    // need to update attribute
    // It will be implemented with implementing SVGTransformList
}

void SVGTransform::setSkewY(float angle)
{
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return;
    }

    if (std::isnan(angle) || std::isinf(angle)) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "The provided float value is non-finite");
    }

    if (m_matrixObject == nullptr) {
        m_matrixObject = new DOMMatrix(m_sourceElement->executionContext());
    }

    ValueList* values = new ValueList(Separator::SpaceSeparator);
    values->emplace_back(
        CSSStyleValuePair(CSSStyleValuePair::Angle, CSSAngle(angle)));
    m_value = CSSTransformFunction(CSSTransformFunction::Kind::SkewY, values);

    updateMatrixByValue();

    // need to update attribute
    // It will be implemented with implementing SVGTransformList
}

bool SVGTransform::isReadOnly()
{
    return m_readOnly;
}

void SVGTransform::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
}

void SVGTransform::updateMatrixByValue()
{
    if (m_value.kind() == CSSTransformFunction::Matrix) {
        if (m_value.values() == nullptr) {
            m_matrixObject->setMatrix(SkMatrix44::I());
        } else {
            m_matrixObject->set2DMatrix(m_value.values()->at(0).numberValue(),
                                        m_value.values()->at(1).numberValue(),
                                        m_value.values()->at(2).numberValue(),
                                        m_value.values()->at(3).numberValue(),
                                        m_value.values()->at(4).numberValue(),
                                        m_value.values()->at(5).numberValue());
        }
        m_matrixComparisonTarget->setMatrix(m_matrixObject->matrix());
        m_matrixComparisonTarget->setIs2D(m_matrixObject->is2D());
        return;
    } else if (m_value.kind() == CSSTransformFunction::Matrix3D) {
        m_matrixObject->set3DMatrix(m_value.values()->at(0).numberValue(),
                                    m_value.values()->at(1).numberValue(),
                                    m_value.values()->at(2).numberValue(),
                                    m_value.values()->at(3).numberValue(),
                                    m_value.values()->at(4).numberValue(),
                                    m_value.values()->at(5).numberValue(),
                                    m_value.values()->at(6).numberValue(),
                                    m_value.values()->at(7).numberValue(),
                                    m_value.values()->at(8).numberValue(),
                                    m_value.values()->at(9).numberValue(),
                                    m_value.values()->at(10).numberValue(),
                                    m_value.values()->at(11).numberValue(),
                                    m_value.values()->at(12).numberValue(),
                                    m_value.values()->at(13).numberValue(),
                                    m_value.values()->at(14).numberValue(),
                                    m_value.values()->at(15).numberValue());
        m_matrixComparisonTarget->setMatrix(m_matrixObject->matrix());
        m_matrixComparisonTarget->setIs2D(m_matrixObject->is2D());
        return;
    } else if (m_value.kind() == CSSTransformFunction::Translate) {
        m_matrixObject->setMatrix(SkMatrix44::I());
        m_matrixObject->translateSelf(
            m_value.values()->at(0).lengthValue().fixed(),
            m_value.values()->at(1).lengthValue().fixed());
        m_matrixComparisonTarget->setMatrix(m_matrixObject->matrix());
        return;
    } else if (m_value.kind() == CSSTransformFunction::Scale) {
        m_matrixObject->setMatrix(SkMatrix44::I());
        m_matrixObject->scaleSelf(m_value.values()->at(0).numberValue(),
                                  m_value.values()->at(1).numberValue());
        m_matrixComparisonTarget->setMatrix(m_matrixObject->matrix());
        return;
    } else if (m_value.kind() == CSSTransformFunction::Rotate) {
        m_matrixObject->setMatrix(SkMatrix44::I());
        if (m_value.values()->size() != 1) {
            m_matrixObject->translateSelf(
                -m_value.values()->at(1).numberValue(),
                -m_value.values()->at(2).numberValue());
            m_matrixObject->rotateSelf(0, 0,
                                       m_value.values()->at(0).numberValue());
            m_matrixObject->translateSelf(
                m_value.values()->at(1).numberValue(),
                m_value.values()->at(2).numberValue());
        } else {
            m_matrixObject->rotateSelf(0, 0,
                                       m_value.values()->at(0).numberValue());
        }
        m_matrixComparisonTarget->setMatrix(m_matrixObject->matrix());
        return;
    } else if (m_value.kind() == CSSTransformFunction::SkewX) {
        m_matrixObject->setMatrix(SkMatrix44::I());
        m_matrixObject->skewXSelf(m_value.values()->at(0).angleValue().value());
        m_matrixComparisonTarget->setMatrix(m_matrixObject->matrix());
        return;
    } else if (m_value.kind() == CSSTransformFunction::SkewY) {
        m_matrixObject->setMatrix(SkMatrix44::I());
        m_matrixObject->skewYSelf(m_value.values()->at(0).angleValue().value());
        m_matrixComparisonTarget->setMatrix(m_matrixObject->matrix());
        return;
    }
    STARFISH_ASSERT_NOT_REACHED();
}

} // namespace Starfish
