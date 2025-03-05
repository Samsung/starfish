/*
 * Copyright (c) 2025-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishSVGNumberList__
#define __StarfishSVGNumberList__

#include "core/dom/svg/SVGNumber.h"
#include "binding/DocumentHoldable.h"
#include "binding/ScriptWrappable.h"

namespace Starfish {

class SVGElement;

class SVGNumberList : public ScriptWrappable {
public:
    SVGNumberList(SVGElement* sourceElement, QualifiedName targetAttribute);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGNumberList() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    unsigned long length();
    unsigned long numberOfItems();

    void clear();
    SVGNumber* initialize(SVGNumber* newItem);
    SVGNumber* getItem(unsigned long index);
    SVGNumber* insertItemBefore(SVGNumber* newItem, unsigned long index);
    SVGNumber* replaceItem(SVGNumber* newItem, unsigned long index);
    SVGNumber* removeItem(unsigned long index);
    SVGNumber* appendItem(SVGNumber* newItem);
    bool defaultIndexedSetter(unsigned long index, SVGNumber* newItem);
    String* toString();

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    void updateListByAttribute();
    void updateAttributeByList();

    bool isUpdated();
    void unsetUpdated();

    bool isReadOnly();
    void setReadOnly();

protected:
    void clearWithoutUpdateAttribute()
    {
        m_v.clear();
    }

    bool insertItemWithoutUpdateAttribute(SVGNumber* newItem,
                                          unsigned long index)
    {
        if (index < length()) {
            m_v.insert(index, newItem);
            return true;
        } else {
            return false;
        }
    }

    bool removeItemWithoutUpdateAttribute(unsigned long index)
    {
        if (index < length()) {
            m_v.erase((size_t)index);
            return true;
        } else {
            return false;
        }
    }

    void appendItemWithoutUpdateAttribute(SVGNumber* newItem)
    {
        m_v.push_back(newItem);
    }

    SVGElement* m_sourceElement;
    QualifiedName m_targetAttribute;

    bool m_isUpdated;
    bool m_isReadOnly;
    GCVector<SVGNumber*> m_v;
};
} // namespace Starfish

#endif
