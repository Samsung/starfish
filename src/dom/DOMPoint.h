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

#ifndef __StarFishDOMPoint__
#define __StarFishDOMPoint__

#include "util/String.h"
#include "dom/DOMPointReadOnly.h"

namespace StarFish {

struct DOMPointInit {
    DOMPointInit(double inX = 0, double inY = 0, double inZ = 0,
                 double inW = 1);

    double x;
    double y;
    double z;
    double w;
};

class DOMPoint : public DOMPointReadOnly {
public:
    static DOMPoint* create(double x = 0, double y = 0, double z = 0,
                            double w = 1);
    static DOMPoint* create(const DOMPointInit&);

    void setX(double x)
    {
        m_x = x;
    }
    void setY(double y)
    {
        m_y = y;
    }
    void setZ(double z)
    {
        m_z = z;
    }
    void setW(double w)
    {
        m_w = w;
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this, instance);
    }

    virtual bool isDOMPoint() const
    {
        return true;
    }

protected:
    DOMPoint(double x, double y, double z, double w);
};
}

#endif
