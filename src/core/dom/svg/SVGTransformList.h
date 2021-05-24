/*
 * Copyright (c) 2021-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishSVGTransformList__
#define __StarfishSVGTransformList__

#include "SVGElement.h"
#include "core/dom/svg/SVGTransform.h"
#include "binding/DocumentHoldable.h"
#include "binding/ScriptWrappable.h"

namespace Starfish {

class SVGElement;

class SVGTransformList : public ScriptWrappable {
public:
    SVGTransformList(SVGElement* sourceElement, QualifiedName targetAttribute,
                     bool readOnly = false);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGTransformList() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    unsigned long length();
    unsigned long numberOfItems();

    void clear();
    SVGTransform* initialize(SVGTransform* newItem);
    SVGTransform* getItem(unsigned long index);
    SVGTransform* insertItemBefore(SVGTransform* newItem, unsigned long index);
    SVGTransform* replaceItem(SVGTransform* newItem, unsigned long index);
    SVGTransform* removeItem(unsigned long index);
    SVGTransform* appendItem(SVGTransform* newItem);
    bool defaultIndexedSetter(unsigned long index, SVGTransform* newItem);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    void updateListByAttribute();
    void updateAttributeByList();

    bool isReadOnly();

    String* toString();

protected:
    void clearWithoutUpdateAttribute()
    {
        m_v.clear();
    }

    bool insertItemWithoutUpdateAttribute(SVGTransform* newItem,
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
            m_v.erase(index);
            return true;
        } else {
            return false;
        }
    }

    void appendItemWithoutUpdateAttribute(SVGTransform* newItem)
    {
        m_v.push_back(newItem);
    }

    SVGElement* m_sourceElement;
    QualifiedName m_targetAttribute;

    bool m_readOnly;
    GCVector<SVGTransform*> m_v;
};
} // namespace Starfish

#endif
