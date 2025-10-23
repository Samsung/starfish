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
                                   QualifiedName targetAttribute, bool readOnly)
    : ScriptWrappable(this)
    , m_sourceElement(sourceElement)
    , m_targetAttribute(targetAttribute)
    , m_readOnly(readOnly)
{
    updateListByAttribute();
}

SVGTransformList::SVGTransformList(SVGElement* sourceElement,
                                   QualifiedName targetAttribute,
                                   SVGTransformList* sourceObject)
    : ScriptWrappable(this)
    , m_sourceElement(sourceElement)
    , m_targetAttribute(targetAttribute)
    , m_sourceObject(sourceObject)
    , m_readOnly(true)
{
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
    if (m_sourceObject) {
        if (m_sourceElement->animatedTransformAttribute(
                m_targetAttribute.localNameAtomic())) {
            return 1;
        }
        return m_sourceObject->numberOfItems();
    } else {
        return m_v.size();
    }
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

    m_sourceElement->updateSVGAttributeNeeded(m_targetAttribute);
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

    m_sourceElement->updateSVGAttributeNeeded(m_targetAttribute);
    return item;
}

SVGTransform* SVGTransformList::getItem(unsigned long index)
{
    if (index >= length()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::INDEX_SIZE_ERR, "IndexSizeError");
        return nullptr;
    }

    if (m_sourceObject) {
        auto v = m_sourceElement->animatedTransformAttribute(
            m_targetAttribute.localNameAtomic());
        if (v) {
            SVGTransform* newItem = new SVGTransform(
                m_sourceElement, AtomicString::emptyAtomicString(),
                CSSTransformFunction::Matrix, false);
            if (v.value()->type() == StyleTransformData::Rotate) {
                newItem->setRotate(v.value()->rotate()->angle(),
                                   v.value()->rotate()->cx().fixed(),
                                   v.value()->rotate()->cy().fixed());
            } else if (v.value()->type() == StyleTransformData::Scale) {
                newItem->setScale(v.value()->scale()->x(),
                                  v.value()->scale()->y());
            } else if (v.value()->type() == StyleTransformData::Translate) {
                newItem->setTranslate(v.value()->translate()->m_tx.fixed(),
                                      v.value()->translate()->m_ty.fixed());
            } else {
                STARFISH_ASSERT(v.value()->type() == StyleTransformData::Skew);
                newItem->setSkewX(v.value()->skew()->angleX());
                newItem->setSkewY(v.value()->skew()->angleY());
            }
            return newItem;
        }
        return m_sourceObject->getItem(index);
    } else {
        return m_v.at(index);
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

    m_sourceElement->updateSVGAttributeNeeded(m_targetAttribute);

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
    m_v.erase((size_t)index);
    m_v.insert(index, item);

    m_sourceElement->updateSVGAttributeNeeded(m_targetAttribute);

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
    m_sourceElement->updateSVGAttributeNeeded(m_targetAttribute);

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

    m_sourceElement->updateSVGAttributeNeeded(m_targetAttribute);

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
        GC_set_bit(desc, GC_WORD_OFFSET(SVGTransformList, m_sourceElement));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGTransformList, m_sourceObject));
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
            STARFISH_ASSERT_NOT_REACHED();
        }

        for (size_t i = 0; i < tokens.size(); ++i) {
            CSSTransformFunction f = value->transformValue()->at(i);
            SVGTransform* newItem = new SVGTransform(
                m_sourceElement, AtomicString::emptyAtomicString(), f,
                isReadOnly());

            appendItemWithoutUpdateAttribute(newItem);
        }
    }
}

bool SVGTransformList::isReadOnly()
{
    return m_readOnly;
}

String* SVGTransformList::toString()
{
    StringBuilder sb;

    for (size_t i = 0; i < length(); ++i) {
        sb.appendString(getItem(i)->toString());
        if (i < length() - 1) {
            sb.appendChar(' ');
        }
    }

    return sb.finalize();
}
} // namespace Starfish
