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

#ifndef __StarFishSVGAnimatedLength__
#define __StarFishSVGAnimatedLength__

#include "binding/DocumentHoldable.h"
#include "binding/ScriptWrappable.h"
#include "core/dom/svg/SVGLength.h"

namespace StarFish {

class SVGAnimatedLength : public ScriptWrappable, public DocumentHoldable {
public:
    SVGAnimatedLength(Document* document, SVGLength* baseVal,
                      SVGLength* animVal);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGAnimatedLength() const override;
    virtual ScriptBindingInstance* scriptBindingInstance()
    {
        return DocumentHoldable::scriptBindingInstance();
    }

    SVGLength* baseVal() const
    {
        return m_baseVal;
    }

    SVGLength* animVal() const
    {
        return m_animVal;
    }

protected:
    SVGLength* m_baseVal;
    SVGLength* m_animVal;
};
}

#endif
