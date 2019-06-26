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

#ifndef __StarfishDOMMatrixReadOnly__
#define __StarfishDOMMatrixReadOnly__

#include "binding/ScriptWrappable.h"
#include "core/page/Serializer.h"
#include <SkMatrix44.h>
#include "binding/DOMStringOrSequenceUnion.h"
#include "core/dom/DOMMatrix2DInit.h"
#include "core/dom/DOMMatrixInit.h"
#include "DOMExceptionOr.h"

namespace Starfish {
class DOMMatrix;
class DOMMatrixReadOnly : public ScriptWrappable, public Serializable {
public:
    static void validateAndFixup(ExecutionContext* executionContext,
                                 DOMMatrix2DInit& init);

    static void validateAndFixup(ExecutionContext* executionContext,
                                 DOMMatrixInit& init);

    static DOMMatrixReadOnly* fromMatrix(ExecutionContext* executionContext,
                                         DOMMatrixInit& init);
    static DOMMatrixReadOnly* fromMatrix(ExecutionContext* executionContext);

    static DOMMatrixReadOnly* fromFloat32Array(
        ExecutionContext* executionContext, ScriptFloat32Array array32);

    static DOMMatrixReadOnly* fromFloat64Array(
        ExecutionContext* executionContext, ScriptFloat64Array array64);

    DOMMatrixReadOnly(ExecutionContext* executionContext);
    DOMMatrixReadOnly(ExecutionContext* executionContext,
                      DOMStringOrSequence value);
    DOMMatrixReadOnly(ExecutionContext* executionContext, SkMatrix44 matrix,
                      bool is2D);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(DOMMatrixReadOnly)

    double a() const;
    double b() const;
    double c() const;
    double d() const;
    double e() const;
    double f() const;
    double m11() const;
    double m12() const;
    double m13() const;
    double m14() const;
    double m21() const;
    double m22() const;
    double m23() const;
    double m24() const;
    double m31() const;
    double m32() const;
    double m33() const;
    double m34() const;
    double m41() const;
    double m42() const;
    double m43() const;
    double m44() const;

    String* toString();

    DOMMatrix* translate(double tx = 0, double ty = 0, double tz = 0);

    DOMMatrix* scale(double sx = 1);
    DOMMatrix* scale(double sx, double sy, double sz = 1, double ox = 0,
                     double oy = 0, double oz = 0);
    DOMMatrix* scaleNonUniform(double sx = 1, double sy = 1);
    DOMMatrix* scale3d(double scale = 1, double ox = 0, double oy = 0,
                       double oz = 0);

    DOMMatrix* rotate(double rot_x);
    DOMMatrix* rotate(double rot_x, double rot_y);
    DOMMatrix* rotate(double rot_x, double rot_y, double rot_z);
    DOMMatrix* rotateFromVector(double x, double y);
    DOMMatrix* rotateAxisAngle(double x = 0, double y = 0, double z = 0,
                               double angle = 0);
    DOMMatrix* skewX(double sx);
    DOMMatrix* skewY(double sy);
    DOMMatrix* multiply();
    DOMMatrix* multiply(DOMMatrixInit& init);
    DOMMatrix* flipX();
    DOMMatrix* flipY();
    DOMMatrix* inverse();

    bool is2D()
    {
        return m_is2D;
    }

    void setIs2D(bool is2D)
    {
        m_is2D = is2D;
    }

    bool isValid()
    {
        return m_isValid;
    }

    void makeInvalid()
    {
        m_isValid = false;
    }

    bool isIdentity()
    {
        return (m_matrix == SkMatrix44::I());
    }

    ExecutionContext* executionContext()
    {
        return m_executionContext;
    }

    void setMatrix(SkMatrix44 matrix)
    {
        m_matrix = matrix;
    }

    SkMatrix44& matrix()
    {
        return m_matrix;
    }

    ScriptFloat32Array toFloat32Array();
    ScriptFloat64Array toFloat64Array();

    virtual bool isSerializable() const override;
    virtual Serializable* toSerializable() const override;
    virtual SerializedData* serialize(SerializingMap& memory) override;
    virtual void deserialize(SerializedData* serialized,
                             DeserializingMap& memory) const override;

    void set2DMatrix(double m11, double m12, double m21, double m22, double m41,
                     double m42);
    void set3DMatrix(double m11, double m12, double m13, double m14, double m21,
                     double m22, double m23, double m24, double m31, double m32,
                     double m33, double m34, double m41, double m42, double m43,
                     double m44);

private:
    ExecutionContext* m_executionContext;
    SkMatrix44 m_matrix;
    bool m_is2D;
    bool m_isValid;
};
} // namespace Starfish
#endif
