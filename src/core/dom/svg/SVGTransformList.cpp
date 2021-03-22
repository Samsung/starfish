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
#include "core/dom/svg/SVGTransformList.h"
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"

namespace Starfish {

SVGTransformList::SVGTransformList(SVGElement* sourceElement,
                                   QualifiedName targetAttribute)
    : ScriptWrappable(this)
    , m_sourceElement(sourceElement)
    , m_targetAttribute(targetAttribute)
    , m_isReadOnly(false)
{
    updateListByAttribute();
}

ScriptBindingInstance* SVGTransformList::scriptBindingInstance()
{
    return m_sourceElement->scriptBindingInstance();
}

unsigned long SVGTransformList::length()
{
    return numberOfItems();
}

unsigned long SVGTransformList::numberOfItems()
{
    return m_v.size();
}

void SVGTransformList::clear()
{
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return;
    }

    for (unsigned long i = 0; i < length(); ++i) {
        getItem(i)->detach();
    }
    clearWithoutUpdateAttribute();

    m_sourceElement->updateAttributeNeeded(m_targetAttribute);
}

SVGTransform* SVGTransformList::initialize(SVGTransform* newItem)
{
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return nullptr;
    }

    for (size_t i = 0; i < length(); ++i) {
        getItem(i)->detach();
    }
    clearWithoutUpdateAttribute();

    SVGTransform* item;
    if (newItem->isDetached() == true) {
        item = newItem;
        item->attach(m_sourceElement, m_targetAttribute);
    } else {
        item = new SVGTransform(m_sourceElement, m_targetAttribute,
                                newItem->value());
    }

    appendItemWithoutUpdateAttribute(item);

    m_sourceElement->updateAttributeNeeded(m_targetAttribute);
    return item;
}

SVGTransform* SVGTransformList::getItem(unsigned long index)
{
    if (index >= length()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::INDEX_SIZE_ERR, "IndexSizeError");
        return nullptr;
    }

    return m_v.at(index);
}

SVGTransform* SVGTransformList::insertItemBefore(SVGTransform* newItem,
                                                 unsigned long index)
{
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return nullptr;
    }

    SVGTransform* item;
    if (newItem->isDetached() == true) {
        item = newItem;
        item->attach(m_sourceElement, m_targetAttribute);
    } else {
        item = new SVGTransform(m_sourceElement, m_targetAttribute,
                                newItem->value());
    }

    if (length() > index) {
        insertItemWithoutUpdateAttribute(item, index);
    } else {
        appendItemWithoutUpdateAttribute(item);
    }

    m_sourceElement->updateAttributeNeeded(m_targetAttribute);

    return item;
}

SVGTransform* SVGTransformList::replaceItem(SVGTransform* newItem,
                                            unsigned long index)
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

    SVGTransform* item;
    if (newItem->isDetached() == true) {
        item = newItem;
        item->attach(m_sourceElement, m_targetAttribute);
    } else {
        item = new SVGTransform(m_sourceElement, m_targetAttribute,
                                newItem->value());
    }

    m_v.at(index)->detach();
    m_v.erase(index);
    m_v.insert(index, item);

    m_sourceElement->updateAttributeNeeded(m_targetAttribute);

    return item;
}

SVGTransform* SVGTransformList::removeItem(unsigned long index)
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

    SVGTransform* item = m_v.at(index);

    item->detach();

    removeItemWithoutUpdateAttribute(index);
    m_sourceElement->updateAttributeNeeded(m_targetAttribute);

    return item;
}

SVGTransform* SVGTransformList::appendItem(SVGTransform* newItem)
{
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return nullptr;
    }

    SVGTransform* item;
    if (newItem->isDetached() == true) {
        item = newItem;
        item->attach(m_sourceElement, m_targetAttribute);
    } else {
        item = new SVGTransform(m_sourceElement, m_targetAttribute,
                                newItem->value());
    }

    appendItemWithoutUpdateAttribute(item);

    m_sourceElement->updateAttributeNeeded(m_targetAttribute);

    return item;
}

bool SVGTransformList::defaultIndexedSetter(unsigned long index,
                                            SVGTransform* newItem)
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

void* SVGTransformList::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGTransformList));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGTransformList)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(SVGTransformList, m_v));
        SVGElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGTransformList));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGTransformList::updateAttributeByList()
{
    m_sourceElement->setAttribute(m_targetAttribute, toString());
}

void SVGTransformList::updateListByAttribute()
{
    clearWithoutUpdateAttribute();

    String* attrValue = m_sourceElement->getAttributeOrEmpty(m_targetAttribute);
    if (attrValue->length()) {
        auto str = attrValue->toUTF8NonGCString();
        CSSTokenVector tokens;
        CSSStyleDeclaration::tokenizeCSSValue(tokens, str.data(), str.length());

        CSSStyleValuePair* value = new CSSStyleValuePair;
        if (value->updateValueTransform(tokens, true,
                                        Separator::SpaceSeparator) == false) {
        }

        for (size_t i = 0; i < tokens.size(); ++i) {
            CSSTransformFunction f = value->transformValue()->at(i);
            SVGTransform* newItem = new SVGTransform(
                m_sourceElement, AtomicString::emptyAtomicString(), f);

            appendItemWithoutUpdateAttribute(newItem);
        }
    }
}

bool SVGTransformList::isReadOnly()
{
    return m_isReadOnly;
}

void SVGTransformList::setReadOnly()
{
    m_isReadOnly = true;
}

String* SVGTransformList::toString()
{
    StringBuilder sb;
    SVGTransform* item;
    for (size_t i = 0; i < length(); ++i) {
        item = getItem(i);
        CSSTransformFunction f = getItem(i)->value();
        sb.appendString(f.functionName());
        sb.appendChar('(');
        ValueList* values = f.values();
        for (size_t j = 0; j < values->size(); ++j) {
            const CSSStyleValuePair& subitem = (*values)[j];

            if (subitem.valueKind() == CSSStyleValuePair::ValueKind::Length) {
                sb.appendString(
                    String::fromFloat(subitem.lengthValue().fixed()));
            } else if (subitem.valueKind() ==
                       CSSStyleValuePair::ValueKind::Angle) {
                sb.appendString(
                    String::fromFloat(subitem.angleValue().value()));
            } else if (subitem.valueKind() ==
                       CSSStyleValuePair::ValueKind::Number) {
                sb.appendString(String::fromFloat(subitem.numberValue()));
            } else {
                STARFISH_ASSERT_NOT_REACHED();
            }

            if (j != values->size() - 1) {
                sb.appendChar(' ');
            }
        }
        sb.appendChar(')');
        if (i < length() - 1) {
            sb.appendChar(' ');
        }
    }

    return sb.finalize();
}
} // namespace Starfish
