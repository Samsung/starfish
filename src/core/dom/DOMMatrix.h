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

#ifndef __StarfishDOMMatrix__
#define __StarfishDOMMatrix__

#include <SkMatrix44.h>
#include "core/dom/DOMMatrixReadOnly.h"
#include "binding/DOMStringOrSequenceUnion.h"

namespace Starfish {
class DOMMatrix : public DOMMatrixReadOnly {
public:
    static DOMMatrix* Create(DOMMatrixReadOnly* domMatrix);

    static DOMMatrix* fromFloat32Array(ExecutionContext* executionContext,
                                       ScriptFloat32Array array32);

    static DOMMatrix* fromFloat64Array(ExecutionContext* executionContext,
                                       ScriptFloat64Array array64);

    static DOMMatrix* fromMatrix(ExecutionContext* executionContext,
                                 DOMMatrixInit& init);
    static DOMMatrix* fromMatrix(ExecutionContext* executionContext);

    DOMMatrix(ExecutionContext* executionContext);
    DOMMatrix(ExecutionContext* executionContext, DOMStringOrSequence value);
    DOMMatrix(ExecutionContext* executionContext, SkMatrix44 matrix, bool is2D);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(DOMMatrix)

    void setA(double value);
    void setB(double value);
    void setC(double value);
    void setD(double value);
    void setE(double value);
    void setF(double value);
    void setM11(double value);
    void setM12(double value);
    void setM13(double value);
    void setM14(double value);
    void setM21(double value);
    void setM22(double value);
    void setM23(double value);
    void setM24(double value);
    void setM31(double value);
    void setM32(double value);
    void setM33(double value);
    void setM34(double value);
    void setM41(double value);
    void setM42(double value);
    void setM43(double value);
    void setM44(double value);

    DOMMatrix* multiplySelf(DOMMatrixInit& other);
    DOMMatrix* multiplySelf();
    DOMMatrix* preMultiplySelf();
    DOMMatrix* preMultiplySelf(DOMMatrixInit& other);
    DOMMatrix* translateSelf(double tx = 0, double ty = 0, double tz = 0);
    DOMMatrix* scaleSelf(double sx = 1);
    DOMMatrix* scaleSelf(double sx, double sy, double sz = 1, double ox = 0,
                         double oy = 0, double oz = 0);
    DOMMatrix* scale3dSelf(double scale = 1, double ox = 0, double oy = 0,
                           double oz = 0);
    DOMMatrix* rotateSelf(double rot_x, double rot_y, double rot_z);
    DOMMatrix* rotateFromVectorSelf(double x, double y);
    DOMMatrix* rotateAxisAngleSelf(double x = 0, double y = 0, double z = 0,
                                   double angle = 0);

    DOMMatrix* skewXSelf(double sx = 0);
    DOMMatrix* skewYSelf(double sy = 0);

    // DOMMatrix* perspectiveSelf(double p);
    DOMMatrix* invertSelf();

    ExecutionContext* executionContext()
    {
        return DOMMatrixReadOnly::executionContext();
    }

    void setMatrix(SkMatrix44 matrix)
    {
        DOMMatrixReadOnly::setMatrix(matrix);
    }

    SkMatrix44& matrix()
    {
        return DOMMatrixReadOnly::matrix();
    }

    bool is2D()
    {
        return DOMMatrixReadOnly::is2D();
    }

    void setIs2D(bool is2D)
    {
        DOMMatrixReadOnly::setIs2D(is2D);
    }

    bool isValid()
    {
        return DOMMatrixReadOnly::isValid();
    }

    void makeInvalid()
    {
        DOMMatrixReadOnly::makeInvalid();
    }

    virtual bool isSerializable() const override;
    virtual Serializable* toSerializable() const override;
    virtual SerializedData* serialize(SerializingMap& memory) override;
    virtual void deserialize(SerializedData* serialized,
                             DeserializingMap& memory) const override;

private:
    DOMMatrix* skew(double sx, double sy);
};
} // namespace Starfish
#endif
