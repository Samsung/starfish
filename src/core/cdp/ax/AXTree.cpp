/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_CDP)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "AXTree.h"
#include "AXName.h"
#include "AXRole.h"
#include "../NodeRegistry.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLDialogElement.h"
#include "core/dom/Node.h"
#include "core/dom/ShadowRoot.h"
#include "core/dom/Traverse.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBlockBox.h"
#include "core/style/ComputedStyle.h"

namespace Starfish {

const char* axIgnoredReasonName(AXIgnoredReason reason)
{
    switch (reason) {
    case AXIgnoredReason::ActiveModalDialog:
        return "activeModalDialog";
    case AXIgnoredReason::AriaHiddenElement:
        return "ariaHiddenElement";
    case AXIgnoredReason::AriaHiddenSubtree:
        return "ariaHiddenSubtree";
    case AXIgnoredReason::EmptyAlt:
        return "emptyAlt";
    case AXIgnoredReason::EmptyText:
        return "emptyText";
    case AXIgnoredReason::NotRendered:
        return "notRendered";
    case AXIgnoredReason::NotVisible:
        return "notVisible";
    case AXIgnoredReason::PresentationalRole:
        return "presentationalRole";
    case AXIgnoredReason::ProbablyPresentational:
        return "probablyPresentational";
    case AXIgnoredReason::Uninteresting:
        return "uninteresting";
    }
    return "uninteresting";
}

Element* AXNode::element() const
{
    return m_domNode && m_domNode->isElement() ? m_domNode->asElement()
                                               : nullptr;
}

AXTree::~AXTree() = default;

AXNode* AXTree::forDOMNode(Node* node) const
{
    auto it = m_byDOMNode.find(node);
    return it == m_byDOMNode.end() ? nullptr : it->second;
}

AXNode* AXTree::forId(const std::string& id) const
{
    auto it = m_byId.find(id);
    return it == m_byId.end() ? nullptr : it->second;
}

namespace {

    // State an AX node inherits from its ancestors. Computing it once on the
    // way down keeps the build a single pass: asking each node to walk back to
    // the root would make the whole build quadratic.
    struct InheritedState {
        // The nearest ancestor-or-self with aria-hidden="true", if any. It is
        // both the reason a node is ignored and the node the reason points at.
        Element* ariaHiddenRoot = nullptr;
        // Set on the node that carries the attribute, so it can report
        // ariaHiddenElement while its descendants report ariaHiddenSubtree.
        bool ariaHiddenSelf = false;
        // The modal dialog that makes everything outside it inert. Null when
        // the node is inside the dialog, or when no dialog is showing.
        Element* blockingModal = nullptr;
    };

    std::string toUTF8(String* s)
    {
        return s ? s->toUTF8NonGCString() : std::string();
    }

    std::string tagOf(Element* element)
    {
        std::string tag = toUTF8(element->localName());
        for (char& c : tag) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return tag;
    }

    bool isTrueAttribute(Element* element, const char* name)
    {
        Optional<String*> value =
            element->getAttribute(String::fromUTF8(name, strlen(name)));
        if (!value.hasValue()) {
            return false;
        }
        std::string text = axCollapseWhitespace(toUTF8(value.value()));
        for (char& c : text) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return text == "true";
    }

    // The accessibility tree never enters these. Chromium prunes the whole
    // subtree instead of reporting ignored nodes for it
    // (IsInPrunableHiddenContainerInclusive in ax_object_cache_impl.cc).
    bool isPrunableHiddenContainer(const std::string& tag)
    {
        return tag == "head" || tag == "style" || tag == "script" ||
               tag == "noframes";
    }

    // An element that shapes layout or another element's content, but never
    // becomes an accessibility node of its own
    // (IsSubtreePrunedForAccessibility in ax_object_cache_impl.cc).
    bool isPrunedElement(Element* element, const std::string& tag)
    {
        if (tag == "map" || tag == "colgroup" || tag == "col" ||
            tag == "title") {
            return true;
        }
        // <area> only means something inside the image map it belongs to.
        if (tag == "area") {
            Node* parent = element->parentNode();
            return !parent || !parent->isElement() ||
                   tagOf(parent->asElement()) != "map";
        }
        return false;
    }

    bool onlyWhitespace(const std::string& text)
    {
        for (char c : text) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                return false;
            }
        }
        return true;
    }

} // namespace

// Turns one document into an AX tree. One instance builds one tree.
class AXTreeBuilder {
public:
    AXTreeBuilder(AXTree* tree, NodeRegistry* registry)
        : m_tree(tree)
        , m_registry(registry)
    {
    }

    void run(Document* document)
    {
        m_tree->m_document = document;

        AXNode* root = create(document, AXNodeKind::Root);
        root->m_role = "RootWebArea";
        root->m_name = axDocumentName(document);
        m_tree->m_root = root;

        InheritedState state;
        state.blockingModal = activeModalDialog(document);
        addChildren(document, root, state);
    }

private:
    // Every id is derived from the backend node id, so it names the same
    // node after a rebuild. The suffix separates the two nodes one text
    // node produces.
    std::string idFor(Node* node, AXNodeKind kind)
    {
        std::string id = "ax-" + std::to_string(m_registry->getOrCreate(node));
        return kind == AXNodeKind::InlineText ? id + "-inline" : id;
    }

    AXNode* create(Node* domNode, AXNodeKind kind)
    {
        std::string id = idFor(domNode, kind);
        m_tree->m_nodes.push_back(
            std::unique_ptr<AXNode>(new AXNode(domNode, kind, id)));
        AXNode* node = m_tree->m_nodes.back().get();
        m_tree->m_byId[id] = node;
        // A text node maps to its StaticText node: the box under it is
        // reachable through the children, never by DOM node.
        if (kind != AXNodeKind::InlineText) {
            m_tree->m_byDOMNode[domNode] = node;
        }
        return node;
    }

    // The dialog that makes the rest of the document inert, or null.
    //
    // The document already answers whether a node is inert, which is true
    // for the document element exactly when a modal dialog is showing. That
    // check is cheap and false on almost every page, so the walk that finds
    // which dialog it is only runs when there is one.
    Element* activeModalDialog(Document* document)
    {
        Optional<Element*> root = document->documentElement();
        if (!root || !document->isInertNode(root.value())) {
            return nullptr;
        }
        Element* found = nullptr;
        Traverse::traverse(document, [&](Node* node) {
            if (node->isElement() && node->asElement()->isHTMLDialogElement() &&
                node->asElement()->asHTMLDialogElement()->isInShowModal()) {
                found = node->asElement();
            }
        });
        return found;
    }

    void addChildren(Node* parentDOMNode, AXNode* parentAXNode,
                     const InheritedState& state)
    {
        Node* child = parentDOMNode->firstRenderingChild();
        RenderingSiblingIterator iterator(child);
        while (Optional<Node*> next = iterator.next()) {
            if (!next.value()) {
                break;
            }
            addSubtree(next.value(), parentAXNode, state);
        }
    }

    void addSubtree(Node* domNode, AXNode* parentAXNode,
                    const InheritedState& parentState)
    {
        if (domNode->nodeType() == Node::TEXT_NODE) {
            addTextNode(domNode, parentAXNode, parentState);
            return;
        }
        if (!domNode->isElement()) {
            return; // comments, doctype, processing instructions
        }

        Element* element = domNode->asElement();
        std::string tag = tagOf(element);
        bool rendered = element->frame() != nullptr;

        // A container the tree never enters prunes itself and everything
        // under it, so no ignored node is reported for any of them.
        if (!rendered && isPrunableHiddenContainer(tag)) {
            return;
        }
        if (isPrunedElement(element, tag)) {
            return;
        }

        InheritedState state = parentState;
        state.ariaHiddenSelf = false;
        if (!state.ariaHiddenRoot && isAriaHiddenRoot(element, tag)) {
            state.ariaHiddenRoot = element;
            state.ariaHiddenSelf = true;
        }
        if (state.blockingModal == element) {
            state.blockingModal = nullptr; // inside the dialog
        }

        AXNode* node = create(element, AXNodeKind::Element);
        node->m_role = axRoleForElement(element, tag);
        computeIgnored(node, element, tag, rendered, state);
        // accname step 2A: a hidden node has no name of its own. It still
        // contributes text when another element points at it, which is
        // computed from the referring element, not from here.
        if (!isHiddenReason(node)) {
            node->m_name = axNameForElement(element, tag, node->m_role);
        }
        attach(node, parentAXNode);

        addChildren(element, node->m_includedInTree ? node : parentAXNode,
                    state);
    }

    void addTextNode(Node* domNode, AXNode* parentAXNode,
                     const InheritedState& state)
    {
        std::string text = axTextForNode(domNode);
        // Collapsible whitespace between blocks produces no box and no
        // accessibility node (IsLayoutTextRelevantForAccessibility).
        if (text.empty() || (!domNode->frame() && onlyWhitespace(text))) {
            return;
        }

        AXNode* node = create(domNode, AXNodeKind::StaticText);
        node->m_role = "StaticText";
        node->m_name = text;
        computeIgnored(node, nullptr, std::string(),
                       domNode->frame() != nullptr, state);
        attach(node, parentAXNode);
        if (!node->m_includedInTree) {
            return;
        }

        // Starfish reports one box per text node. Chromium splits a wrapped
        // text node into one box per line, which is a layout detail this
        // engine does not expose to the protocol.
        AXNode* box = create(domNode, AXNodeKind::InlineText);
        box->m_role = "InlineTextBox";
        box->m_name = text;
        box->m_ignored = node->m_ignored;
        attach(box, node);
    }

    void attach(AXNode* node, AXNode* parent)
    {
        node->m_includedInTree = computeIncludedInTree(node);
        // The parent is recorded either way. A node left out of the tree
        // still reports where it sits, and only misses from its parent's
        // child list.
        node->m_parent = parent;
        if (node->m_includedInTree) {
            parent->m_children.push_back(node);
        }
    }

    // Whether the node was ignored because it is hidden, as opposed to
    // being ignored for carrying no semantics.
    static bool isHiddenReason(AXNode* node)
    {
        for (AXIgnoredReason reason : node->m_ignoredReasons) {
            if (reason == AXIgnoredReason::AriaHiddenElement ||
                reason == AXIgnoredReason::AriaHiddenSubtree ||
                reason == AXIgnoredReason::NotRendered ||
                reason == AXIgnoredReason::NotVisible) {
                return true;
            }
        }
        return false;
    }

    // aria-hidden="true" hides a subtree the way display:none does, but the
    // attribute has no effect on <html>, <body> or <option>.
    bool isAriaHiddenRoot(Element* element, const std::string& tag)
    {
        if (!isTrueAttribute(element, "aria-hidden")) {
            return false;
        }
        return tag != "html" && tag != "body" && tag != "option";
    }

    void computeIgnored(AXNode* node, Element* element, const std::string& tag,
                        bool rendered, const InheritedState& state)
    {
        if (state.ariaHiddenRoot) {
            node->m_ignored = true;
            node->m_ignoredReasons.push_back(
                state.ariaHiddenSelf ? AXIgnoredReason::AriaHiddenElement
                                     : AXIgnoredReason::AriaHiddenSubtree);
            node->m_ignoredReasonTarget = state.ariaHiddenRoot;
            return;
        }
        if (state.blockingModal) {
            node->m_ignored = true;
            node->m_ignoredReasons.push_back(
                AXIgnoredReason::ActiveModalDialog);
            node->m_ignoredReasonTarget = state.blockingModal;
            return;
        }
        if (!rendered) {
            // No box at all means the node was never rendered. A node that
            // has a box but cannot be seen is a different reason, reported
            // below, so the client can tell the two apart.
            node->m_ignored = true;
            node->m_ignoredReasons.push_back(AXIgnoredReason::NotRendered);
            return;
        }
        if (!element) {
            return; // rendered text is never ignored
        }

        ComputedStyle* style = element->style();
        if (style &&
            style->visibility() != VisibilityValue::VisibleVisibilityValue) {
            node->m_ignored = true;
            node->m_ignoredReasons.push_back(AXIgnoredReason::NotVisible);
            return;
        }

        // role="none" and role="presentation" ask for the element's own
        // semantics to be dropped while its content stays.
        if (axHasPresentationalRole(element)) {
            node->m_ignored = true;
            node->m_ignoredReasons.push_back(
                AXIgnoredReason::PresentationalRole);
            return;
        }

        // Everything left without semantics of its own is structure the
        // client has no use for. The exception is a box that lays out
        // inline content directly: that box is what gives the text inside
        // it a place, so it is reported as a generic container. This is
        // the LayoutBlockFlow rule at the end of
        // AXNodeObject::ComputeIsIgnored.
        if (node->m_role.empty() || node->m_role == "generic" ||
            tag == "html" || tag == "body") {
            bool interesting = false;
            if (tag != "html" && tag != "body") {
                // An author who wrote an ARIA attribute, a title or a tab
                // stop meant this element to be reported, whatever its
                // role says.
                interesting = axHasAnyAriaAttribute(element) ||
                              axHasAttribute(element, "title") ||
                              axHasAttribute(element, "tabindex");
                if (!interesting && element->frame() &&
                    element->frame()->isFrameBlockBox()) {
                    FrameBlockBox* box = element->frame()->asFrameBlockBox();
                    interesting = box->firstChild() && !box->hasBlockFlow();
                }
            }
            if (!interesting) {
                node->m_ignored = true;
                node->m_ignoredReasons.push_back(
                    AXIgnoredReason::Uninteresting);
            }
        }
    }

    // An ignored node still appears in the tree when dropping it would lose
    // something the client needs: the shape of the tree, or an element that
    // another element's name is computed from
    // (AXObject::ComputeIsIgnoredButIncludedInTree). Everything else is
    // left out, and its children move up to its parent.
    bool computeIncludedInTree(AXNode* node)
    {
        if (!node->m_ignored) {
            return true;
        }
        if (node->m_kind == AXNodeKind::InlineText) {
            return false;
        }
        Element* element = node->element();
        if (!element) {
            return false;
        }
        std::string tag = tagOf(element);

        // The document element anchors the chain from the root down.
        if (tag == "html") {
            return true;
        }
        // A label is read to name the control it belongs to, so it has to
        // survive even when it carries no semantics of its own.
        if (tag == "label") {
            return true;
        }
        // A table part's role depends on several levels of ancestry, so it
        // would otherwise move in and out of the tree as the page changes.
        if (tag == "table" || tag == "tbody" || tag == "thead" ||
            tag == "tfoot" || tag == "tr" || tag == "td" || tag == "th") {
            return true;
        }
        // An element that states its language carries information even
        // without a role.
        if (axHasAttribute(element, "lang")) {
            return true;
        }
        // A menu is kept while hidden so the client sees it open.
        if (node->m_role == "menu") {
            return true;
        }
        if (Element* parent = element->parentElement()) {
            std::string parentTag = tagOf(parent);
            // Children of a label are part of the name it provides. A
            // <span> is left out: it is uninteresting on its own.
            if (parentTag == "label" && tag != "span") {
                return true;
            }
            // Children of a map are the image's areas, and a ruby
            // annotation is read as the ruby's description.
            if (parentTag == "map" || parentTag == "rt") {
                return true;
            }
        }
        // A visible element that lays out as a block holds the shape of
        // the tree. Dropping it would move its children up one level.
        if (element->frame() && element->frame()->isFrameBlockBox()) {
            return true;
        }
        return false;
    }

    AXTree* m_tree;
    NodeRegistry* m_registry;
};

std::unique_ptr<AXTree> AXTree::build(Document* document,
                                      NodeRegistry* registry)
{
    std::unique_ptr<AXTree> tree(new AXTree());
    AXTreeBuilder builder(tree.get(), registry);
    builder.run(document);
    return tree;
}

} // namespace Starfish

#endif
