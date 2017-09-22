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

#ifndef __StarFishTraverse__
#define __StarFishTraverse__

namespace StarFish {
class Traverse {
    Traverse()
    {
    }

public:
    template <typename Func>
    static Node* findDescendant(Node* parent, Func matchingRule)
    {
        Node* child = parent->firstChild();
        while (child) {
            if (matchingRule(child)) {
                return child;
            } else {
                Node* matchedDescendant = findDescendant(child, matchingRule);
                if (matchedDescendant) {
                    return matchedDescendant;
                }
            }
            child = child->nextSibling();
        }
        return nullptr;
    }

    template <typename Coll, typename Func>
    static bool collectDescendants(Coll& collection, Node* root, Func filter,
                                   bool shouldOnlyMatchFirstElement)
    {
        Node* child = root->firstChild();
        while (child) {
            if (child->isElement()) {
                Element* elm = child->asElement();
                if (filter(elm)) {
                    collection.push_back(child->asElement());
                    if (shouldOnlyMatchFirstElement) {
                        return true;
                    }
                }
            }

            if (collectDescendants(collection, child, filter,
                                   shouldOnlyMatchFirstElement)) {
                return true;
            }
            child = child->nextSibling();
        }

        return false;
    }

    template <typename Func>
    static Node* firstChild(Node* parent, Func matchingRule)
    {
        Node* child = parent->firstChild();
        while (child) {
            if (matchingRule(child)) {
                return child;
            } else {
                child = child->nextSibling();
            }
        }
        return nullptr;
    }

    static Node* firstChild(Node* parent)
    {
        Node* child = parent->firstChild();
        while (child) {
            if (child->isElement()) {
                return child;
            } else {
                child = child->nextSibling();
            }
        }
        return nullptr;
    }

    template <typename Func>
    static Node* lastChild(Node* parent, Func matchingRule)
    {
        Node* child = parent->lastChild();
        while (child) {
            if (matchingRule(child)) {
                return child;
            } else {
                child = child->previousSibling();
            }
        }
        return nullptr;
    }

    static Node* nextAncestorSibling(const Node* current,
                                     const Node* stayWithin)
    {
        STARFISH_ASSERT(!current->nextSibling());
        STARFISH_ASSERT(current != stayWithin);
        for (Node* parent = current->parentNode(); parent;
             parent = parent->parentNode()) {
            if (parent == stayWithin) {
                return 0;
            }
            if (parent->nextSibling()) {
                return parent->nextSibling();
            }
        }
        return 0;
    }

    static Node* next(Node* current, const Node* stayWithin)
    {
        if (current->hasChildNodes()) {
            return current->firstChild();
        }
        if (current == stayWithin) {
            return 0;
        }
        if (current->nextSibling()) {
            return current->nextSibling();
        }
        return nextAncestorSibling(current, stayWithin);
    }

    static Element* nextElement(Node* current, const Node* stayWithin)
    {
        Node* node = next(current, stayWithin);
        while (node && !node->isElement())
            node = next(node, stayWithin);
        if (node) {
            return node->asElement();
        }
        return nullptr;
    }

    static Node* nextSkippingChildren(Node* current, const Node* stayWithin)
    {
        if (current == stayWithin) {
            return 0;
        }
        if (current->nextSibling()) {
            return current->nextSibling();
        }
        return nextAncestorSibling(current, stayWithin);
    }

    static Element* nextSkippingChildrenElement(Node* current,
                                                const Node* stayWithin)
    {
        Node* node = nextSkippingChildren(current, stayWithin);
        while (node && !node->isElement()) {
            node = nextSkippingChildren(node, stayWithin);
        }
        if (node) {
            return node->asElement();
        }
        return nullptr;
    }
};
}

#endif
