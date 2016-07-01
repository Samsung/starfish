/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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
    typedef std::vector<Node*, gc_allocator<Node*>> NodeCollection;

    template<typename Func>
    static Node* findDescendant(Node* parent, Func matchingRule)
    {
        Node* child = parent->firstChild();
        while (child) {
            if (matchingRule(child))
                return child;
            else {
                Node* matchedDescendant = findDescendant(child, matchingRule);
                if (matchedDescendant)
                    return matchedDescendant;
            }
            child = child->nextSibling();
        }
        return nullptr;
    }

    template<typename Func>
    static void getherDescendant(std::vector<Node*, gc_allocator<Node*>>* collection, Node* root, Func filter)
    {
        Node* child = root->firstChild();
        while (child) {
            if (filter(child))
                collection->push_back(child);

            getherDescendant(collection, child, filter);
            child = child->nextSibling();
        }
    }

    template<typename Func>
    static Node* firstChild(Node* parent, Func matchingRule)
    {
        Node* child = parent->firstChild();
        while (child) {
            if (matchingRule(child))
                return child;
            else
                child = child->nextSibling();
        }
        return nullptr;
    }

    template<typename Func>
    static Node* lastChild(Node* parent, Func matchingRule)
    {
        Node* child = parent->lastChild();
        while (child) {
            if (matchingRule(child))
                return child;
            else
                child = child->previousSibling();
        }
        return nullptr;
    }

    template<typename Func>
    static Node* nextSibling(Node* start, Func matchingRule)
    {
        Node* sibling = start->nextSibling();
        while (sibling) {
            if (matchingRule(sibling))
                return sibling;
            else
                sibling = sibling->nextSibling();
        }
        return nullptr;
    }

    template<typename Func>
    static Node* previousSibling(Node* start, Func matchingRule)
    {
        Node* sibling = start->previousSibling();
        while (sibling) {
            if (matchingRule(sibling))
                return sibling;
            else
                sibling = sibling->previousSibling();
        }
        return nullptr;
    }

    template<typename Func>
    static NodeCollection* nextSiblings(Node* start, Func matchingRule)
    {
        auto siblings = new NodeCollection();
        for (Node* sibling = start->nextSibling(); sibling; sibling = sibling -> nextSibling()) {
            if (matchingRule(sibling)) {
                siblings->push_back(sibling);
            }
        }
        return siblings;
    }

    template<typename Func>
    static NodeCollection* previousSiblings(Node* start, Func matchingRule)
    {
        auto siblings = new NodeCollection();
        for (Node* sibling = start->previousSibling(); sibling; sibling = sibling -> previousSibling()) {
            if (matchingRule(sibling)) {
                siblings->push_back(sibling);
            }
        }
        std::reverse(siblings->begin(), siblings->end());
        return siblings;
    }

    template<typename Func>
    static unsigned long childCount(Node* parent, Func matchingRule)
    {
        unsigned long count = 0;
        Node* child = parent->firstChild();
        while (child) {
            if (matchingRule(child))
                count++;
            child = child->nextSibling();
        }
        return count;
    }
};

}

#endif
