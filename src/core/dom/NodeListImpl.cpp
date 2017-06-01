/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLElement.h"
#include "core/dom/Node.h"
#include "core/dom/NodeListImpl.h"

namespace StarFish {

bool isChildNode(Node* node, void* data)
{
    Node* parent = (Node*)data;
    return node->parentNode() == data;
};

bool isChildElement(Node* node, void* data)
{
    Node* parent = (Node*)data;
    return node->parentNode() == parent && node->isElement();
};

bool isSameTagName(Node* node, void* data)
{
    QualifiedName* tagName = (QualifiedName*)data;
    if (node->isElement()) {
        if (node->asElement()->name().localNameAtomic() ==
            tagName->localNameAtomic()) {
            return true;
        }
        if (tagName->localName()->equals("*")) {
            return true;
        }
    }
    return false;
};

bool hasClassNames(Node* node, void* data)
{
    String* classNames = (String*)data;
    if (node->isHTMLElement()) {
        HTMLElement* htmlElement = node->asHTMLElement();
        if (htmlElement->classNames().size() > 0) {
            size_t length = classNames->length();
            bool isWhiteSpaceState = true;

            UTF32String str;
            for (size_t i = 0; i < length; i++) {
                char32_t ch = classNames->charAt(i);
                if (isWhiteSpaceState) {
                    if (ch != ' ' && ch != '\n' && ch != '\t' && ch != '\f' &&
                        ch != '\r') {
                        isWhiteSpaceState = false;
                        str += ch;
                    }
                } else {
                    if (ch == ' ' || ch == '\n' || ch == '\t' || ch == '\f' ||
                        ch == '\r') {
                        isWhiteSpaceState = true;

                        String* tok = new StringDataUTF32(std::move(str));

                        if (!htmlElement->hasClassName(tok)) {
                            return false;
                        }

                        str.clear();
                    } else {
                        str += ch;
                    }
                }
            }

            if (str.length()) {
                String* tok = new StringDataUTF32(std::move(str));
                if (!htmlElement->hasClassName(tok)) {
                    return false;
                }
            }

            return true;
        }
    }

    return false;
};

// https://html.spec.whatwg.org/multipage/browsers.html#named-access-on-the-window-object
bool isSameNamedAccess(Node* node, void* data)
{
    QualifiedName* namedAccess = (QualifiedName*)data;

    if (node->isHTMLElement()) {
        // a, applet, area, embed, form, frameset, img, or object elements
        // that have a name content attribute whose value is name, or
        HTMLElement* htmlElement = node->asHTMLElement();
        QualifiedName name = htmlElement->name();
        StaticStrings* ss = node->starFish()->staticStrings();
        bool shouldConsiderNameAttribute = false;
        if (name == ss->m_aTagName) {
            shouldConsiderNameAttribute = true;
        } else if (name == ss->m_areaTagName) {
            shouldConsiderNameAttribute = true;
        } else if (name == ss->m_embedTagName) {
            shouldConsiderNameAttribute = true;
        } else if (name == ss->m_formTagName) {
            shouldConsiderNameAttribute = true;
        } else if (name == ss->m_framesetTagName) {
            shouldConsiderNameAttribute = true;
        } else if (name == ss->m_imgTagName) {
            shouldConsiderNameAttribute = true;
        } else if (name == ss->m_objectTagName) {
            shouldConsiderNameAttribute = true;
        }

        if (shouldConsiderNameAttribute) {
            if (htmlElement->getAttributeOrEmpty(ss->m_name)
                    ->equals(namedAccess->localName())) {
                return true;
            }
        }

        // HTML elements that have an id content attribute whose value is
        // name
        if (htmlElement->hasId() &&
            htmlElement->atomicId() == namedAccess->localNameAtomic()) {
            return true;
        }
    }
    return false;
};

void NodeListImpl::getherDescendant(GCVector<Node*>* collection,
                                    Node* root) const
{
    STARFISH_ASSERT(m_filter);
    Node* child = root->firstChild();
    while (child) {
        if (m_filter(child, m_data)) {
            collection->push_back(child);
        }

        getherDescendant(collection, child);
        child = child->nextSibling();
    }
}

size_t NodeListImpl::length() const
{
    if (m_canCache) {
        fillCacheIfNeed();
        return m_cachedNodeList.size();
    }
    GCVector<Node*> collection;
    getherDescendant(&collection, m_root);
    return collection.size();
}

Node* NodeListImpl::item(uint32_t index) const
{
    if (m_canCache) {
        fillCacheIfNeed();
        if (index < m_cachedNodeList.size()) {
            return m_cachedNodeList[index];
        }
    } else {
        GCVector<Node*> collection;
        getherDescendant(&collection, m_root);
        if (index < collection.size()) {
            return collection[index];
        }
    }
    return nullptr;
}

void NodeListImpl::fillCacheIfNeed() const
{
    STARFISH_ASSERT(m_canCache);
    if (!m_isCacheValid) {
        getherDescendant(&m_cachedNodeList, m_root);
        m_isCacheValid = true;
    }
}

void NodeListImpl::setItems(GCVector<Element*>& elements)
{
    m_cachedNodeList.insert(m_cachedNodeList.end(), elements.begin(),
                            elements.end());
}
}
