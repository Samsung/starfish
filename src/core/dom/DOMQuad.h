/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishDOMQuad__
#define __StarfishDOMQuad__

namespace Starfish {

struct DOMRectInit;
struct DOMPointInit;
class DOMPoint;
class DOMRectReadOnly;

class DOMQuad : public ScriptWrappable {
public:
    DOMQuad(ExecutionContext* executionContext, const DOMPointInit&,
            const DOMPointInit&, const DOMPointInit&, const DOMPointInit&);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(DOMQuad)

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
    ExecutionContext* m_executionContext;
    DOMPoint* m_p1;
    DOMPoint* m_p2;
    DOMPoint* m_p3;
    DOMPoint* m_p4;
    mutable DOMRect* m_bounds; // allocated lazily
};
} // namespace Starfish

#endif
