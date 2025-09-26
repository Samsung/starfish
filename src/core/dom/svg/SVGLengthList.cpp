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

#include "StarfishConfig.h"
#include "SVGLengthList.h"
#include "SVGElement.h"
#include "core/style/ComputedStyle.h"
#include "core/style/CalcData.h"
#include "core/style/CSSParser.h"
#include "core/layout/FrameBox.h"
#include "core/layout/svg/FrameSVGSVGBox.h"
#include "core/page/BrowsingContext.h"
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"

namespace Starfish {

SVGLengthList::SVGLengthList(SVGElement* sourceElement,
                             QualifiedName targetAttribute)
    : ScriptWrappable(this)
    , m_sourceElement(sourceElement)
    , m_targetAttribute(targetAttribute)
    , m_isUpdated(false)
    , m_isReadOnly(false)
{
}

ScriptBindingInstance* SVGLengthList::scriptBindingInstance()
{
    return m_sourceElement->scriptBindingInstance();
}

// The length and numberOfItems IDL attributes represents the length of the
// list, and on getting simply return the length of the list.
unsigned long SVGLengthList::length()
{
    return numberOfItems();
}

unsigned long SVGLengthList::numberOfItems()
{
    return m_v.size();
}

void SVGLengthList::clear()
{
    // 1. If the list is read only, then throw a NoModificationAllowedError.
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return;
    }
    // 2. Detach and then remove all elements in the list.
    for (size_t i = 0; i < length(); ++i) {
        getItem(i)->detach();
    }
    clearWithoutUpdateAttribute();

    // 3. If the list reflects an attribute, or represents the base value of an
    // object that reflects an attribute, then reserialize the reflected
    // attribute.
    if (m_targetAttribute.toString()->equals(
            AtomicString::emptyAtomicString()) != true) {
        updateAttributeByList();
    }
}

SVGLength* SVGLengthList::initialize(SVGLength* newItem)
{
    // 1. If the list is read only, then throw a NoModificationAllowedError.
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return nullptr;
    }

    // 2. Detach and then remove all elements in the list.
    for (size_t i = 0; i < length(); ++i) {
        getItem(i)->detach();
    }
    clearWithoutUpdateAttribute();

    // 3. If newItem is an object type, and newItem is not a detached object,
    // then set newItem to be a newly created object of the same type as newItem
    // and which has the same (number or length) value.
    // 4. Attach newItem to the list interface object.
    SVGLength* item;
    if (newItem->isDetached() == true) {
        item = newItem;
        item->attach(m_sourceElement, m_targetAttribute);
    } else {
        item = new SVGLength(m_sourceElement, m_targetAttribute,
                             newItem->unitType(), newItem->value());
    }

    // 5. Append newItem to this list.
    appendItemWithoutUpdateAttribute(item);

    // 6. If the list reflects an attribute, or represents the base value of an
    // object that reflects an attribute, then reserialize the reflected
    // attribute.
    if (m_targetAttribute.toString()->equals(
            AtomicString::emptyAtomicString()) != true) {
        updateAttributeByList();
    }

    // 7. Return newItem
    return item;
}

SVGLength* SVGLengthList::getItem(unsigned long index)
{
    // 1. If index is greater than or equal to the length of the list, then
    // throw an IndexSizeError.
    if (index >= length()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::INDEX_SIZE_ERR, "IndexSizeError");
        return nullptr;
    }

    // 2. Return the element in the list at position index.
    return m_v.at(index);
}

SVGLength* SVGLengthList::insertItemBefore(SVGLength* newItem,
                                           unsigned long index)
{
    // 1. If the list is read only, then throw a NoModificationAllowedError.
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return nullptr;
    }

    // 2. If newItem is an object type, and newItem is not a detached object,
    // then set newItem to be a newly created object of the same type as newItem
    // and which has the same (number or length) value.
    // 5. Attach newItem to the list interface object.
    SVGLength* item;
    if (newItem->isDetached() == true) {
        item = newItem;
        item->attach(m_sourceElement, m_targetAttribute);
    } else {
        item = new SVGLength(m_sourceElement, m_targetAttribute,
                             newItem->unitType(), newItem->value());
    }

    // 3. If index is greater than the length of the list, then set index to be
    // the list length.
    // 4. Insert newItem into the list at index index.
    if (length() > index) {
        insertItemWithoutUpdateAttribute(item, index);
    } else {
        appendItemWithoutUpdateAttribute(item);
    }

    // 6. If the list reflects an attribute, or represents the base value of an
    // object that reflects an attribute, then reserialize the reflected
    // attribute.
    if (m_targetAttribute.toString()->equals(
            AtomicString::emptyAtomicString()) != true) {
        updateAttributeByList();
    }
    // 7. return newItem
    return item;
}

SVGLength* SVGLengthList::replaceItem(SVGLength* newItem, unsigned long index)
{
    // 1. If the list is read only, then throw a NoModificationAllowedError.
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return nullptr;
    }

    // 2. If index is greater than or equal to the length of the list, then
    // throw an IndexSizeError.
    if (index >= length()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::INDEX_SIZE_ERR, "IndexSizeError");
        return nullptr;
    }

    // 3. If newItem is an object type, and newItem is not a detached object,
    // then set newItem to be a newly created object of the same type as newItem
    // and which has the same (number or length) value.
    // 6. Attach newItem to the list interface object.
    SVGLength* item;
    if (newItem->isDetached() == true) {
        item = newItem;
        item->attach(m_sourceElement, m_targetAttribute);
    } else {
        item = new SVGLength(m_sourceElement, m_targetAttribute,
                             newItem->unitType(), newItem->value());
    }

    // 4. Detach the element in the list at index index.
    m_v.at(index)->detach();

    // 5. Replace the element in the list at index index with newItem.
    m_v.at(index)->newValueSpecifiedUnits(item->unitType(), item->value());

    // 7. If the list reflects an attribute, or represents the base value of an
    // object that reflects an attribute, then reserialize the reflected
    // attribute.
    if (m_targetAttribute.toString()->equals(
            AtomicString::emptyAtomicString()) != true) {
        updateAttributeByList();
    }

    // 8. Return newItem.
    return item;
}

SVGLength* SVGLengthList::removeItem(unsigned long index)
{
    // 1. If the list is read only, then throw a NoModificationAllowedError.
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return nullptr;
    }

    // 2. If index is greater than or equal to the length of the list, then
    // throw an IndexSizeError with code.
    if (index >= length()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::INDEX_SIZE_ERR, "IndexSizeError");
        return nullptr;
    }

    // 3. Let item be the list element at index index.
    SVGLength* item = m_v.at(index);

    // 4. Detach item.
    item->detach();

    // 5. Remove the list element at index Index.
    removeItemWithoutUpdateAttribute(index);

    updateAttributeByList();
    // 6. Return item.
    return item;
}

SVGLength* SVGLengthList::appendItem(SVGLength* newItem)
{
    // 1. If the list is read only, then throw a NoModificationAllowedError.
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return nullptr;
    }
    // 2. If newItem is an object type, and newItem is not a detached object,
    // then set newItem to be a newly created object of the same type as newItem
    // and which has the same (number or length) value.
    // 5. Attach newItem to the list interface object.
    SVGLength* item;
    if (newItem->isDetached() == true) {
        item = newItem;
        item->attach(m_sourceElement, m_targetAttribute);
    } else {
        item = new SVGLength(m_sourceElement, m_targetAttribute,
                             newItem->unitType(), newItem->value());
    }

    // 3. Let index be the length of the list.
    // 4. Append newItem to the end of the list.
    appendItemWithoutUpdateAttribute(item);

    // 6. If the list reflects an attribute, or represents the base value of an
    // object that reflects an attribute, then reserialize the reflected
    // attribute.
    if (m_targetAttribute.toString()->equals(
            AtomicString::emptyAtomicString()) != true) {
        updateAttributeByList();
    }

    // 7. Return newItem.
    return item;
}

bool SVGLengthList::defaultIndexedSetter(unsigned long index,
                                         SVGLength* newItem)
{
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return false;
    }

    if (index >= length()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::INDEX_SIZE_ERR, "IndexSizeError");
        return false;
    }

    replaceItem(newItem, index);
    return true;
}

void* SVGLengthList::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGLengthList));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGLengthList)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(SVGLengthList, m_v));
        SVGElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGLengthList));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGLengthList::updateAttributeByList()
{
    m_isUpdated = true;

    StringBuilder sb;
    for (size_t i = 0; i < length(); ++i) {
        sb.appendString(getItem(i)->valueAsString());
        if (i < length() - 1) {
            sb.appendChar(' ');
        }
    }

    m_sourceElement->setAttribute(m_targetAttribute, sb.finalize());
}

void SVGLengthList::updateListByAttribute()
{
    clearWithoutUpdateAttribute();

    String* attrValue = m_sourceElement->getAttributeOrEmpty(m_targetAttribute);
    if (attrValue->length()) {
        GCVector<StringView> tokens;
        StringUtils::wordTokenizer(attrValue, tokens);

        for (size_t i = 0; i < tokens.size(); ++i) {
            SVGLength* newItem = new SVGLength(
                m_sourceElement, AtomicString::emptyAtomicString());
            newItem->setValueAsString(tokens[i].substring(), false, false);

            appendItemWithoutUpdateAttribute(newItem);
        }
    }
}

bool SVGLengthList::isUpdated()
{
    return m_isUpdated;
}

void SVGLengthList::unsetUpdated()
{
    m_isUpdated = false;
}

bool SVGLengthList::isReadOnly()
{
    return m_isReadOnly;
}

void SVGLengthList::setReadOnly()
{
    m_isReadOnly = true;
}
} // namespace Starfish
