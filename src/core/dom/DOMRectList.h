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

#ifndef __StarFishDOMRectList__
#define __StarFishDOMRectList__

#include "binding/ScriptWrappable.h"

namespace StarFish {

class DOMRect;

class DOMRectList : public ScriptWrappable {
public:
    static DOMRectList* create(Document* document)
    {
        return new DOMRectList(document);
    }

    static DOMRectList* create(Document* document,
                               const GCVector<DOMQuad*>& quads)
    {
        return new DOMRectList(document, quads);
    }

    uint32_t length() const;
    DOMRect* item(uint32_t index);

    virtual ScriptBindingInstance* scriptBindingInstance()
    {
        return m_scriptBindingInstance;
    }
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isDOMRectList() const override;

private:
    DOMRectList(Document* document);
    explicit DOMRectList(Document* document, const GCVector<DOMQuad*>&);
    ScriptBindingInstance* m_scriptBindingInstance;
    GCVector<DOMRect*> m_list;
};
}

#endif
