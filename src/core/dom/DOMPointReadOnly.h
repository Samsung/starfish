/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishDOMPointReadOnly__
#define __StarFishDOMPointReadOnly__

#include "binding/ScriptWrappable.h"

namespace StarFish {

class DOMPointReadOnly : public ScriptWrappable {
public:
    DOMPointReadOnly(double x, double y, double z, double w);

    double x() const
    {
        return m_x;
    }

    double y() const
    {
        return m_y;
    }

    double z() const
    {
        return m_z;
    }

    double w() const
    {
        return m_w;
    }

    virtual void init(ScriptBindingInstance* instance) override;
    virtual bool isDOMPointReadOnly() const override;

    // to do : doesn't appear to be supported anywhere yet.
    // DOMPoint matrixTransform(DOMMatrixReadOnly matrix);

protected:
    double m_x;
    double m_y;
    double m_z;
    double m_w;
};
}

#endif
