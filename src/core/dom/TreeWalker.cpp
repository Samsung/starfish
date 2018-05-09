/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "core/dom/TreeWalker.h"
#include "core/dom/NodeFilter.h"
#include "core/dom/Traverse.h"
#include "core/page/Window.h"

namespace StarFish {

ScriptValue TreeWalker::filter()
{
    if (!m_filter) {
        return scriptNull();
    }

    return m_filter;
}

Node* TreeWalker::parentNode()
{
    Node* node = m_current;

    while (node != root()) {
        node = node->parentNode();
        if (!node) {
            return nullptr;
        }

        bool result = false;
        unsigned acceptNodeResult = acceptNode(node, result);
        if (result) {
            return nullptr;
        }

        if (acceptNodeResult == NodeFilter::FILTERACCEPT) {
            m_current = node;
            return m_current;
        }
    }

    return nullptr;
}

Node* TreeWalker::firstChild()
{
    for (Node* node = m_current->firstChild(); node;) {
        bool result = false;
        unsigned acceptNodeResult = acceptNode(node, result);
        if (result) {
            return nullptr;
        }

        switch (acceptNodeResult) {
        case NodeFilter::FILTERACCEPT:
            m_current = node;
            return m_current;
            break;
        case NodeFilter::FILTERSKIP:
            if (node->hasChildNodes()) {
                node = node->firstChild();
                continue;
            }
        case NodeFilter::FILTERREJECT:
            break;
        default:
            return nullptr;
        }
        do {
            if (node->nextSibling()) {
                node = node->nextSibling();
                break;
            }

            Node* parent = node->parentNode();
            if (!parent || parent == root() || parent == m_current) {
                return nullptr;
            }
            node = parent;
        } while (node);
    }

    return nullptr;
}

Node* TreeWalker::lastChild()
{
    for (Node* node = m_current->lastChild(); node;) {
        bool result = false;
        unsigned acceptNodeResult = acceptNode(node, result);
        if (result) {
            return nullptr;
        }

        switch (acceptNodeResult) {
        case NodeFilter::FILTERACCEPT:
            m_current = node;
            return m_current;
            break;
        case NodeFilter::FILTERSKIP:
            if (node->lastChild()) {
                node = node->lastChild();
                continue;
            }
        case NodeFilter::FILTERREJECT:
            break;
        default:
            return nullptr;
        }
        do {
            if (node->previousSibling()) {
                node = node->previousSibling();
                break;
            }

            Node* parent = node->parentNode();
            if (!parent || parent == root() || parent == m_current) {
                return nullptr;
            }
            node = parent;
        } while (node);
    }

    return nullptr;
}

Node* TreeWalker::previousNode()
{
    Node* node = m_current;
    while (node != root()) {
        while (Node* previousSibling = node->previousSibling()) {
            node = previousSibling;
            bool result = false;
            unsigned acceptNodeResult = acceptNode(node, result);
            if (result) {
                return nullptr;
            }

            if (acceptNodeResult == NodeFilter::FILTERREJECT) {
                continue;
            }

            while (Node* lastChild = node->lastChild()) {
                node = lastChild;
                acceptNodeResult = acceptNode(node, result);
                if (result) {
                    return nullptr;
                }

                if (acceptNodeResult == NodeFilter::FILTERREJECT) {
                    break;
                }
            }

            if (acceptNodeResult == NodeFilter::FILTERACCEPT) {
                m_current = node;
                return m_current;
            }
        }

        if (node == root()) {
            return nullptr;
        }

        Node* parent = node->parentNode();

        if (!parent) {
            return nullptr;
        }

        node = parent;
        bool result = false;
        unsigned acceptNodeResult = acceptNode(node, result);
        if (result) {
            return nullptr;
        }

        if (acceptNodeResult == NodeFilter::FILTERACCEPT) {
            m_current = node;
            return m_current;
        }
    }

    return nullptr;
}

Node* TreeWalker::nextNode()
{
    Node* node = m_current;
Again:
    while (Node* firstChild = node->firstChild()) {
        node = firstChild;
        bool result = false;
        unsigned acceptNodeResult = acceptNode(node, result);
        if (result) {
            return nullptr;
        }

        if (acceptNodeResult == NodeFilter::FILTERACCEPT) {
            m_current = node;
            return m_current;
        }
        if (acceptNodeResult == NodeFilter::FILTERREJECT) {
            break;
        }
    }

    while (Node* nextSibling = Traverse::nextSkippingChildren(node, root())) {
        node = nextSibling;
        bool result = false;
        unsigned acceptNodeResult = acceptNode(node, result);
        if (result) {
            return nullptr;
        }
        if (acceptNodeResult == NodeFilter::FILTERACCEPT) {
            m_current = node;
            return m_current;
        }
        if (acceptNodeResult == NodeFilter::FILTERSKIP) {
            goto Again;
        }
    }

    return nullptr;
}

template <typename Strategy>
Node* TreeWalker::TraverseSiblings()
{
    Node* node = m_current;

    if (node == root()) {
        return nullptr;
    }

    while (true) {
        Node* sibling = Strategy::nextNode(*node);
        while (sibling) {
            node = sibling;
            bool result = false;
            unsigned acceptNodeResult = acceptNode(node, result);
            if (result) {
                return nullptr;
            }

            if (acceptNodeResult == NodeFilter::FILTERACCEPT) {
                m_current = node;
                return m_current;
            }

            sibling = Strategy::startNode(*sibling);
            if (acceptNodeResult == NodeFilter::FILTERREJECT || !sibling) {
                sibling = Strategy::nextNode(*node);
            }
        }

        node = node->parentNode();

        if (!node || node == root()) {
            return nullptr;
        }

        bool result = false;
        unsigned acceptNodeResult = acceptNode(node, result);
        if (result) {
            return nullptr;
        }

        if (acceptNodeResult == NodeFilter::FILTERACCEPT) {
            return nullptr;
        }
    }
}

Node* TreeWalker::previousSibling()
{
    return TraverseSiblings<PreviousNodeTraversalStrategy>();
}

Node* TreeWalker::nextSibling()
{
    return TraverseSiblings<NextNodeTraversalStrategy>();
}

unsigned TreeWalker::acceptNode(Node* node, bool& error)
{
    if (!node) {
        return NodeFilter::FILTERREJECT;
    }

    unsigned type = node->nodeType();
    if (!(((1 << (type - 1)) & m_whatToShow))) {
        return NodeFilter::FILTERSKIP;
    }

    if (!m_filter || isNullOrUndefinedScriptValue(m_filter)) {
        return NodeFilter::FILTERACCEPT;
    }

    if (!scriptBindingInstance()) {
        return NodeFilter::FILTERREJECT;
    }

    if (isCallableScriptValue(m_filter)) {
        ScriptValue* argv;
        argv = (ScriptValue*)alloca(sizeof(ScriptValue) * 1);
        argv[0] = node->scriptValue();
        ScriptValue thisValue = node->window()->scriptValue();
        ScriptValue ret = callScriptFunctionWithError(
            scriptBindingInstance(), m_filter, argv, 1, thisValue, error);

        if (isBooleanScriptValue(ret)) {
            if (scriptValueAsBoolean(ret)) {
                return NodeFilter::FILTERACCEPT;
            } else {
                return NodeFilter::FILTERREJECT;
            }
        } else if (isNumberScriptValue(ret)) {
            unsigned number = scriptValueAsNumber(ret);
            if (number == NodeFilter::FILTERACCEPT ||
                number == NodeFilter::FILTERREJECT ||
                number == NodeFilter::FILTERSKIP) {
                return number;
            } else {
                return NodeFilter::FILTERREJECT;
            }
        } else if (!ret || isNullOrUndefinedScriptValue(ret)) {
            return NodeFilter::FILTERREJECT;
        }
    } else if (isObjectScriptValue(m_filter)) {
        ScriptValue* argv;
        argv = (ScriptValue*)alloca(sizeof(ScriptValue) * 1);
        argv[0] = node->scriptValue();
        ScriptValue thisValue = node->window()->scriptValue();
        ScriptValue ret = callHandleNodeFilterFunction(
            scriptBindingInstance(), m_filter, argv, 1, thisValue, error);
        if (isBooleanScriptValue(ret)) {
            if (scriptValueAsBoolean(ret)) {
                return NodeFilter::FILTERACCEPT;
            } else {
                return NodeFilter::FILTERREJECT;
            }
        } else if (isNumberScriptValue(ret)) {
            unsigned number = scriptValueAsNumber(ret);
            if (number == NodeFilter::FILTERACCEPT ||
                number == NodeFilter::FILTERREJECT ||
                number == NodeFilter::FILTERSKIP) {
                return number;
            } else {
                return NodeFilter::FILTERREJECT;
            }
        } else if (!ret || isNullOrUndefinedScriptValue(ret)) {
            return NodeFilter::FILTERREJECT;
        }
    }

    return NodeFilter::FILTERREJECT;
}
}
