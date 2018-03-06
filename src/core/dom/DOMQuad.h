/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
    DOMQuad(Document* document, const DOMPointInit&, const DOMPointInit&,
            const DOMPointInit&, const DOMPointInit&);
    DOMQuad(const DOMRectInit&);

    virtual ScriptBindingInstance* scriptBindingInstance()
    {
        return m_scriptBindingInstance;
    }
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
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
    ScriptBindingInstance* m_scriptBindingInstance;
    DOMPoint* m_p1;
    DOMPoint* m_p2;
    DOMPoint* m_p3;
    DOMPoint* m_p4;
    mutable DOMRect* m_bounds; // allocated lazily
};
}

#endif
