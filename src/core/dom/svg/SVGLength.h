/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __StarFishSVGLength__
#define __StarFishSVGLength__

#include "binding/DocumentHoldable.h"
#include "binding/ScriptWrappable.h"

namespace StarFish {

class SVGElement;

class SVGLength : public ScriptWrappable {
public:
    SVGLength(SVGElement* sourceElement, QualifiedName targetAttribute);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGLength() const override;
    virtual ScriptBindingInstance* scriptBindingInstance();

    unsigned short unitType();
    float value();
    void setValue(float v);

protected:
    SVGElement* m_sourceElement;
    QualifiedName m_targetAttribute;
};
}

#endif
