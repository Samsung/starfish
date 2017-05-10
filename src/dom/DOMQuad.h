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

#ifndef __StarFishDOMQuad__
#define __StarFishDOMQuad__

#include "binding/ScriptWrappable.h"

namespace StarFish {

struct DOMRectInit;
struct DOMPointInit;
class DOMPoint;
class DOMRectReadOnly;

class DOMQuad : public ScriptWrappable {
public:
    DOMQuad(const DOMPointInit&, const DOMPointInit&, const DOMPointInit&,
            const DOMPointInit&);
    DOMQuad(const DOMRectInit&);

    virtual void init(ScriptBindingInstance* instance) override;
    virtual bool isDOMQuad() const override;

    DOMPoint* p1() const
    {
        return m_p1;
    }

    DOMPoint* p2() const
    {
        return m_p2;
    }

    DOMPoint* p3() const
    {
        return m_p3;
    }

    DOMPoint* p4() const
    {
        return m_p4;
    }

    DOMRect* getBounds() const;

protected:
    DOMPoint* m_p1;
    DOMPoint* m_p2;
    DOMPoint* m_p3;
    DOMPoint* m_p4;
    mutable DOMRect* m_bounds; // allocated lazily
};
}

#endif
