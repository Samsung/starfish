/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishDOMStringList__
#define __StarFishDOMStringList__

#include "binding/ScriptWrappable.h"

namespace StarFish {

class Document;
class DOMStringList : public ScriptWrappable, public GCVector<String*> {
public:
    DOMStringList(Document* document);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isDOMStringList() const override;

    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
        return m_scriptBindingInstance;
    }

    size_t length() const
    {
        return m_size;
    }

    Nullable<String*> item(unsigned long index)
    {
        if (index < size()) {
            return at(index);
        }
        return Nullable<String*>();
    }

    bool contains(String* item)
    {
        return std::find(begin(), end(), item) != end();
    }

private:
    ScriptBindingInstance* m_scriptBindingInstance;
};
}
#endif
