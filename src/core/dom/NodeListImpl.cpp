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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLElement.h"
#include "core/dom/Node.h"
#include "core/dom/NodeListImpl.h"
#include "core/dom/HTMLInputElement.h"
#include "core/dom/HTMLOptionElement.h"
#include "core/dom/HTMLLabelElement.h"

namespace StarFish {

bool isChildNode(Node* node, void* data, GCVector<Node*>* collection)
{
    Node* parent = (Node*)data;
    return node->parentNode() == data;
};

bool isChildElement(Node* node, void* data, GCVector<Node*>* collection)
{
    Node* parent = (Node*)data;
    return node->parentNode() == parent && node->isElement();
};

bool isSameTagName(Node* node, void* data, GCVector<Node*>* collection)
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

bool isSameTagNameNS(Node* node, void* data, GCVector<Node*>* collection)
{
    QualifiedName* tagNameNS = (QualifiedName*)data;
    if (node->isElement()) {
        if (tagNameNS->namespaceURI().getValue().string()->equals("*")) {
            if (tagNameNS->localName()->equals("*")) {
                return true;
            }

            if (tagNameNS->equalsLocalName(node->asElement()->name())) {
                return true;
            }
        } else {
            if (tagNameNS->localName()->equals("*")) {
                if (tagNameNS->equalsNamespace(node->asElement()->name())) {
                    return true;
                }
            }

            if (node->asElement()->name() == *tagNameNS) {
                return true;
            }
        }
    }
    return false;
};

bool hasClassNames(Node* node, void* data, GCVector<Node*>* collection)
{
    String* classNames = (String*)data;
    if (node->isElement()) {
        Element* element = node->asElement();
        if (element->classNames().size() > 0) {
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

                        if (!element->hasClassName(tok)) {
                            return false;
                        }

                        str.clear();
                    } else {
                        str += ch;
                    }
                }
            }

            if (str.length()) {
                StringDataOnStackUTF32 tok(str.data(), str.length());
                if (!element->hasClassName(&tok)) {
                    return false;
                }
            }

            return true;
        }
    }

    return false;
};

// https://html.spec.whatwg.org/multipage/browsers.html#named-access-on-the-window-object
bool isSameNamedAccess(Node* node, void* data, GCVector<Node*>* collection)
{
    QualifiedName* namedAccess = (QualifiedName*)data;

    if (node->isElement()) {
        // a, applet, area, embed, form, frameset, img, or object elements
        // that have a name content attribute whose value is name, or

        // https://html.spec.whatwg.org/multipage/forms.html#form-associated-element
        // form-associated elements : button, fieldset, input, object, output,
        // select, textarea, img
        Element* element = node->asElement();
        QualifiedName name = element->name();
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
        } else if (name == ss->m_buttonTagName) {
            shouldConsiderNameAttribute = true;
        } else if (name == ss->m_inputTagName) {
            shouldConsiderNameAttribute = true;
        } else if (name == ss->m_outputTagName) {
            shouldConsiderNameAttribute = true;
        } else if (name == ss->m_selectTagName) {
            shouldConsiderNameAttribute = true;
        } else if (name == ss->m_textareaTagName) {
            shouldConsiderNameAttribute = true;
        }

        if (shouldConsiderNameAttribute) {
            Nullable<String*> attr = element->getAttribute(ss->m_name);
            if (attr.hasValue() &&
                attr.getValue()->equals(namedAccess->localName())) {
                return true;
            }
        }

        // HTML elements that have an id content attribute whose value is
        // name
        if (element->hasId() &&
            element->atomicId() == namedAccess->localNameAtomic()) {
            return true;
        }
    }
    return false;
};

struct TableRowsCollectionData : public gc {
    Node* root;
    Node* lastNode;
    GCVector<GCVector<Node*>> tag;
};

bool isSameTableElement(Node* node, void* data, GCVector<Node*>* collection)
{
    if (!data) {
        if (node->isHTMLTableRowElement()) {
            return true;
        }
        return false;
    }
    TableRowsCollectionData* tableData = (TableRowsCollectionData*)data;
    if (tableData->lastNode == nullptr) {
        for (Node* mv = node; mv; mv = mv->nextSibling()) {
            if (mv->isHTMLTableRowElement()) {
                tableData->lastNode = mv;
            } else if (mv->isHTMLTableSectionElement()) {
                tableData->lastNode = mv;
            }
        }
        if (tableData->lastNode == nullptr) {
            return false;
        }
        if (tableData->lastNode->isHTMLTableSectionElement()) {
            for (Node* mv = tableData->lastNode->firstChild(); mv;
                 mv = mv->nextSibling()) {
                if (mv->isHTMLTableRowElement()) {
                    tableData->lastNode = mv;
                }
            }
        }
    }

    if (node->isEqualNode(tableData->lastNode)) {
        tableData->tag[2].push_back(node);

        collection->insert(collection->end(), tableData->tag[0].begin(),
                           tableData->tag[0].end());
        collection->insert(collection->end(), tableData->tag[1].begin(),
                           tableData->tag[1].end());
        collection->insert(collection->end(), tableData->tag[2].begin(),
                           tableData->tag[2].end());
        collection->insert(collection->end(), tableData->tag[3].begin(),
                           tableData->tag[3].end());
        tableData->tag[0].clear(); // thead tr
        tableData->tag[1].clear(); // tbody tr
        tableData->tag[2].clear(); // tr
        tableData->tag[3].clear(); // tfoot tr
        tableData->lastNode = nullptr;
    } else if (node->isHTMLTableRowElement()) {
        if (node->parentNode()->isHTMLTableSectionElement() &&
            node->parentNode()->parentNode()->isEqualNode(tableData->root)) {
            HTMLElement* htmlElement = node->parentNode()->asHTMLElement();
            QualifiedName name = htmlElement->name();
            StaticStrings* ss = node->starFish()->staticStrings();
            if (name == ss->m_theadTagName) {
                tableData->tag[0].push_back(node);
            } else if (name == ss->m_tbodyTagName) {
                tableData->tag[1].push_back(node);
            } else if (name == ss->m_tfootTagName) {
                tableData->tag[3].push_back(node);
            }
        } else if (node->parentNode()->isHTMLTableElement() &&
                   node->parentNode()->isEqualNode(tableData->root)) {
            tableData->tag[2].push_back(node);
        }
    }

    return false;
};

bool isTBodiesElement(Node* node, void* data, GCVector<Node*>* collection)
{
    if (node->isHTMLTBodyElement()) {
        return true;
    }
    return false;
}

bool isTableCellsElement(Node* node, void* data, GCVector<Node*>* collection)
{
    if (node->isHTMLTableCellElement()) {
        return true;
    }
    return false;
}

bool isFormElements(Node* node, void* data, GCVector<Node*>* collection)
{
    // https://html.spec.whatwg.org/#dom-form-elements
    // Filter matches listed elements whose form owner is the form element
    // Listed elements is button,fieldset,input,object,output,select,textarea
    if (node->isHTMLElement()) {
        if (node->isHTMLFormControl() &&
            node->asHTMLFormControl()->isListedElement()) {
            return true;
        }
    }
    return false;
}

bool isSelectedOption(Node* node, void* data, GCVector<Node*>* collection)
{
    if (node->isHTMLOptionElement()) {
        if (node->asHTMLOptionElement()->selectedness()) {
            return true;
        }
    }
    return false;
}

bool isOptionElement(Node* node, void* data, GCVector<Node*>* collection)
{
    if (node->isHTMLOptionElement()) {
        return true;
    }
    return false;
}

bool isMapAreasElement(Node* node, void* data, GCVector<Node*>* collection)
{
    if (node->isHTMLAreaElement()) {
        return true;
    }
    return false;
}

bool isAssociatedLabelElement(Node* node, void* data,
                              GCVector<Node*>* collection)
{
    if (node->isHTMLLabelElement()) {
        if (node->asHTMLLabelElement()->control() == (HTMLElement*)data) {
            return true;
        }
    }
    return false;
}

void NodeListImpl::getherDescendant(GCVector<Node*>* collection,
                                    Node* root) const
{
    STARFISH_ASSERT(m_filter);
    Node* child = root->firstChild();
    while (child) {
        if (m_filter(child, m_data, collection)) {
            collection->push_back(child);
        }

        getherDescendant(collection, child);
        child = child->nextSibling();
    }
}

void NodeListImpl::getherDescendantIncludingRoot(GCVector<Node*>* collection,
                                                 Node* root) const
{
    STARFISH_ASSERT(m_filter && root);
    if (m_filter(root, m_data, collection)) {
        collection->push_back(root);
    }

    getherDescendant(collection, root);
}

size_t NodeListImpl::length() const
{
    if (m_canCache) {
        fillCacheIfNeed();
        return m_cachedNodeList.size();
    }
    GCVector<Node*> collection;
    if (m_includeRoot) {
        getherDescendantIncludingRoot(&collection, m_root);
    } else {
        getherDescendant(&collection, m_root);
    }
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
        if (m_includeRoot) {
            getherDescendantIncludingRoot(&collection, m_root);
        } else {
            getherDescendant(&collection, m_root);
        }
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
        if (m_includeRoot) {
            getherDescendantIncludingRoot(&m_cachedNodeList, m_root);
        } else {
            getherDescendant(&m_cachedNodeList, m_root);
        }
        m_isCacheValid = true;
    }
}

void NodeListImpl::setItems(GCVector<Element*>& elements)
{
    m_cachedNodeList.insert(m_cachedNodeList.end(), elements.begin(),
                            elements.end());
}
}
