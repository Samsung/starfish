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
#include "core/dom/HTMLLabelElement.h"
#include "core/dom/HTMLOptionElement.h"
#include "core/dom/HTMLSelectElement.h"
#include "core/dom/HTMLInputElement.h"
#include "core/dom/Node.h"
#include "core/dom/ShadowRoot.h"
#include "core/dom/Traverse.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBlockBox.h"
#include "core/page/Window.h"
#include "core/dom/DOMRect.h"
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
    case AXIgnoredReason::InertElement:
        return "inertElement";
    case AXIgnoredReason::LabelFor:
        return "labelFor";
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
        // True inside an element that has the inert attribute.
        bool inert = false;
        // Set inside a subtree that content-visibility has locked. Layout
        // skipped it, so its nodes are built from the DOM alone.
        bool displayLocked = false;
        // Set on the element that owns the lock. It is always reported.
        bool displayLockRoot = false;
    };

    constexpr const char* kInertAttribute = "inert";
    constexpr const char* kPopupIdSuffix = "-popup";
    constexpr const char* kMarkerGroupIdSuffix = "-marker-group";
    constexpr const char* kMenuListPopupRole = "MenuListPopup";

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
        // CSS Overflow 5 tabs mode: the element children of a scroller that
        // has a marker group are its slides, and only the active slide is
        // in the accessibility tree.
        Element* tabsScroller = nullptr;
        int activeIndex = -1;
        if (parentDOMNode->isElement()) {
            Element* element = parentDOMNode->asElement();
            ComputedStyle* style = element->style();
            if (style &&
                style->scrollMarkerGroup() !=
                    ScrollMarkerGroupValue::NoneScrollMarkerGroupValue) {
                tabsScroller = element;
                activeIndex = activeSlideIndex(element);
            }
        }

        int slideIndex = 0;
        Node* child = parentDOMNode->firstRenderingChild();
        RenderingSiblingIterator iterator(child);
        while (Optional<Node*> next = iterator.next()) {
            if (!next.value()) {
                break;
            }
            if (tabsScroller) {
                // Only the element children are slides; whitespace between
                // them and generated pseudo-element boxes are not.
                if (next.value()->isPseudoElement() ||
                    !next.value()->isElement()) {
                    continue;
                }
                // Only the active slide is in the accessibility tree; the
                // inactive ones are pruned entirely (CSS Overflow 5 tabs
                // mode -- "Only the active tabpanel is in the accessibility
                // tree").
                if (slideIndex != activeIndex) {
                    slideIndex++;
                    continue;
                }
                addSubtree(next.value(), parentAXNode, state, tabsScroller,
                           true);
                slideIndex++;
            } else {
                addSubtree(next.value(), parentAXNode, state, nullptr, false);
            }
        }
    }

    // The slide a tabs-mode scroller is scrolled to. One slide is one
    // client width of scroll offset, the layout the carousel pattern uses.
    int activeSlideIndex(Element* scroller)
    {
        double width = scroller->clientWidth();
        if (width <= 0) {
            return 0;
        }
        double offset = scroller->scrollLeft(false);
        int index = static_cast<int>(offset / width + 0.5);
        return index < 0 ? 0 : index;
    }

    ContentVisibilityValue contentVisibilityOf(Element* element)
    {
        ComputedStyle* style = element->style();
        return style ? style->usedContentVisibility()
                     : ContentVisibilityValue::VisibleContentVisibilityValue;
    }

    // Chromium locks an auto subtree while it is far from the viewport.
    // Here, outside the viewport is the only condition checked.
    bool isOutsideViewport(Element* element)
    {
        Window* window = element->ownerDocument()->window();
        if (!window) {
            return false;
        }
        DOMRect* rect = element->getBoundingClientRect();
        return rect->y() + rect->height() < 0 ||
               rect->y() > window->innerHeight() ||
               rect->x() + rect->width() < 0 ||
               rect->x() > window->innerWidth();
    }

    void addSubtree(Node* domNode, AXNode* parentAXNode,
                    const InheritedState& parentState,
                    Element* tabsScroller = nullptr, bool tabsActive = false)
    {
        if (domNode->isText()) {
            addTextNode(domNode, parentAXNode, parentState);
            return;
        }
        if (!domNode->isElement()) {
            return; // comments, doctype, processing instructions
        }

        Element* element = domNode->asElement();
        std::string tag = tagOf(element);
        // An element with display: contents has no box but is not hidden.
        bool rendered =
            element->frame() != nullptr ||
            (element->style() &&
             element->style()->display() == DisplayValue::ContentsDisplayValue);

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
        if (!state.inert && element->hasAttribute(String::fromUTF8(
                                kInertAttribute, strlen(kInertAttribute)))) {
            state.inert = true;
        }
        if (state.blockingModal == element) {
            state.blockingModal = nullptr; // inside the dialog
        }

        // content-visibility: hidden never exposes its contents. A locked
        // auto subtree does, but it has no layout.
        state.displayLockRoot = false;
        bool lockedChildren = state.displayLocked;
        bool skipChildren = false;
        if (!state.displayLocked && rendered) {
            ContentVisibilityValue visibility = contentVisibilityOf(element);
            if (visibility ==
                ContentVisibilityValue::HiddenContentVisibilityValue) {
                state.displayLockRoot = true;
                skipChildren = true;
            } else if (visibility ==
                           ContentVisibilityValue::AutoContentVisibilityValue &&
                       isOutsideViewport(element)) {
                state.displayLockRoot = true;
                lockedChildren = true;
            }
        }

        AXNode* node = create(element, AXNodeKind::Element);
        node->m_role = axRoleForElement(element, tag);
        if (tabsScroller && tabsActive &&
            (node->m_role.empty() || node->m_role == "generic")) {
            // The minimum role of an active slide in the tabs mode is
            // tabpanel (CSS Overflow 5, ::scroll-marker-group).
            node->m_role = "tabpanel";
        }
        computeIgnored(node, element, tag, rendered, state);
        // accname step 2A: a hidden node has no name of its own. It still
        // contributes text when another element points at it, which is
        // computed from the referring element, not from here.
        if (!isHiddenReason(node)) {
            node->m_name = axNameForElement(element, tag, node->m_role);
        }
        attach(node, parentAXNode);

        // An inactive slide of a tabs-mode scroller is out of the tree with
        // its whole subtree (the tabpanel the user cannot see). The node
        // itself stays reachable through the parent chain.
        if (tabsScroller && !tabsActive) {
            return;
        }
        if (skipChildren) {
            return;
        }
        AXNode* childParent = node->m_includedInTree ? node : parentAXNode;
        // Chrome puts the options of a drop down under a popup node.
        if (element->isHTMLSelectElement() && !node->m_ignored &&
            isDropDown(element)) {
            childParent = createMenuListPopup(element, node);
        }
        // The text of an option is already its name.
        if (element->isHTMLOptionElement() && isInsideDropDown(element)) {
            return;
        }
        state.displayLockRoot = false;
        state.displayLocked = lockedChildren;
        addChildren(element, childParent, state);
        addScrollMarkerGroupIfNeeds(element, node, childParent, parentAXNode);
    }

    static bool isInsideDropDown(Element* option)
    {
        HTMLSelectElement* select =
            option->asHTMLOptionElement()->selectElement();
        return select && isDropDown(select);
    }

    // A select with no multiple attribute and a size of at most one.
    static bool isDropDown(Element* element)
    {
        if (axHasAttribute(element, "multiple")) {
            return false;
        }
        return element->asHTMLSelectElement()->size() <= 1;
    }

    AXNode* createMenuListPopup(Element* select, AXNode* selectNode)
    {
        std::string id = idFor(select, AXNodeKind::Element) + kPopupIdSuffix;
        m_tree->m_nodes.push_back(std::unique_ptr<AXNode>(
            new AXNode(nullptr, AXNodeKind::MenuListPopup, id)));
        AXNode* popup = m_tree->m_nodes.back().get();
        m_tree->m_byId[id] = popup;
        popup->m_role = kMenuListPopupRole;
        attach(popup, selectNode);
        return popup;
    }

    // CSS Overflow 5: a scroller whose scroll-marker-group is not none has
    // a generated ::scroll-marker-group, and with the tentative tabs switch
    // it gets the implicit tablist role while each slide's marker gets the
    // implicit tab role. The engine does not generate boxes for the
    // pseudo-elements yet, so this subtree is synthesized straight from the
    // scroller's element children. Chromium reports the group outside the
    // scroller's child list (only the active tabpanel is a child), so the
    // group attaches to the scroller's own parent.
    void addScrollMarkerGroupIfNeeds(Element* scroller, AXNode* scrollerNode,
                                     AXNode* childParent, AXNode* groupParent)
    {
        ComputedStyle* style = scroller->style();
        if (!style || style->scrollMarkerGroup() ==
                          ScrollMarkerGroupValue::NoneScrollMarkerGroupValue) {
            return;
        }
        // A scroller left out of the tree reports no marker group either;
        // its slides moved up to groupParent, so keep the group with them.
        if (!scrollerNode->m_includedInTree) {
            groupParent = childParent;
        }

        std::string baseId = idFor(scroller, AXNodeKind::Element);
        std::string groupId = baseId + kMarkerGroupIdSuffix;
        m_tree->m_nodes.push_back(std::unique_ptr<AXNode>(
            new AXNode(nullptr, AXNodeKind::ScrollMarkerGroup, groupId)));
        AXNode* group = m_tree->m_nodes.back().get();
        m_tree->m_byId[groupId] = group;
        group->m_role = "tablist";
        group->m_controlsTarget = scroller;
        attach(group, groupParent);

        int index = 0;
        Node* child = scroller->firstRenderingChild();
        RenderingSiblingIterator iterator(child);
        while (Optional<Node*> next = iterator.next()) {
            if (!next.value()) {
                break;
            }
            if (next.value()->isPseudoElement() || !next.value()->isElement()) {
                continue;
            }
            std::string tabId = baseId + "-marker-" + std::to_string(index);
            m_tree->m_nodes.push_back(std::unique_ptr<AXNode>(
                new AXNode(nullptr, AXNodeKind::ScrollMarker, tabId)));
            AXNode* tab = m_tree->m_nodes.back().get();
            m_tree->m_byId[tabId] = tab;
            tab->m_role = "tab";
            attach(tab, group);
            index++;
        }
    }

    void addTextNode(Node* domNode, AXNode* parentAXNode,
                     const InheritedState& state)
    {
        // A locked subtree has no layout, so its text is reported as written.
        std::string text = state.displayLocked ? rawTextForNode(domNode)
                                               : axTextForNode(domNode);
        // Collapsible whitespace between blocks produces no box and no
        // accessibility node (IsLayoutTextRelevantForAccessibility).
        if (text.empty() || (!state.displayLocked && !domNode->frame() &&
                             onlyWhitespace(text))) {
            return;
        }

        AXNode* node = create(domNode, AXNodeKind::StaticText);
        node->m_role = "StaticText";
        node->m_name = text;
        computeIgnored(node, nullptr, std::string(),
                       domNode->frame() != nullptr, state);
        attach(node, parentAXNode);
        if (!node->m_includedInTree || state.displayLocked) {
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

    std::string rawTextForNode(Node* node)
    {
        return toUTF8(node->textContent().hasValue()
                          ? node->textContent().value()
                          : nullptr);
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

    // The checkbox or radio a label makes redundant, or null
    // (AXNodeObject::IsRedundantLabel).
    // That happens when the control is visible, the label is what names it,
    // and the label holds nothing but the control.
    Element* redundantLabelControl(Element* element)
    {
        if (!element->isHTMLLabelElement()) {
            return nullptr;
        }
        HTMLElement* control = element->asHTMLLabelElement()->control();
        if (!control || !control->isHTMLInputElement() || !control->frame()) {
            return nullptr;
        }
        ComputedStyle* style = control->style();
        if (style &&
            style->visibility() != VisibilityValue::VisibleVisibilityValue) {
            return nullptr;
        }
        std::string type = axAttribute(control, "type");
        for (char& c : type) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (type != "checkbox" && type != "radio") {
            return nullptr;
        }
        // aria-label and aria-labelledby take precedence over the label.
        if (!axAttribute(control, "aria-label").empty() ||
            !axLabelledByAttribute(control).empty()) {
            return nullptr;
        }
        // An element child other than the control may carry meaning.
        Element* first = element->firstElementChild();
        if (!first) {
            return control;
        }
        if (first != control || first->nextElementSibling()) {
            return nullptr;
        }
        return control;
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
        if (state.inert) {
            node->m_ignored = true;
            node->m_ignoredReasons.push_back(AXIgnoredReason::InertElement);
            return;
        }
        if (state.blockingModal) {
            node->m_ignored = true;
            node->m_ignoredReasons.push_back(
                AXIgnoredReason::ActiveModalDialog);
            node->m_ignoredReasonTarget = state.blockingModal;
            return;
        }
        if (state.displayLocked) {
            return; // laid out never, but still part of the page
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
        if (axHasPresentationalRole(element) ||
            (tag == "li" && axIsPresentationalListItem(element))) {
            node->m_ignored = true;
            node->m_ignoredReasons.push_back(
                AXIgnoredReason::PresentationalRole);
            return;
        }

        // A label that only repeats the name of a checkbox or radio adds
        // nothing, so it is ignored in favor of the control.
        if (tag == "label") {
            if (Element* control = redundantLabelControl(element)) {
                node->m_ignored = true;
                node->m_ignoredReasons.push_back(AXIgnoredReason::LabelFor);
                node->m_ignoredReasonTarget = control;
            }
            return;
        }

        // A canvas of one pixel or less is too small to see. A canvas with
        // fallback content stays, as it holds that content.
        if (tag == "canvas" && !element->firstChild()) {
            Frame* frame = element->frame();
            if (frame && frame->isFrameBox()) {
                FrameBox* box = frame->asFrameBox();
                if (box->width().toFloat() <= 1 ||
                    box->height().toFloat() <= 1) {
                    node->m_ignored = true;
                    node->m_ignoredReasons.push_back(
                        AXIgnoredReason::ProbablyPresentational);
                    return;
                }
            }
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
                interesting = state.displayLockRoot ||
                              axHasAnyAriaAttribute(element) ||
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
        if (tag == "html" || tag == "body") {
            return true;
        }
        // A list item of a presentational list keeps its place.
        if (tag == "li") {
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
        // An element that is hidden or presentational on purpose does not.
        if (element->frame() && element->frame()->isFrameBlockBox() &&
            !isHiddenReason(node) &&
            !hasReason(node, AXIgnoredReason::InertElement) &&
            !hasReason(node, AXIgnoredReason::PresentationalRole)) {
            return true;
        }
        return false;
    }

    static bool hasReason(AXNode* node, AXIgnoredReason wanted)
    {
        for (AXIgnoredReason reason : node->m_ignoredReasons) {
            if (reason == wanted) {
                return true;
            }
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
