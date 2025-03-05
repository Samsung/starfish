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

#include "StarfishConfig.h"
#include "SVGNumberList.h"
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

SVGNumberList::SVGNumberList(SVGElement* sourceElement,
                             QualifiedName targetAttribute)
    : ScriptWrappable(this)
    , m_sourceElement(sourceElement)
    , m_targetAttribute(targetAttribute)
    , m_isUpdated(false)
    , m_isReadOnly(false)
{
}

ScriptBindingInstance* SVGNumberList::scriptBindingInstance()
{
    return m_sourceElement->scriptBindingInstance();
}

unsigned long SVGNumberList::length()
{
    return numberOfItems();
}

unsigned long SVGNumberList::numberOfItems()
{
    return m_v.size();
}

void SVGNumberList::clear()
{
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return;
    }
    clearWithoutUpdateAttribute();

    if (m_targetAttribute.toString()->equals(
            AtomicString::emptyAtomicString()) != true) {
        updateAttributeByList();
    }
}

SVGNumber* SVGNumberList::initialize(SVGNumber* newItem)
{
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return nullptr;
    }

    clearWithoutUpdateAttribute();

    SVGNumber* item =
        new SVGNumber(m_sourceElement, m_targetAttribute, newItem->value());

    appendItemWithoutUpdateAttribute(item);

    if (m_targetAttribute.toString()->equals(
            AtomicString::emptyAtomicString()) != true) {
        updateAttributeByList();
    }
    return item;
}

SVGNumber* SVGNumberList::getItem(unsigned long index)
{
    if (index >= length()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::INDEX_SIZE_ERR, "IndexSizeError");
        return nullptr;
    }
    return m_v.at(index);
}

SVGNumber* SVGNumberList::insertItemBefore(SVGNumber* newItem,
                                           unsigned long index)
{
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return nullptr;
    }

    SVGNumber* item =
        new SVGNumber(m_sourceElement, m_targetAttribute, newItem->value());

    if (length() > index) {
        insertItemWithoutUpdateAttribute(item, index);
    } else {
        appendItemWithoutUpdateAttribute(item);
    }

    if (m_targetAttribute.toString()->equals(
            AtomicString::emptyAtomicString()) != true) {
        updateAttributeByList();
    }
    return item;
}

SVGNumber* SVGNumberList::replaceItem(SVGNumber* newItem, unsigned long index)
{
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return nullptr;
    }

    if (index >= length()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::INDEX_SIZE_ERR, "IndexSizeError");
        return nullptr;
    }

    SVGNumber* item =
        new SVGNumber(m_sourceElement, m_targetAttribute, newItem->value());

    m_v.at(index)->setValue(item->value());

    if (m_targetAttribute.toString()->equals(
            AtomicString::emptyAtomicString()) != true) {
        updateAttributeByList();
    }

    return item;
}

SVGNumber* SVGNumberList::removeItem(unsigned long index)
{
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return nullptr;
    }

    if (index >= length()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::INDEX_SIZE_ERR, "IndexSizeError");
        return nullptr;
    }

    SVGNumber* item = m_v.at(index);
    removeItemWithoutUpdateAttribute(index);
    updateAttributeByList();
    return item;
}

SVGNumber* SVGNumberList::appendItem(SVGNumber* newItem)
{
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return nullptr;
    }
    SVGNumber* item =
        new SVGNumber(m_sourceElement, m_targetAttribute, newItem->value());
    appendItemWithoutUpdateAttribute(item);

    if (m_targetAttribute.toString()->equals(
            AtomicString::emptyAtomicString()) != true) {
        updateAttributeByList();
    }
    return item;
}

bool SVGNumberList::defaultIndexedSetter(unsigned long index,
                                         SVGNumber* newItem)
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

String* SVGNumberList::toString()
{
    StringBuilder sb;
    for (auto* item : m_v) {
        if (sb.length()) {
            sb.appendChar(' ');
        }
        sb.appendString(String::fromFloat(item->value()));
    }
    return sb.finalize();
}

void* SVGNumberList::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGNumberList));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGNumberList)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(SVGNumberList, m_v));
        SVGElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGNumberList));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGNumberList::updateAttributeByList()
{
    m_isUpdated = true;

    StringBuilder sb;
    for (size_t i = 0; i < length(); ++i) {
        sb.appendString(String::fromFloat(getItem(i)->value()));
        if (i < length() - 1) {
            sb.appendChar(' ');
        }
    }

    m_sourceElement->setAttribute(m_targetAttribute, sb.finalize());
}

void SVGNumberList::updateListByAttribute()
{
    clearWithoutUpdateAttribute();

    String* attrValue = m_sourceElement->getAttributeOrEmpty(m_targetAttribute);
    if (attrValue->length()) {
        GCVector<StringView> tokens;
        StringUtils::wordTokenizer(attrValue, tokens);

        for (size_t i = 0; i < tokens.size(); ++i) {
            SVGNumber* newItem = new SVGNumber(
                m_sourceElement, AtomicString::emptyAtomicString());
            newItem->setValue(String::parseFloat(tokens[i].substring()));
            appendItemWithoutUpdateAttribute(newItem);
        }
    }
}

bool SVGNumberList::isUpdated()
{
    return m_isUpdated;
}

void SVGNumberList::unsetUpdated()
{
    m_isUpdated = false;
}

bool SVGNumberList::isReadOnly()
{
    return m_isReadOnly;
}

void SVGNumberList::setReadOnly()
{
    m_isReadOnly = true;
}
} // namespace Starfish
