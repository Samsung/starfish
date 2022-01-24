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
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/DOMMatrixReadOnly.h"
#include "core/dom/DOMMatrix.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"

namespace Starfish {

static bool sameValueZero(double a, double b)
{
    // https://tc39.github.io/ecma262/#sec-samevaluezero
    if (std::isnan(a) && std::isnan(b)) {
        return true;
    }
    return a == b;
}

void DOMMatrixReadOnly::validateAndFixup(ExecutionContext* executionContext,
                                         DOMMatrix2DInit& init)
{
    STARFISH_ASSERT(executionContext != nullptr);

    // https://drafts.fxtf.org/geometry/#matrix-validate-and-fixup-2d
    if (init.hasA() && init.hasM11() && !sameValueZero(init.a(), init.m11())) {
        throw new DOMException(executionContext,
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "init.a and init.m11 do not match");
    }
    if (init.hasB() && init.hasM12() && !sameValueZero(init.b(), init.m12())) {
        throw new DOMException(executionContext,
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "init.b and init.m12 do not match");
    }
    if (init.hasC() && init.hasM21() && !sameValueZero(init.c(), init.m21())) {
        throw new DOMException(executionContext,
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "init.c and init.m21 do not match");
    }
    if (init.hasD() && init.hasM22() && !sameValueZero(init.d(), init.m22())) {
        throw new DOMException(executionContext,
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "init.d and init.m22 do not match");
    }
    if (init.hasE() && init.hasM41() && !sameValueZero(init.e(), init.m41())) {
        throw new DOMException(executionContext,
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "init.e and init.m41 do not match");
    }
    if (init.hasF() && init.hasM42() && !sameValueZero(init.f(), init.m42())) {
        throw new DOMException(executionContext,
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "init.a and init.m11 do not match");
    }

    if (!init.hasM11()) {
        init.setM11(init.hasA() ? init.a() : 1);
    }

    if (!init.hasM12()) {
        init.setM12(init.hasB() ? init.b() : 0);
    }

    if (!init.hasM21()) {
        init.setM21(init.hasC() ? init.c() : 0);
    }

    if (!init.hasM22()) {
        init.setM22(init.hasD() ? init.d() : 1);
    }

    if (!init.hasM41()) {
        init.setM41(init.hasE() ? init.e() : 0);
    }

    if (!init.hasM42()) {
        init.setM42(init.hasF() ? init.f() : 0);
    }
}

void DOMMatrixReadOnly::validateAndFixup(ExecutionContext* executionContext,
                                         DOMMatrixInit& init)
{
    STARFISH_ASSERT(executionContext != nullptr);
    // https://drafts.fxtf.org/geometry/#matrix-validate-and-fixup

    DOMMatrix2DInit init_2d;
    memcpy(&init_2d, &init, sizeof(DOMMatrix2DInit));
    validateAndFixup(executionContext, init_2d);
    memcpy(&init, &init_2d, sizeof(DOMMatrix2DInit));

    if ((init.m13() != 0 || init.m14() != 0 || init.m23() != 0 ||
         init.m24() != 0 || init.m31() != 0 || init.m32() != 0 ||
         init.m34() != 0 || init.m43() != 0) ||
        (init.m13() != 1 || init.m14() != 1)) {
        if (init.hasIs2D()) {
            if (init.is2D()) {
                throw new DOMException(executionContext,
                                       DOMException::Code::SCRIPT_TYPE_ERR, "");
            }
        } else {
            init.setIs2D(false);
        }
    } else {
        if (!init.hasIs2D()) {
            init.setIs2D(true);
        }
    }
}

DOMMatrixReadOnly* DOMMatrixReadOnly::fromMatrix(
    ExecutionContext* executionContext, DOMMatrixInit& init)
{
    validateAndFixup(executionContext, init);
    DOMMatrixReadOnly* result = new DOMMatrixReadOnly(executionContext);
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

DOMMatrixReadOnly* DOMMatrixReadOnly::fromFloat32Array(
    ExecutionContext* executionContext, ScriptFloat32Array array32)
{
    ContextRef* ctx =
        executionContext->scriptBindingInstance()->scriptContext();
    DOMMatrixReadOnly* result = new DOMMatrixReadOnly(executionContext);
    Evaluator::execute(
        ctx,
        [](ExecutionStateRef* state, ExecutionContext* executionContext,
           ScriptFloat32Array array32, DOMMatrixReadOnly* result) -> ValueRef* {
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

DOMMatrixReadOnly* DOMMatrixReadOnly::fromFloat64Array(
    ExecutionContext* executionContext, ScriptFloat64Array array64)
{
    DOMMatrixReadOnly* result = new DOMMatrixReadOnly(executionContext);
    ContextRef* ctx =
        executionContext->scriptBindingInstance()->scriptContext();

    Evaluator::execute(
        ctx,
        [](ExecutionStateRef* state, ExecutionContext* executionContext,
           ScriptFloat64Array array64, DOMMatrixReadOnly* result) -> ValueRef* {
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

DOMMatrixReadOnly* DOMMatrixReadOnly::fromMatrix(
    ExecutionContext* executionContext)
{
    DOMMatrixInit matrix;
    return fromMatrix(executionContext, matrix);
}

DOMMatrixReadOnly::DOMMatrixReadOnly(ExecutionContext* executionContext)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_matrix(SkMatrix44::I())
    , m_is2D(true)
    , m_isValid(true)
{
    STARFISH_ASSERT(executionContext != nullptr);
}

DOMMatrixReadOnly::DOMMatrixReadOnly(ExecutionContext* executionContext,
                                     DOMStringOrSequenceOfdouble value)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_matrix(SkMatrix44::I())
    , m_is2D(true)
    , m_isValid(true)
{
    STARFISH_ASSERT(executionContext != nullptr);
    bool isValid = false;
    if (value.isDOMStringValue()) {
        CSSStyleValuePair pair;

        String* param = value.getDOMStringValue();
        // parsing string
        auto str = param->toUTF8NonGCString();
        CSSTokenVector tokens;
        CSSStyleDeclaration::tokenizeCSSValue(tokens, str.data(), str.length());
        if (pair.updateValueTransform(tokens, true)) {
            if (pair.valueKind() ==
                CSSStyleValuePair::ValueKind::TransformFunctions) {
                CSSTransformFunctions* funcs = pair.transformValue();
                for (unsigned c = 0; c < funcs->size(); c++) {
                    CSSTransformFunction f = (*funcs)[c];
                    int valueSize = f.values()->size();
                    float* dValues = ALLOCA(valueSize * sizeof(float), float);
                    for (int i = 0; i < valueSize; i++) {
                        const CSSStyleValuePair& item = (*f.values())[i];
                        if (item.valueKind() ==
                            CSSStyleValuePair::ValueKind::Number) {
                            dValues[i] = item.numberValue();
                        }
                    }
                    if (f.kind() == CSSTransformFunction::Kind::Matrix) {
                        set2DMatrix(dValues[0], dValues[1], dValues[2],
                                    dValues[3], dValues[4], dValues[5]);
                        isValid = true;
                    } else if (f.kind() ==
                               CSSTransformFunction::Kind::Matrix3D) {
                        set3DMatrix(
                            dValues[0], dValues[1], dValues[2], dValues[3],
                            dValues[4], dValues[5], dValues[6], dValues[7],
                            dValues[8], dValues[9], dValues[10], dValues[11],
                            dValues[12], dValues[13], dValues[14], dValues[15]);
                        isValid = true;
                    }
                }
            }
        } else {
            // https://drafts.fxtf.org/geometry/#ref-for-parse-a-string-into-an-abstract-matrix
            throw new DOMException(m_executionContext,
                                   DOMException::Code::SYNTAX_ERR,
                                   "Failed to parse a string into an matrix");
        }

    } else if (value.isSequenceOfdoubleValue()) {
        GCAtomicVector<double> param = value.getSequenceOfdoubleValue();
        if (param.size() == 6) {
            set2DMatrix(param[0], param[1], param[2], param[3], param[4],
                        param[5]);
            isValid = true;
        } else if (param.size() == 16) {
            set3DMatrix(param[0], param[1], param[2], param[3], param[4],
                        param[5], param[6], param[7], param[8], param[9],
                        param[10], param[11], param[12], param[13], param[14],
                        param[15]);
            isValid = true;
        }
    }
    if (!isValid) {
        throw new DOMException(m_executionContext,
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "Failed to construct DOMMatrix");
    }
}

void DOMMatrixReadOnly::set2DMatrix(double m11, double m12, double m21,
                                    double m22, double m41, double m42)
{
    m_is2D = true;
    m_matrix.setDouble(0, 0, m11);
    m_matrix.setDouble(1, 0, m12);
    m_matrix.setDouble(0, 1, m21);
    m_matrix.setDouble(1, 1, m22);
    m_matrix.setDouble(0, 3, m41);
    m_matrix.setDouble(1, 3, m42);
}

void DOMMatrixReadOnly::set3DMatrix(double m11, double m12, double m13,
                                    double m14, double m21, double m22,
                                    double m23, double m24, double m31,
                                    double m32, double m33, double m34,
                                    double m41, double m42, double m43,
                                    double m44)
{
    m_is2D = false;
    m_matrix.setDouble(0, 0, m11);
    m_matrix.setDouble(1, 0, m12);
    m_matrix.setDouble(2, 0, m13);
    m_matrix.setDouble(3, 0, m14);
    m_matrix.setDouble(0, 1, m21);
    m_matrix.setDouble(1, 1, m22);
    m_matrix.setDouble(2, 1, m23);
    m_matrix.setDouble(3, 1, m24);
    m_matrix.setDouble(0, 2, m31);
    m_matrix.setDouble(1, 2, m32);
    m_matrix.setDouble(2, 2, m33);
    m_matrix.setDouble(3, 2, m34);
    m_matrix.setDouble(0, 3, m41);
    m_matrix.setDouble(1, 3, m42);
    m_matrix.setDouble(2, 3, m43);
    m_matrix.setDouble(3, 3, m44);
}

DOMMatrixReadOnly::DOMMatrixReadOnly(ExecutionContext* executionContext,
                                     SkMatrix44 matrix, bool is2D)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_matrix(matrix)
    , m_is2D(is2D)
{
    STARFISH_ASSERT(executionContext != nullptr);
}

double DOMMatrixReadOnly::a() const
{
    return m_matrix.getDouble(0, 0);
}

double DOMMatrixReadOnly::b() const
{
    return m_matrix.getDouble(1, 0);
}

double DOMMatrixReadOnly::c() const
{
    return m_matrix.getDouble(0, 1);
}

double DOMMatrixReadOnly::d() const
{
    return m_matrix.getDouble(1, 1);
}

double DOMMatrixReadOnly::e() const
{
    return m_matrix.getDouble(0, 3);
}

double DOMMatrixReadOnly::f() const
{
    return m_matrix.getDouble(1, 3);
}

double DOMMatrixReadOnly::m11() const
{
    return m_matrix.getDouble(0, 0);
}

double DOMMatrixReadOnly::m12() const
{
    return m_matrix.getDouble(1, 0);
}

double DOMMatrixReadOnly::m13() const
{
    return m_matrix.getDouble(2, 0);
}

double DOMMatrixReadOnly::m14() const
{
    return m_matrix.getDouble(3, 0);
}

double DOMMatrixReadOnly::m21() const
{
    return m_matrix.getDouble(0, 1);
}

double DOMMatrixReadOnly::m22() const
{
    return m_matrix.getDouble(1, 1);
}

double DOMMatrixReadOnly::m23() const
{
    return m_matrix.getDouble(2, 1);
}

double DOMMatrixReadOnly::m24() const
{
    return m_matrix.getDouble(3, 1);
}

double DOMMatrixReadOnly::m31() const
{
    return m_matrix.getDouble(0, 2);
}

double DOMMatrixReadOnly::m32() const
{
    return m_matrix.getDouble(1, 2);
}

double DOMMatrixReadOnly::m33() const
{
    return m_matrix.getDouble(2, 2);
}

double DOMMatrixReadOnly::m34() const
{
    return m_matrix.getDouble(3, 2);
}

double DOMMatrixReadOnly::m41() const
{
    return m_matrix.getDouble(0, 3);
}

double DOMMatrixReadOnly::m42() const
{
    return m_matrix.getDouble(1, 3);
}

double DOMMatrixReadOnly::m43() const
{
    return m_matrix.getDouble(2, 3);
}

double DOMMatrixReadOnly::m44() const
{
    return m_matrix.getDouble(3, 3);
}

ScriptFloat32Array DOMMatrixReadOnly::toFloat32Array()
{
    const int ArraySize = 16;
    size_t byteSize = ArraySize * sizeof(float);

    ContextRef* ctx =
        executionContext()->scriptBindingInstance()->scriptContext();

    auto scriptArrayBuffer = createScriptArrayBuffer(
        executionContext()->scriptBindingInstance(), byteSize);
    auto martixArrayBuffer = createScriptValue(scriptArrayBuffer);
    auto float32Array =
        createEmptyFloat32Array(executionContext()->scriptBindingInstance());

    Evaluator::execute(
        ctx,
        [](ExecutionStateRef* state, ScriptFloat32Array float32Array,
           DOMMatrixReadOnly* self,
           ScriptValue martixArrayBuffer) -> ValueRef* {
            const int ArraySize = 16;
            size_t byteSize = ArraySize * sizeof(float);

            float32Array->setBuffer(
                martixArrayBuffer->toObject(state)->asArrayBufferObject(), 0,
                byteSize, ArraySize);

            float32Array->set(state, ValueRef::create(0),
                              ValueRef::create(self->m11()));
            float32Array->set(state, ValueRef::create(1),
                              ValueRef::create(self->m12()));
            float32Array->set(state, ValueRef::create(2),
                              ValueRef::create(self->m13()));
            float32Array->set(state, ValueRef::create(3),
                              ValueRef::create(self->m14()));
            float32Array->set(state, ValueRef::create(4),
                              ValueRef::create(self->m21()));
            float32Array->set(state, ValueRef::create(5),
                              ValueRef::create(self->m22()));
            float32Array->set(state, ValueRef::create(7),
                              ValueRef::create(self->m24()));
            float32Array->set(state, ValueRef::create(8),
                              ValueRef::create(self->m31()));
            float32Array->set(state, ValueRef::create(9),
                              ValueRef::create(self->m32()));
            float32Array->set(state, ValueRef::create(10),
                              ValueRef::create(self->m33()));
            float32Array->set(state, ValueRef::create(11),
                              ValueRef::create(self->m34()));
            float32Array->set(state, ValueRef::create(12),
                              ValueRef::create(self->m41()));
            float32Array->set(state, ValueRef::create(13),
                              ValueRef::create(self->m42()));
            float32Array->set(state, ValueRef::create(14),
                              ValueRef::create(self->m43()));
            float32Array->set(state, ValueRef::create(15),
                              ValueRef::create(self->m44()));

            return ValueRef::createUndefined();
        },
        float32Array, this, martixArrayBuffer);

    return float32Array;
}

ScriptFloat64Array DOMMatrixReadOnly::toFloat64Array()
{
    const int ArraySize = 16;
    size_t byteSize = ArraySize * sizeof(double);

    ContextRef* ctx =
        executionContext()->scriptBindingInstance()->scriptContext();
    auto scriptArrayBuffer = createScriptArrayBuffer(
        executionContext()->scriptBindingInstance(), byteSize);
    auto martixArrayBuffer = createScriptValue(scriptArrayBuffer);
    auto float64Array =
        createEmptyFloat64Array(executionContext()->scriptBindingInstance());

    Evaluator::execute(
        ctx,
        [](ExecutionStateRef* state, ScriptFloat64Array float64Array,
           DOMMatrixReadOnly* self,
           ScriptValue martixArrayBuffer) -> ValueRef* {
            const int ArraySize = 16;
            size_t byteSize = ArraySize * sizeof(double);

            float64Array->setBuffer(
                martixArrayBuffer->toObject(state)->asArrayBufferObject(), 0,
                byteSize, ArraySize);

            float64Array->set(state, ValueRef::create(0),
                              ValueRef::create(self->m11()));
            float64Array->set(state, ValueRef::create(1),
                              ValueRef::create(self->m12()));
            float64Array->set(state, ValueRef::create(2),
                              ValueRef::create(self->m13()));
            float64Array->set(state, ValueRef::create(3),
                              ValueRef::create(self->m14()));
            float64Array->set(state, ValueRef::create(4),
                              ValueRef::create(self->m21()));
            float64Array->set(state, ValueRef::create(5),
                              ValueRef::create(self->m22()));
            float64Array->set(state, ValueRef::create(7),
                              ValueRef::create(self->m24()));
            float64Array->set(state, ValueRef::create(8),
                              ValueRef::create(self->m31()));
            float64Array->set(state, ValueRef::create(9),
                              ValueRef::create(self->m32()));
            float64Array->set(state, ValueRef::create(10),
                              ValueRef::create(self->m33()));
            float64Array->set(state, ValueRef::create(11),
                              ValueRef::create(self->m34()));
            float64Array->set(state, ValueRef::create(12),
                              ValueRef::create(self->m41()));
            float64Array->set(state, ValueRef::create(13),
                              ValueRef::create(self->m42()));
            float64Array->set(state, ValueRef::create(14),
                              ValueRef::create(self->m43()));
            float64Array->set(state, ValueRef::create(15),
                              ValueRef::create(self->m44()));

            return ValueRef::createUndefined();
        },
        float64Array, this, martixArrayBuffer);

    return float64Array;
}

String* DOMMatrixReadOnly::toString()
{
    // https://drafts.fxtf.org/geometry/#dommatrixreadonly-stringification-behavior
    StringBuilder sb;
    if (is2D()) {
        sb.appendString(String::createASCIIString("matrix("));

        sb.appendString(String::fromInt(m11()));
        sb.appendString(String::createASCIIString(", "));

        sb.appendString(String::fromInt(m12()));
        sb.appendString(String::createASCIIString(", "));

        sb.appendString(String::fromInt(m21()));
        sb.appendString(String::createASCIIString(", "));

        sb.appendString(String::fromInt(m22()));
        sb.appendString(String::createASCIIString(", "));

        sb.appendString(String::fromInt(m41()));
        sb.appendString(String::createASCIIString(", "));

        sb.appendString(String::fromInt(m42()));
    } else {
        sb.appendString(String::createASCIIString("matrix3d("));

        sb.appendString(String::fromInt(m11()));
        sb.appendString(String::createASCIIString(", "));

        sb.appendString(String::fromInt(m12()));
        sb.appendString(String::createASCIIString(", "));

        sb.appendString(String::fromInt(m13()));
        sb.appendString(String::createASCIIString(", "));

        sb.appendString(String::fromInt(m14()));
        sb.appendString(String::createASCIIString(", "));

        sb.appendString(String::fromInt(m21()));
        sb.appendString(String::createASCIIString(", "));

        sb.appendString(String::fromInt(m22()));
        sb.appendString(String::createASCIIString(", "));

        sb.appendString(String::fromInt(m23()));
        sb.appendString(String::createASCIIString(", "));

        sb.appendString(String::fromInt(m24()));
        sb.appendString(String::createASCIIString(", "));

        sb.appendString(String::fromInt(m31()));
        sb.appendString(String::createASCIIString(", "));

        sb.appendString(String::fromInt(m32()));
        sb.appendString(String::createASCIIString(", "));

        sb.appendString(String::fromInt(m33()));
        sb.appendString(String::createASCIIString(", "));

        sb.appendString(String::fromInt(m34()));
        sb.appendString(String::createASCIIString(", "));

        sb.appendString(String::fromInt(m41()));
        sb.appendString(String::createASCIIString(", "));

        sb.appendString(String::fromInt(m42()));
        sb.appendString(String::createASCIIString(", "));

        sb.appendString(String::fromInt(m43()));
        sb.appendString(String::createASCIIString(", "));

        sb.appendString(String::fromInt(m44()));
    }
    sb.appendString(String::createASCIIString(")"));
    return sb.finalize();
}

DOMMatrix* DOMMatrixReadOnly::translate(double tx, double ty, double tz)
{
    return DOMMatrix::Create(this)->translateSelf(tx, ty, tz);
}

DOMMatrix* DOMMatrixReadOnly::scale(double sx)
{
    return scale(sx, sx);
}

DOMMatrix* DOMMatrixReadOnly::scale(double sx, double sy, double sz, double ox,
                                    double oy, double oz)
{
    return DOMMatrix::Create(this)->scaleSelf(sx, sy, sz, ox, oy, oz);
}

DOMMatrix* DOMMatrixReadOnly::scaleNonUniform(double sx, double sy)
{
    return DOMMatrix::Create(this)->scaleSelf(sx, sy, 1, 0, 0, 0);
}

DOMMatrix* DOMMatrixReadOnly::scale3d(double scale, double ox, double oy,
                                      double oz)
{
    return DOMMatrix::Create(this)->scale3dSelf(scale, ox, oy, oz);
}

DOMMatrix* DOMMatrixReadOnly::rotate(double rot_x)
{
    return DOMMatrix::Create(this)->rotateSelf(0, 0, rot_x);
}

DOMMatrix* DOMMatrixReadOnly::rotate(double rot_x, double rot_y)
{
    return DOMMatrix::Create(this)->rotateSelf(rot_x, rot_y, 0);
}

DOMMatrix* DOMMatrixReadOnly::rotate(double rot_x, double rot_y, double rot_z)
{
    return DOMMatrix::Create(this)->rotateSelf(rot_x, rot_y, rot_z);
}

DOMMatrix* DOMMatrixReadOnly::rotateFromVector(double x, double y)
{
    return DOMMatrix::Create(this)->rotateFromVectorSelf(x, y);
}

DOMMatrix* DOMMatrixReadOnly::rotateAxisAngle(double x, double y, double z,
                                              double angle)
{
    return DOMMatrix::Create(this)->rotateAxisAngleSelf(x, y, z, angle);
}

DOMMatrix* DOMMatrixReadOnly::skewX(double sx)
{
    return DOMMatrix::Create(this)->skewXSelf(sx);
}

DOMMatrix* DOMMatrixReadOnly::skewY(double sy)
{
    return DOMMatrix::Create(this)->skewYSelf(sy);
}

DOMMatrix* DOMMatrixReadOnly::multiply()
{
    return DOMMatrix::Create(this)->multiplySelf();
}

DOMMatrix* DOMMatrixReadOnly::multiply(DOMMatrixInit& other)
{
    return DOMMatrix::Create(this)->multiplySelf(other);
}

DOMMatrix* DOMMatrixReadOnly::flipX()
{
    DOMMatrix* result = DOMMatrix::Create(this);
    result->setM11(-result->m11());
    result->setM12(-result->m12());
    result->setM13(-result->m13());
    result->setM14(-result->m14());
    return result;
}

DOMMatrix* DOMMatrixReadOnly::flipY()
{
    DOMMatrix* result = DOMMatrix::Create(this);
    result->setM21(-result->m21());
    result->setM22(-result->m22());
    result->setM23(-result->m23());
    result->setM24(-result->m24());
    return result;
}

DOMMatrix* DOMMatrixReadOnly::inverse()
{
    return DOMMatrix::Create(this)->invertSelf();
}

ScriptBindingInstance* DOMMatrixReadOnly::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

SerializedData* DOMMatrixReadOnly::serialize(SerializingMap& memory)
{
    STARFISH_UNIMPLEMENTED();
    return nullptr;
}
void DOMMatrixReadOnly::deserialize(SerializedData* serialized,
                                    DeserializingMap& memory) const
{
    STARFISH_UNIMPLEMENTED();
}
} // namespace Starfish
