/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#ifndef __StarFishDOMRect__
#define __StarFishDOMRect__

#include "util/String.h"
#include "dom/DOMRectReadOnly.h"

namespace StarFish {

struct DOMRectInit {
    DOMRectInit(double inX = 0 , double inY = 0, double inWidth = 0, double inHeight = 0);

    double x;
    double y;
    double width;
    double height;
};

class DOMRect : public DOMRectReadOnly {

public:
    static DOMRect* create(double x = 0 , double y = 0, double width = 0, double height = 0);
    static DOMRect* create(const DOMRectReadOnly*);

    void setX(double x) { m_x = x; }
    void setY(double y) { m_y = y; }
    void setWidth(double width) { m_width = width; }
    void setHeight(double height) { m_height = height; }
    void unite(const DOMRectReadOnly*);

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this, instance);
    }

    virtual Type type()
    {
        return ScriptWrappable::Type::DOMRectObject;
    }

protected:
    DOMRect(double x, double y, double width, double height);
};

}

#endif
