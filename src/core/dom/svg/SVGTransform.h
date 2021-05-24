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

#ifndef __StarfishSVGTransform__
#define __StarfishSVGTransform__

#include "binding/DocumentHoldable.h"
#include "binding/ScriptWrappable.h"
#include "core/style/Style.h"
#include "core/dom/DOMMatrix.h"

namespace Starfish {

class SVGElement;

class SVGTransform : public ScriptWrappable {
public:
    enum Type {
        SVG_TRANSFORM_UNKOWN = 0,
        SVG_TRANSFORM_MATRIX,
        SVG_TRANSFORM_TRANSLATE,
        SVG_TRANSFORM_SCALE,
        SVG_TRANSFORM_ROTATE,
        SVG_TRANSFORM_SKEWX,
        SVG_TRANSFORM_SKEWY
    };

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    SVGTransform(SVGElement* sourceElement, QualifiedName targetAttribute,
                 CSSTransformFunction value =
                     CSSTransformFunction(CSSTransformFunction::Matrix),
                 bool readOnly = false);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGTransform() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    unsigned short type();
    DOMMatrix* matrix();
    float angle();

    void setMatrix(DOMMatrixReadOnly* matrix);
    void setTranslate(float tx, float ty);
    void setScale(float sx, float sy);
    void setRotate(float angle, float cx, float cy);
    void setSkewX(float angle);
    void setSkewY(float angle);

    bool isReadOnly();
    void updateMatrixByValue();

    void detach();
    bool isDetached();
    void attach(SVGElement* sourceElement, QualifiedName targetAttribute);

    CSSTransformFunction value();

protected:
    SVGElement* m_sourceElement;
    QualifiedName m_targetAttribute;

    CSSTransformFunction m_value;
    DOMMatrix* m_matrixObject;
    DOMMatrix* m_matrixComparisonTarget;

    bool m_readOnly;
};
} // namespace Starfish

#endif
