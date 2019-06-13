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
#include "DOMExceptionOr.h"

namespace Starfish {
class DOMMatrix;
class DOMMatrixReadOnly : public ScriptWrappable, public Serializable {
public:
    static DOMExceptionOr<void> validateAndFixup(
        ExecutionContext* executionContext, DOMMatrix2DInit& init);

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

    bool is2D()
    {
        return m_is2D;
    }

    void setIs2D(bool is2D)
    {
        m_is2D = is2D;
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

    virtual bool isSerializable() const override;
    virtual Serializable* toSerializable() const override;
    virtual SerializedData* serialize(SerializingMap& memory) override;
    virtual void deserialize(SerializedData* serialized,
                             DeserializingMap& memory) const override;

private:
    void set2DMatrix(double val1, double val2, double val3, double val4,
                     double val5, double val6);
    void set3DMatrix(double val1, double val2, double val3, double val4,
                     double val5, double val6, double val7, double val8,
                     double val9, double val10, double val11, double val12,
                     double val13, double val14, double val15, double val16);

    ExecutionContext* m_executionContext;
    SkMatrix44 m_matrix;
    bool m_is2D;
};
} // namespace Starfish
#endif
