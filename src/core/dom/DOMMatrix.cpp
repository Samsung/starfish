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
#include "EscargotPublic.h"
#include "binding/ScriptBindingInstance.h"
#include "core/dom/DOMMatrix.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"

namespace Starfish {

DOMMatrix* DOMMatrix::Create(DOMMatrixReadOnly* domMatrix)
{
    STARFISH_ASSERT(domMatrix != nullptr);
    return new DOMMatrix(domMatrix->executionContext(), domMatrix->matrix(),
                         domMatrix->is2D());
}

DOMMatrix* DOMMatrix::fromFloat32Array(ExecutionContext* executionContext,
                                       ScriptFloat32Array array32)
{
    ContextRef* ctx =
        executionContext->scriptBindingInstance()->scriptContext();

    DOMMatrix* result = new DOMMatrix(executionContext);

    Evaluator::execute(
        ctx,
        [](ExecutionStateRef* state, ExecutionContext* executionContext,
           ScriptFloat32Array array32, DOMMatrix* result) -> ValueRef* {
            size_t arrayLength = array32->byteLength() / sizeof(float);

            if (arrayLength != 6 && arrayLength != 16) {
                throw new DOMException(
                    executionContext, DOMException::Code::SCRIPT_TYPE_ERR,
                    "The sequence must contain 6 or 16 elements");
            }

            if (arrayLength == 6) {
                result->set2DMatrix(
                    array32->get(state, ValueRef::create(0))->toNumber(state),
                    array32->get(state, ValueRef::create(1))->toNumber(state),
                    array32->get(state, ValueRef::create(2))->toNumber(state),
                    array32->get(state, ValueRef::create(3))->toNumber(state),
                    array32->get(state, ValueRef::create(4))->toNumber(state),
                    array32->get(state, ValueRef::create(5))->toNumber(state));
            } else {
                result->set3DMatrix(
                    array32->get(state, ValueRef::create(0))->toNumber(state),
                    array32->get(state, ValueRef::create(1))->toNumber(state),
                    array32->get(state, ValueRef::create(2))->toNumber(state),
                    array32->get(state, ValueRef::create(3))->toNumber(state),
                    array32->get(state, ValueRef::create(4))->toNumber(state),
                    array32->get(state, ValueRef::create(5))->toNumber(state),
                    array32->get(state, ValueRef::create(6))->toNumber(state),
                    array32->get(state, ValueRef::create(7))->toNumber(state),
                    array32->get(state, ValueRef::create(8))->toNumber(state),
                    array32->get(state, ValueRef::create(9))->toNumber(state),
                    array32->get(state, ValueRef::create(10))->toNumber(state),
                    array32->get(state, ValueRef::create(11))->toNumber(state),
                    array32->get(state, ValueRef::create(12))->toNumber(state),
                    array32->get(state, ValueRef::create(13))->toNumber(state),
                    array32->get(state, ValueRef::create(14))->toNumber(state),
                    array32->get(state, ValueRef::create(15))->toNumber(state));
            }

            return ValueRef::createUndefined();
        },
        executionContext, array32, result);

    return result;
}

DOMMatrix* DOMMatrix::fromFloat64Array(ExecutionContext* executionContext,
                                       ScriptFloat64Array array64)
{
    DOMMatrix* result = new DOMMatrix(executionContext);

    ContextRef* ctx =
        executionContext->scriptBindingInstance()->scriptContext();

    Evaluator::execute(
        ctx,
        [](ExecutionStateRef* state, ExecutionContext* executionContext,
           ScriptFloat64Array array64, DOMMatrix* result) -> ValueRef* {
            size_t arrayLength = array64->byteLength() / sizeof(double);
            if (arrayLength != 6 && arrayLength != 16) {
                throw new DOMException(
                    executionContext, DOMException::Code::SCRIPT_TYPE_ERR,
                    "The sequence must contain 6 or 16 elements");
            }

            if (arrayLength == 6) {
                result->set2DMatrix(
                    array64->get(state, ValueRef::create(0))->toNumber(state),
                    array64->get(state, ValueRef::create(1))->toNumber(state),
                    array64->get(state, ValueRef::create(2))->toNumber(state),
                    array64->get(state, ValueRef::create(3))->toNumber(state),
                    array64->get(state, ValueRef::create(4))->toNumber(state),
                    array64->get(state, ValueRef::create(5))->toNumber(state));
            } else {
                result->set3DMatrix(
                    array64->get(state, ValueRef::create(0))->toNumber(state),
                    array64->get(state, ValueRef::create(1))->toNumber(state),
                    array64->get(state, ValueRef::create(2))->toNumber(state),
                    array64->get(state, ValueRef::create(3))->toNumber(state),
                    array64->get(state, ValueRef::create(4))->toNumber(state),
                    array64->get(state, ValueRef::create(5))->toNumber(state),
                    array64->get(state, ValueRef::create(6))->toNumber(state),
                    array64->get(state, ValueRef::create(7))->toNumber(state),
                    array64->get(state, ValueRef::create(8))->toNumber(state),
                    array64->get(state, ValueRef::create(9))->toNumber(state),
                    array64->get(state, ValueRef::create(10))->toNumber(state),
                    array64->get(state, ValueRef::create(11))->toNumber(state),
                    array64->get(state, ValueRef::create(12))->toNumber(state),
                    array64->get(state, ValueRef::create(13))->toNumber(state),
                    array64->get(state, ValueRef::create(14))->toNumber(state),
                    array64->get(state, ValueRef::create(15))->toNumber(state));
            }

            return ValueRef::createUndefined();
        },
        executionContext, array64, result);
    return result;
}

DOMMatrix* DOMMatrix::fromMatrix(ExecutionContext* executionContext,
                                 DOMMatrixInit& init)
{
    validateAndFixup(executionContext, init);
    DOMMatrix* result = new DOMMatrix(executionContext);
    if (init.is2D()) {
        result->set2DMatrix(init.a(), init.b(), init.c(), init.d(), init.e(),
                            init.f());
    } else {
        result->set3DMatrix(init.m11(), init.m12(), init.m13(), init.m14(),
                            init.m21(), init.m22(), init.m23(), init.m24(),
                            init.m31(), init.m32(), init.m33(), init.m34(),
                            init.m41(), init.m42(), init.m43(), init.m44());
    }
    return result;
}

DOMMatrix* DOMMatrix::fromMatrix(ExecutionContext* executionContext)
{
    DOMMatrixInit matrix;
    return fromMatrix(executionContext, matrix);
}

DOMMatrix::DOMMatrix(ExecutionContext* executionContext)
    : DOMMatrixReadOnly(executionContext)
{
    STARFISH_ASSERT(executionContext != nullptr);
}

DOMMatrix::DOMMatrix(ExecutionContext* executionContext,
                     DOMStringOrSequenceOfdouble value)
    : DOMMatrixReadOnly(executionContext, value)
{
    STARFISH_ASSERT(executionContext != nullptr);
}

DOMMatrix::DOMMatrix(ExecutionContext* executionContext, SkMatrix44 matrix,
                     bool is2D)
    : DOMMatrixReadOnly(executionContext, matrix, is2D)
{
    STARFISH_ASSERT(executionContext != nullptr);
}

void DOMMatrix::setA(double value)
{
    matrix().setDouble(0, 0, value);
}

void DOMMatrix::setB(double value)
{
    matrix().setDouble(1, 0, value);
}

void DOMMatrix::setC(double value)
{
    matrix().setDouble(0, 1, value);
}

void DOMMatrix::setD(double value)
{
    matrix().setDouble(1, 1, value);
}

void DOMMatrix::setE(double value)
{
    matrix().setDouble(0, 3, value);
}

void DOMMatrix::setF(double value)
{
    matrix().setDouble(1, 3, value);
}

void DOMMatrix::setM11(double value)
{
    matrix().setDouble(0, 0, value);
}

void DOMMatrix::setM12(double value)
{
    matrix().setDouble(1, 0, value);
}

void DOMMatrix::setM13(double value)
{
    if (value != 0) {
        setIs2D(false);
    }
    matrix().setDouble(2, 0, value);
}

void DOMMatrix::setM14(double value)
{
    if (value != 0) {
        setIs2D(false);
    }
    matrix().setDouble(3, 0, value);
}

void DOMMatrix::setM21(double value)
{
    matrix().setDouble(0, 1, value);
}

void DOMMatrix::setM22(double value)
{
    matrix().setDouble(1, 1, value);
}

void DOMMatrix::setM23(double value)
{
    if (value != 0) {
        setIs2D(false);
    }
    matrix().setDouble(2, 1, value);
}

void DOMMatrix::setM24(double value)
{
    if (value != 0) {
        setIs2D(false);
    }
    matrix().setDouble(3, 1, value);
}

void DOMMatrix::setM31(double value)
{
    if (value != 0) {
        setIs2D(false);
    }
    matrix().setDouble(0, 2, value);
}

void DOMMatrix::setM32(double value)
{
    if (value != 0) {
        setIs2D(false);
    }
    matrix().setDouble(1, 2, value);
}

void DOMMatrix::setM33(double value)
{
    if (value != 1) {
        setIs2D(false);
    }
    matrix().setDouble(2, 2, value);
}

void DOMMatrix::setM34(double value)
{
    if (value != 0) {
        setIs2D(false);
    }
    matrix().setDouble(3, 2, value);
}

void DOMMatrix::setM41(double value)
{
    matrix().setDouble(0, 3, value);
}

void DOMMatrix::setM42(double value)
{
    matrix().setDouble(1, 3, value);
}

void DOMMatrix::setM43(double value)
{
    if (value != 0) {
        setIs2D(false);
    }
    matrix().setDouble(2, 3, value);
}

void DOMMatrix::setM44(double value)
{
    if (value != 1) {
        setIs2D(false);
    }
    matrix().setDouble(3, 3, value);
}

DOMMatrix* DOMMatrix::multiplySelf()
{
    DOMMatrixInit init;
    return multiplySelf(init);
}

DOMMatrix* DOMMatrix::multiplySelf(DOMMatrixInit& other)
{
    if (!other.is2D()) {
        setIs2D(false);
    }
    setMatrix(matrix() * fromMatrix(executionContext(), other)->matrix());
    return this;
}

DOMMatrix* DOMMatrix::preMultiplySelf()
{
    DOMMatrixInit init;
    return preMultiplySelf(init);
}

DOMMatrix* DOMMatrix::preMultiplySelf(DOMMatrixInit& other)
{
    if (!other.is2D()) {
        setIs2D(false);
    }
    setMatrix(fromMatrix(executionContext(), other)->matrix() * matrix());
    return this;
}

DOMMatrix* DOMMatrix::translateSelf(double tx, double ty, double tz)
{
    // https://www.w3.org/TR/geometry-1/#dom-dommatrix-translateself
    if ((tx != 0) && (ty != 0) && (tz != 0)) {
        return this;
    }

    if (tz != 0) {
        setIs2D(false);
    }

    matrix().preTranslate(tx, ty, tz);
    return this;
}

DOMMatrix* DOMMatrix::scaleSelf(double sx)
{
    return scaleSelf(sx, sx);
}

DOMMatrix* DOMMatrix::scaleSelf(double sx, double sy, double sz, double ox,
                                double oy, double oz)
{
    // https://www.w3.org/TR/geometry-1/#dom-dommatrix-scaleself
    if ((sz != 1) || (oz != 0)) {
        setIs2D(false);
    }

    if ((sx == 1) && (sy == 1) && (sz == 1)) {
        return this;
    }

    bool hasTranslation = false;
    if ((ox != 0) || (oy != 0) || (oz != 0)) {
        hasTranslation = true;
        translateSelf(ox, oy, oz);
    }
    matrix().postScale(sx, sy, sz);
    if (hasTranslation) {
        translateSelf(-ox, -oy, -oz);
    }

    return this;
}

DOMMatrix* DOMMatrix::scale3dSelf(double scale, double ox, double oy, double oz)
{
    return scaleSelf(scale, scale, scale, ox, oy, oz);
}

DOMMatrix* DOMMatrix::rotateSelf(double rot_x, double rot_y, double rot_z)
{
    // https://www.w3.org/TR/geometry-1/#dom-dommatrix-rotateself
    SkMatrix44 mat = SkMatrix44::I();

    if (rot_z != 0) {
        mat.setRotateAbout(0, 0, 1, rot_z);
    }

    if (rot_y != 0) {
        mat.setRotateAbout(0, 1, 0, rot_y);
        setIs2D(false);
    }

    if (rot_x != 0) {
        mat.setRotateAbout(1, 0, 0, rot_x);
        setIs2D(false);
    }

    setMatrix(matrix() * mat);
    return this;
}

DOMMatrix* DOMMatrix::rotateFromVectorSelf(double x, double y)
{
    rotateSelf(0, 0, atan2(y, x) * 180.0 / SK_MScalarPI);
    return this;
}

DOMMatrix* DOMMatrix::rotateAxisAngleSelf(double x, double y, double z,
                                          double angle)
{
    SkMatrix44 mat = SkMatrix44::I();

    mat.setRotateDegreesAbout(x, y, z, angle);
    if ((x != 0) || (y != 0)) {
        setIs2D(false);
    }
    setMatrix(matrix() * mat);

    return this;
}

DOMMatrix* DOMMatrix::skewXSelf(double sx)
{
    return skew(sx, 0);
}

DOMMatrix* DOMMatrix::skewYSelf(double sy)
{
    return skew(0, sy);
}

DOMMatrix* DOMMatrix::skew(double sx, double sy)
{
    SkMatrix44 mat = SkMatrix44::I();
    mat.set(0, 1, std::tan(sx * SK_MScalarPI / 180.0));
    mat.set(1, 0, std::tan(sy * SK_MScalarPI / 180.0));
    matrix().preConcat(mat);
    return this;
}

DOMMatrix* DOMMatrix::invertSelf()
{
    SkMatrix44 result;
    if (matrix().invert(&result)) {
        setMatrix(result);
    } else {
        setIs2D(false);
        makeInvalid();
    }
    return this;
}

ScriptBindingInstance* DOMMatrix::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

SerializedData* DOMMatrix::serialize(SerializingMap& memory)
{
    STARFISH_UNIMPLEMENTED();
    return nullptr;
}
void DOMMatrix::deserialize(SerializedData* serialized,
                            DeserializingMap& memory) const
{
    STARFISH_UNIMPLEMENTED();
}
} // namespace Starfish
