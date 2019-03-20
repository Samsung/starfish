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

#ifndef __StarfishDOMRectList__
#define __StarfishDOMRectList__

namespace Starfish {

class DOMRect;

class DOMRectList : public ScriptWrappable {
public:
    static DOMRectList* create(ExecutionContext* executionContext)
    {
        return new DOMRectList(executionContext);
    }

    static DOMRectList* create(ExecutionContext* executionContext,
                               const GCVector<DOMQuad*>& quads)
    {
        return new DOMRectList(executionContext, quads);
    }

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(DOMRectList)

    uint32_t length() const;
    DOMRect* item(uint32_t index);

private:
    DOMRectList(ExecutionContext* executionContext);
    explicit DOMRectList(ExecutionContext* executionContext,
                         const GCVector<DOMQuad*>&);
    GCVector<DOMRect*> m_list;
};
}

#endif
