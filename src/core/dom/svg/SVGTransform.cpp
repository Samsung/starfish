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
                           QualifiedName targetAttribute)
    : ScriptWrappable(this)
    , m_sourceElement(sourceElement)
    , m_targetAttribute(targetAttribute)
    , m_value(nullptr)
    , m_matrixObject(nullptr)
    , m_matrixComparisonTarget(nullptr)
    , m_readOnly(false)
{
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
    if (m_matrixObject == nullptr) {
        initTransform();
    } else if (m_matrixObject->matrix() != m_matrixComparisonTarget->matrix()) {
        setMatrix(m_matrixObject);
    }

    if (m_value != nullptr) {
        switch (m_value->transformValue()->at(0).kind()) {
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
    }
    return SVG_TRANSFORM_UNKOWN;
}

DOMMatrix* SVGTransform::matrix()
{
    if (m_matrixObject == nullptr) {
        initTransform();
    } else if (m_matrixObject->matrix() != m_matrixComparisonTarget->matrix()) {
        setMatrix(m_matrixObject);
    }

    return m_matrixObject;
}

float SVGTransform::angle()
{
    if (m_matrixObject == nullptr) {
        initTransform();
    } else if (m_matrixObject->matrix() != m_matrixComparisonTarget->matrix()) {
        setMatrix(m_matrixObject);
    }

    CSSTransformFunction f = m_value->transformValue()->at(0);

    if (f.kind() == CSSTransformFunction::Kind::Rotate ||
        f.kind() == CSSTransformFunction::Kind::SkewX ||
        f.kind() == CSSTransformFunction::Kind::SkewY) {
        return f.values()->at(0).angleValue().value();
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

    auto str = matrix->toString()->toUTF8NonGCString();
    CSSTokenVector tokens;
    CSSStyleDeclaration::tokenizeCSSValue(tokens, str.data(), str.length());

    m_value = new CSSStyleValuePair;
    m_value->updateValueTransform(tokens, true, Separator::SpaceSeparator);

    m_matrixObject->setIs2D(matrix->is2D());
    m_matrixObject->setMatrix(matrix->matrix());
    m_matrixComparisonTarget->setMatrix(m_matrixObject->matrix());

    if (m_targetAttribute.localName()->length()) {
        m_sourceElement->setAttribute(m_targetAttribute, m_value->toString());
    }
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

    m_matrixObject->setMatrix(SkMatrix44::I());
    m_matrixObject->translateSelf(tx, ty);
    m_matrixComparisonTarget->setMatrix(m_matrixObject->matrix());

    CSSTransformFunctions* transformations = new CSSTransformFunctions();
    m_value = new CSSStyleValuePair;
    m_value->setTransformFunctionsValue(transformations);

    ValueList* values = new ValueList(Separator::SpaceSeparator);
    CSSStyleValuePair ret;
    ret.setLengthValue(tx);
    values->emplace_back(ret);
    ret.setLengthValue(ty);
    values->emplace_back(ret);
    m_value->transformValue()->emplace_back(
        CSSTransformFunction::Kind::Translate, values);

    if (m_targetAttribute.localName()->length()) {
        m_sourceElement->setAttribute(m_targetAttribute, m_value->toString());
    }
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

    m_matrixObject->setMatrix(SkMatrix44::I());
    m_matrixObject->scaleSelf(sx, sy);
    m_matrixComparisonTarget->setMatrix(m_matrixObject->matrix());

    CSSTransformFunctions* transformations = new CSSTransformFunctions();
    m_value = new CSSStyleValuePair;
    m_value->setTransformFunctionsValue(transformations);

    ValueList* values = new ValueList(Separator::SpaceSeparator);
    CSSStyleValuePair ret;
    ret.setNumberValue(sx);
    values->emplace_back(ret);
    ret.setNumberValue(sy);
    values->emplace_back(ret);
    m_value->transformValue()->emplace_back(CSSTransformFunction::Kind::Scale,
                                            values);

    if (m_targetAttribute.localName()->length()) {
        m_sourceElement->setAttribute(m_targetAttribute, m_value->toString());
    }
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

    m_matrixObject->setMatrix(SkMatrix44::I());

    if (cx != 0 && cy != 0) {
        m_matrixObject->translateSelf(-cx, -cy);
        m_matrixObject->rotateSelf(0, 0, angle);
        m_matrixObject->translateSelf(cx, cy);
    } else {
        m_matrixObject->rotateSelf(0, 0, angle);
    }
    m_matrixComparisonTarget->setMatrix(m_matrixObject->matrix());

    CSSTransformFunctions* transformations = new CSSTransformFunctions();
    m_value = new CSSStyleValuePair;
    m_value->setTransformFunctionsValue(transformations);

    ValueList* values = new ValueList(Separator::SpaceSeparator);
    CSSStyleValuePair ret;
    ret.setAngleValue(angle);
    values->emplace_back(ret);
    if (cx != 0 && cy != 0) {
        ret.setLengthValue(cx);
        values->emplace_back(ret);
        ret.setLengthValue(cy);
        values->emplace_back(ret);
    }
    m_value->transformValue()->emplace_back(CSSTransformFunction::Kind::Rotate,
                                            values);

    if (m_targetAttribute.localName()->length()) {
        m_sourceElement->setAttribute(m_targetAttribute, m_value->toString());
    }
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

    m_matrixObject->setMatrix(SkMatrix44::I());
    m_matrixObject->skewXSelf(angle);
    m_matrixComparisonTarget->setMatrix(m_matrixObject->matrix());

    CSSTransformFunctions* transformations = new CSSTransformFunctions();
    m_value = new CSSStyleValuePair;
    m_value->setTransformFunctionsValue(transformations);

    ValueList* values = new ValueList(Separator::SpaceSeparator);
    CSSStyleValuePair ret;
    ret.setAngleValue(angle);
    values->emplace_back(ret);
    m_value->transformValue()->emplace_back(CSSTransformFunction::Kind::SkewX,
                                            values);

    if (m_targetAttribute.localName()->length()) {
        m_sourceElement->setAttribute(m_targetAttribute, m_value->toString());
    }
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

    m_matrixObject->setMatrix(SkMatrix44::I());
    m_matrixObject->skewYSelf(angle);
    m_matrixComparisonTarget->setMatrix(m_matrixObject->matrix());

    CSSTransformFunctions* transformations = new CSSTransformFunctions();
    m_value = new CSSStyleValuePair;
    m_value->setTransformFunctionsValue(transformations);

    ValueList* values = new ValueList(Separator::SpaceSeparator);
    CSSStyleValuePair ret;
    ret.setAngleValue(angle);
    values->emplace_back(ret);
    m_value->transformValue()->emplace_back(CSSTransformFunction::Kind::SkewY,
                                            values);

    if (m_targetAttribute.localName()->length()) {
        m_sourceElement->setAttribute(m_targetAttribute, m_value->toString());
    }
}

void SVGTransform::initTransform()
{
    m_matrixObject = new DOMMatrix(m_sourceElement->executionContext());
    m_matrixComparisonTarget =
        new DOMMatrix(m_sourceElement->executionContext());
    if (m_targetAttribute != AtomicString::emptyAtomicString()) {
        String* attrValue =
            m_sourceElement->getAttributeOrEmpty(m_targetAttribute);

        if (attrValue->length()) {
            auto str = attrValue->toUTF8NonGCString();
            CSSTokenVector tokens;
            CSSStyleDeclaration::tokenizeCSSValue(tokens, str.data(),
                                                  str.length());

            m_value = new CSSStyleValuePair;
            m_value->updateValueTransform(tokens, true,
                                          Separator::SpaceSeparator);

            CSSTransformFunction f = m_value->transformValue()->at(0);

            if (f.kind() == CSSTransformFunction::Matrix) {
                m_matrixObject->set2DMatrix(f.values()->at(0).numberValue(),
                                            f.values()->at(1).numberValue(),
                                            f.values()->at(2).numberValue(),
                                            f.values()->at(3).numberValue(),
                                            f.values()->at(4).numberValue(),
                                            f.values()->at(5).numberValue());
            } else if (f.kind() == CSSTransformFunction::Translate) {
                m_matrixObject->setMatrix(SkMatrix44::I());
                m_matrixObject->translateSelf(f.values()->at(0).numberValue(),
                                              f.values()->at(1).numberValue());
            } else if (f.kind() == CSSTransformFunction::Scale) {
                m_matrixObject->setMatrix(SkMatrix44::I());
                m_matrixObject->scaleSelf(f.values()->at(0).numberValue(),
                                          f.values()->at(1).numberValue());
            } else if (f.kind() == CSSTransformFunction::Rotate) {
                m_matrixObject->setMatrix(SkMatrix44::I());
                if (f.values()->size() != 1) {
                    m_matrixObject->translateSelf(
                        -f.values()->at(1).numberValue(),
                        -f.values()->at(2).numberValue());
                    m_matrixObject->rotateSelf(0, 0,
                                               f.values()->at(0).numberValue());
                    m_matrixObject->translateSelf(
                        f.values()->at(1).numberValue(),
                        f.values()->at(2).numberValue());
                } else {
                    m_matrixObject->rotateSelf(0, 0,
                                               f.values()->at(0).numberValue());
                }
            } else if (f.kind() == CSSTransformFunction::SkewX) {
                m_matrixObject->setMatrix(SkMatrix44::I());
                m_matrixObject->skewXSelf(f.values()->at(0).numberValue());
            } else if (f.kind() == CSSTransformFunction::SkewY) {
                m_matrixObject->setMatrix(SkMatrix44::I());
                m_matrixObject->skewYSelf(f.values()->at(0).numberValue());
            } else {
                STARFISH_ASSERT_NOT_REACHED();
            }

        } else {
            setMatrix(new DOMMatrixReadOnly(m_sourceElement->executionContext(),
                                            SkMatrix44::I(), true));
        }
    } else {
        setMatrix(new DOMMatrixReadOnly(m_sourceElement->executionContext(),
                                        SkMatrix44::I(), true));
    }
    m_matrixComparisonTarget->setMatrix(m_matrixObject->matrix());
}

bool SVGTransform::isReadOnly()
{
    return m_readOnly;
}

void SVGTransform::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
}
} // namespace Starfish
