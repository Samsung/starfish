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
#include "core/dom/NodeIterator.h"
#include "core/dom/NodeFilter.h"
#include "core/page/Window.h"
#include "core/dom/Traverse.h"

namespace StarFish {

NodeIterator::NodePointer::NodePointer(Node* n, bool b)
    : m_node(n)
    , m_isPointerBeforeNode(b)
{
}

void NodeIterator::NodePointer::clear()
{
    m_node = nullptr;
}

bool NodeIterator::NodePointer::moveToNext(Node* root)
{
    if (!m_node) {
        return false;
    }

    if (!root) {
        return false;
    }

    if (m_isPointerBeforeNode) {
        m_isPointerBeforeNode = false;
        return true;
    }

    m_node = Traverse::next(m_node, root);

    if (m_node) {
        return true;
    }

    return false;
}

bool NodeIterator::NodePointer::moveToPrevious(Node* root)
{
    if (!m_node) {
        return false;
    }

    if (!root) {
        return false;
    }

    if (!m_isPointerBeforeNode) {
        m_isPointerBeforeNode = true;
        return true;
    }

    m_node = Traverse::previous(m_node, root);

    if (m_node) {
        return true;
    }

    return false;
}

NodeIterator::NodeIterator(Document* document, Node* root, unsigned whatToShow,
                           ScriptValue filter)
    : ScriptWrappable(this)
    , m_scriptBindingInstance(document->scriptBindingInstance())
    , m_root(root)
    , m_referenceNode(root, true)
    , m_whatToShow(whatToShow)
    , m_filter(filter)
{
    if (document && !root->isAttr()) {
        document->attachNodeIterator(this);
    }
}

ScriptValue NodeIterator::filter()
{
    if (!m_filter) {
        return scriptNull();
    }

    return m_filter;
}

void NodeIterator::willNodeBeRemoved(Node* removedNode)
{
    updateForNodeRemoval(removedNode, m_candidateNode);
    updateForNodeRemoval(removedNode, m_referenceNode);
}

void NodeIterator::updateForNodeRemoval(Node* removedNode,
                                        NodePointer& referenceNode)
{
    if (!removedNode) {
        return;
    }

    if (!removedNode->isDescendantOf(root())) {
        return;
    }

    bool willRemoveReferenceNode = removedNode == referenceNode.m_node;
    bool willRemoveReferenceNodeAncestor =
        referenceNode.m_node &&
        referenceNode.m_node->isDescendantOf(removedNode);

    if (!willRemoveReferenceNode && !willRemoveReferenceNodeAncestor) {
        return;
    }

    if (referenceNode.m_isPointerBeforeNode) {
        Node* node = Traverse::next(removedNode, root());
        if (node) {
            while (node && node->isDescendantOf(removedNode)) {
                node = Traverse::next(node, root());
            }

            if (node) {
                referenceNode.m_node = node;
            }
        } else {
            node = Traverse::previous(removedNode, root());
            ;
            if (node) {
                if (willRemoveReferenceNodeAncestor) {
                    while (node && node->isDescendantOf(removedNode)) {
                        node = Traverse::previous(node, root());
                    }
                }

                if (node) {
                    referenceNode.m_node = node;
                    referenceNode.m_isPointerBeforeNode = false;
                }
            }
        }
    } else {
        Node* node = Traverse::previous(removedNode, root());
        if (node) {
            if (willRemoveReferenceNodeAncestor) {
                while (node && node->isDescendantOf(removedNode)) {
                    node = Traverse::previous(node, root());
                }
            }

            if (node) {
                referenceNode.m_node = node;
            }
        } else {
            node = Traverse::next(removedNode, root());
            if (willRemoveReferenceNodeAncestor) {
                while (node && node->isDescendantOf(removedNode)) {
                    node = Traverse::previous(node, root());
                }
            }

            if (node) {
                referenceNode.m_node = node;
            }
        }
    }
}

Node* NodeIterator::nextNode()
{
    Node* result = nullptr;
    m_candidateNode = m_referenceNode;

    while (m_candidateNode.moveToNext(root())) {
        Node* candidateResult = m_candidateNode.m_node;
        bool result = false;
        bool accepted =
            acceptNode(candidateResult, result) == NodeFilter::FILTERACCEPT;

        if (result) {
            break;
        }

        if (accepted) {
            m_referenceNode = m_candidateNode;
            result = candidateResult;
            break;
        }
    }

    m_candidateNode.clear();

    return result;
}

Node* NodeIterator::previousNode()
{
    Node* result = nullptr;
    m_candidateNode = m_referenceNode;

    while (m_candidateNode.moveToPrevious(root())) {
        Node* candidateResult = m_candidateNode.m_node;
        bool result = false;
        bool accepted =
            acceptNode(candidateResult, result) == NodeFilter::FILTERACCEPT;

        if (result) {
            break;
        }

        if (accepted) {
            m_referenceNode = m_candidateNode;
            result = candidateResult;
            break;
        }
    }

    m_candidateNode.clear();

    return result;
}

void NodeIterator::detach()
{
    // This operation is a no-op. It doesn't do anything.
    // Previously it was telling the engine that the NodeIterator was no more
    // used,
    // but this is now useless.
}

unsigned NodeIterator::acceptNode(Node* node, bool& error)
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
