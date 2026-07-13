/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#ifdef STARFISH_ENABLE_A11Y_ATSPI

#include "core/page/A11yAtspiTreeSource.h"

#include "Starfish.h"
#include "core/dom/CharacterData.h"
#include "core/dom/Document.h"
#include "core/dom/DOMRect.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLElement.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/dom/HTMLInputElement.h"
#include "core/dom/HTMLLabelElement.h"
#include "core/dom/HTMLTextAreaElement.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBox.h"
#include "core/modules/tts/TextAlternativeHelper.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"

namespace Starfish {

static bool isAriaHiddenSelf(WebView* webView, Element* element)
{
    const QualifiedName& hidden =
        webView->starfish()->staticStrings()->m_ariaHidden;
    return element->getAttributeOrEmpty(hidden)->equalsIgnoreCase("true");
}

// Offset of an element's frame chain: sums, over every enclosing iframe, the
// iframe's border box position in ITS parent frame's viewport plus the
// border+padding to the inner content origin (same content-origin math as
// BrowsingContext::isInnerIFrameEvent). Yields top-level viewport CSS px.
static void frameChainOffset(Element* element, double& offsetX, double& offsetY)
{
    offsetX = 0;
    offsetY = 0;
    Document* doc = element->ownerDocument();
    BrowsingContext* bc = doc ? doc->browsingContext() : nullptr;
    while (bc && bc->parentBrowsingContext()) {
        HTMLIFrameElement* source = bc->sourceElement();
        if (!source) {
            break;
        }
        DOMRect* rect = source->getBoundingClientRect();
        double contentX = 0, contentY = 0;
        if (source->frame() && source->frame()->isFrameBox()) {
            FrameBox* fb = source->frame()->asFrameBox();
            contentX = (double)(fb->paddingLeft() + fb->borderLeft());
            contentY = (double)(fb->paddingTop() + fb->borderTop());
        }
        offsetX += rect->x() + contentX;
        offsetY += rect->y() + contentY;
        bc = bc->parentBrowsingContext();
    }
}

static A11yAtspiTreeSource* g_currentTreeSource;
static A11yAtspiTreeSource::ChangeListener g_changeListener;
static void* g_changeListenerData;

A11yAtspiTreeSource::A11yAtspiTreeSource(WebView* webView)
    : WebViewHoldable(webView)
{
    g_currentTreeSource = this;
}

A11yAtspiTreeSource* A11yAtspiTreeSource::current()
{
    return g_currentTreeSource;
}

void A11yAtspiTreeSource::setChangeListener(ChangeListener listener,
                                            void* userData)
{
    g_changeListener = listener;
    g_changeListenerData = userData;
}

void A11yAtspiTreeSource::notifyPageChanged(Document* document)
{
    A11yAtspiTreeSource* source = g_currentTreeSource;
    if (!source || !document) {
        return;
    }
    Window* window = document->window();
    if (!window || window->webView() != source->webView()) {
        return;
    }
    source->m_dirty = true;
    if (g_changeListener) {
        g_changeListener(g_changeListenerData);
    }
}

// Interactive controls are unconditional targets (chromium-efl
// ax_platform_node_efl.cc IsAccessible: focusable objects are always
// accepted).
static bool isInteractiveTarget(WebView* webView, Element* element)
{
    if (!element->hasFocusableStyle()) {
        return false;
    }
    StaticStrings* ss = webView->starfish()->staticStrings();
    String* role = element->getAttributeOrEmpty(ss->m_role);
    if (role->equalsIgnoreCase("presentation") ||
        role->equalsIgnoreCase("none")) {
        return false;
    }
    if (element->isHTMLButtonElement() || element->isHTMLAnchorElement() ||
        element->isHTMLInputElement() || element->isHTMLSelectElement() ||
        element->isHTMLTextAreaElement()) {
        return true;
    }
    if (element->tabIndexSetExplicitly() && element->tabIndex() >= 0) {
        return true;
    }
    return role->equals("button") || role->equalsIgnoreCase("link");
}

// Explicit accessible name on the element itself (alt counts for images).
static bool hasOwnLabel(WebView* webView, Element* element)
{
    StaticStrings* ss = webView->starfish()->staticStrings();
    if (!element->getAttributeOrEmpty(ss->m_ariaLabel)->isEmpty() ||
        !element->getAttributeOrEmpty(ss->m_ariaLabelledby)->isEmpty()) {
        return true;
    }
    if (element->isHTMLImageElement()) {
        return !element->getAttributeOrEmpty(ss->m_alt)->isEmpty();
    }
    return false;
}

// Direct non-whitespace text child - the counterpart of chromium's
// HasText() for leaf-ish content, so plain text runs stay reachable by
// swipe even without ARIA markup.
static bool hasDirectText(Element* element)
{
    for (Node* child = element->firstChild(); child;
         child = child->nextSibling()) {
        if (child->nodeType() == Node::TEXT_NODE) {
            String* data = static_cast<CharacterData*>(child)->data();
            if (data && !data->containsOnlyWhitespace()) {
                return true;
            }
        }
    }
    return false;
}

// Post-order DOM collection that also descends into iframe content documents
// at the iframe's position (reading order), skipping aria-hidden subtrees.
//
// Targets follow chromium-efl's IsAccessible: interactive controls are
// always targets; a labelled or text-bearing element becomes a target only
// when NO descendant produced one (leaf grouping), so a big aria-labelled
// wrapper does not shadow - or duplicate - the individually reachable
// content inside it.
//
// Chromium-style hierarchy: any element with included descendants is kept
// as a container node (its children attach to it), and a document element
// is always kept when its document contributed anything - so the exposed
// tree mirrors the DOM ancestor chain instead of flattening everything
// under the window.
std::vector<size_t> A11yAtspiTreeSource::collectFrom(Node* node)
{
    Element* element = nullptr;
    if (node->isElement()) {
        element = node->asElement();
        if (isAriaHiddenSelf(webView(), element)) {
            return std::vector<size_t>();
        }
        if (element->isHTMLIFrameElement()) {
            // Expose only the iframe's content, not the (nameless) iframe
            // shell itself; its DOM children are fallback content.
            Document* doc = element->asHTMLIFrameElement()->contentDocument();
            return doc ? collectFrom(doc) : std::vector<size_t>();
        }
    }

    std::vector<size_t> pending;
    for (Node* child = node->firstChild(); child;
         child = child->nextSibling()) {
        std::vector<size_t> fromChild = collectFrom(child);
        pending.insert(pending.end(), fromChild.begin(), fromChild.end());
    }

    if (!element) {
        return pending;
    }

    bool interactive = isInteractiveTarget(webView(), element);
    bool leafLabelled =
        !interactive && pending.empty() && element->hasFocusableStyle() &&
        (hasOwnLabel(webView(), element) || hasDirectText(element));
    Document* ownerDocument = element->ownerDocument();
    bool isDocumentElement =
        ownerDocument && ownerDocument->documentElement() == element;
    if (!interactive && !leafLabelled && pending.empty() &&
        !isDocumentElement) {
        return pending;
    }
    if (isDocumentElement && pending.empty() && !interactive && !leafLabelled) {
        // Keep the main document element even when the page has no targets
        // (there is always a Document node, as in chromium); drop empty
        // iframe document elements.
        BrowsingContext* bc = ownerDocument->browsingContext();
        if (!bc || bc->parentBrowsingContext()) {
            return pending;
        }
    }

    size_t index = m_snapshot.size();
    m_snapshot.push_back(element);
    m_parent.push_back(SIZE_MAX);
    m_isTarget.push_back((interactive || leafLabelled) ? 1 : 0);
    for (size_t childIndex : pending) {
        m_parent[childIndex] = index;
    }
    return std::vector<size_t>(1, index);
}

void A11yAtspiTreeSource::ensureTree()
{
    if (!m_dirty) {
        return;
    }
    m_dirty = false;
    m_snapshot.clear();
    m_parent.clear();
    m_isTarget.clear();
    BrowsingContext* bc = webView()->mainBrowsingContext();
    Document* doc = bc ? bc->document() : nullptr;
    if (doc) {
        collectFrom(doc);
    }
}

size_t A11yAtspiTreeSource::indexOfInternal(void* handle) const
{
    for (size_t i = 0; i < m_snapshot.size(); i++) {
        if (m_snapshot[i] == handle) {
            return i;
        }
    }
    return SIZE_MAX;
}

size_t A11yAtspiTreeSource::rootChildCount()
{
    return childCountOf(nullptr);
}

void* A11yAtspiTreeSource::rootChildAt(size_t index)
{
    return childAt(nullptr, index);
}

void* A11yAtspiTreeSource::parentOf(void* handle)
{
    ensureTree();
    size_t index = indexOfInternal(handle);
    if (index == SIZE_MAX || m_parent[index] == SIZE_MAX) {
        return nullptr;
    }
    return m_snapshot[m_parent[index]];
}

// Children of `handle` (nullptr = root level). Sibling order is DOM order:
// collectFrom pushes siblings' subtree roots in DOM order, so scanning by
// ascending node index yields it back.
size_t A11yAtspiTreeSource::childCountOf(void* handle)
{
    ensureTree();
    size_t parentIndex = SIZE_MAX;
    if (handle) {
        parentIndex = indexOfInternal(handle);
        if (parentIndex == SIZE_MAX) {
            return 0;
        }
    }
    size_t count = 0;
    for (size_t i = 0; i < m_parent.size(); i++) {
        if (m_parent[i] == parentIndex) {
            count++;
        }
    }
    return count;
}

void* A11yAtspiTreeSource::childAt(void* handle, size_t index)
{
    ensureTree();
    size_t parentIndex = SIZE_MAX;
    if (handle) {
        parentIndex = indexOfInternal(handle);
        if (parentIndex == SIZE_MAX) {
            return nullptr;
        }
    }
    size_t seen = 0;
    for (size_t i = 0; i < m_parent.size(); i++) {
        if (m_parent[i] == parentIndex) {
            if (seen == index) {
                return m_snapshot[i];
            }
            seen++;
        }
    }
    return nullptr;
}

size_t A11yAtspiTreeSource::indexInParentOf(void* handle)
{
    ensureTree();
    size_t index = indexOfInternal(handle);
    if (index == SIZE_MAX) {
        return SIZE_MAX;
    }
    size_t parentIndex = m_parent[index];
    size_t position = 0;
    for (size_t i = 0; i < index; i++) {
        if (m_parent[i] == parentIndex) {
            position++;
        }
    }
    return position;
}

bool A11yAtspiTreeSource::isValid(void* handle)
{
    ensureTree();
    return indexOfInternal(handle) != SIZE_MAX;
}

bool A11yAtspiTreeSource::isTarget(void* handle)
{
    ensureTree();
    size_t index = indexOfInternal(handle);
    return index != SIZE_MAX && m_isTarget[index];
}

Element* A11yAtspiTreeSource::toElement(void* handle)
{
    if (!isValid(handle)) {
        return nullptr;
    }
    return static_cast<Element*>(handle);
}

void* A11yAtspiTreeSource::hitTest(double clientX, double clientY)
{
    BrowsingContext* bc = webView()->mainBrowsingContext();
    if (!bc) {
        return nullptr;
    }
    // Descend through iframes the same way event dispatch does: hitTest()
    // works in page coordinates per browsing context, isInnerIFrameEvent
    // converts to the inner frame's viewport coordinates.
    double pageX = clientX + bc->window()->scrollX(false);
    double pageY = clientY + bc->window()->scrollY(false);
    Node* node = nullptr;
    while (true) {
        node = bc->hitTest(pageX, pageY);
        if (!node || !node->isHTMLIFrameElement()) {
            break;
        }
        double innerX = pageX, innerY = pageY;
        if (!bc->isInnerIFrameEvent(node, innerX, innerY)) {
            break;
        }
        BrowsingContext* child = node->asHTMLIFrameElement()->browsingContext();
        if (!child) {
            break;
        }
        bc = child;
        pageX = innerX + bc->window()->scrollX(false);
        pageY = innerY + bc->window()->scrollY(false);
    }
    if (!node) {
        return nullptr;
    }
    ensureTree();
    Element* element =
        node->isElement() ? node->asElement() : node->parentElement();
    // Promote to the nearest enumerated TARGET (containers only shape the
    // tree), crossing frame boundaries upward through the owning iframe
    // element. A label promotes to its associated control (touching "Agree"
    // next to a checkbox must land on the checkbox).
    while (element) {
        size_t index = indexOfInternal(element);
        if (index != SIZE_MAX && m_isTarget[index]) {
            break;
        }
        if (element->isHTMLLabelElement()) {
            HTMLElement* control = element->asHTMLLabelElement()->control();
            size_t controlIndex = control ? indexOfInternal(control) : SIZE_MAX;
            if (controlIndex != SIZE_MAX && m_isTarget[controlIndex]) {
                element = control;
                break;
            }
        }
        Element* parent = element->parentElement();
        if (!parent) {
            Document* doc = element->ownerDocument();
            BrowsingContext* ebc = doc ? doc->browsingContext() : nullptr;
            parent = ebc ? ebc->sourceElement() : nullptr;
        }
        element = parent;
    }
    return element;
}

UTF8StringDataNonGCStd A11yAtspiTreeSource::nameOf(void* handle)
{
    Element* element = toElement(handle);
    if (!element) {
        return UTF8StringDataNonGCStd();
    }
    // Containers exist only for tree shape; without an explicit label their
    // computed text alternative would concatenate the whole subtree (accname
    // rule 2C), so keep them nameless like chromium's unlabelled generic
    // containers.
    size_t index = indexOfInternal(handle);
    if (index != SIZE_MAX && !m_isTarget[index] &&
        !hasOwnLabel(webView(), element)) {
        return UTF8StringDataNonGCStd();
    }
    TextAlternativeHelper tah(webView());
    String* text = tah.getComputedTextAlternative(element);
    return text->toUTF8NonGCString();
}

bool A11yAtspiTreeSource::rectOf(void* handle, double& x, double& y,
                                 double& width, double& height)
{
    Element* element = toElement(handle);
    if (!element) {
        return false;
    }
    DOMRect* rect = element->getBoundingClientRect();
    double offsetX = 0, offsetY = 0;
    frameChainOffset(element, offsetX, offsetY);
    x = rect->x() + offsetX;
    y = rect->y() + offsetY;
    width = rect->width();
    height = rect->height();
    return true;
}

A11yAtspiTreeSource::Role A11yAtspiTreeSource::roleOf(void* handle)
{
    ensureTree();
    size_t index = indexOfInternal(handle);
    if (index == SIZE_MAX) {
        return Role::Label;
    }
    Element* element = static_cast<Element*>(handle);
    Document* ownerDocument = element->ownerDocument();
    if (ownerDocument && ownerDocument->documentElement() == element) {
        return Role::Document;
    }
    if (!m_isTarget[index]) {
        return Role::Section;
    }
    StaticStrings* ss = webView()->starfish()->staticStrings();
    String* role = element->getAttributeOrEmpty(ss->m_role);
    if (role->equals("button")) {
        return Role::Button;
    }
    if (role->equalsIgnoreCase("link")) {
        return Role::Link;
    }
    if (element->isHTMLButtonElement()) {
        return Role::Button;
    }
    if (element->isHTMLAnchorElement()) {
        return Role::Link;
    }
    if (element->isHTMLInputElement()) {
        String* type = element->getAttributeOrEmpty(ss->m_type)->toASCIILower();
        if (type->equals("checkbox")) {
            return Role::CheckBox;
        }
        if (type->equals("radio")) {
            return Role::RadioButton;
        }
        if (type->equals("button") || type->equals("submit") ||
            type->equals("reset") || type->equals("image")) {
            return Role::Button;
        }
        return Role::Entry;
    }
    if (element->isHTMLTextAreaElement()) {
        return Role::Entry;
    }
    if (element->isHTMLSelectElement()) {
        return Role::ComboBox;
    }
    if (element->isHTMLImageElement()) {
        return Role::Image;
    }
    String* local = element->localName();
    if (local->equals("h1") || local->equals("h2") || local->equals("h3") ||
        local->equals("h4") || local->equals("h5") || local->equals("h6")) {
        return Role::Heading;
    }
    return Role::Label;
}

UTF8StringDataNonGCStd A11yAtspiTreeSource::textOf(void* handle)
{
    Element* element = toElement(handle);
    if (!element) {
        return UTF8StringDataNonGCStd();
    }
    String* value = nullptr;
    if (element->isHTMLInputElement()) {
        value = element->asHTMLInputElement()->value();
    } else if (element->isHTMLTextAreaElement()) {
        value = element->asHTMLTextAreaElement()->value();
    }
    if (!value) {
        return UTF8StringDataNonGCStd();
    }
    return value->toUTF8NonGCString();
}

void A11yAtspiTreeSource::scrollBy(double dx, double dy)
{
    BrowsingContext* bc = webView()->mainBrowsingContext();
    if (!bc) {
        return;
    }
    bc->window()->scrollBy(dx, dy);
}

bool A11yAtspiTreeSource::isChecked(void* handle)
{
    Element* element = toElement(handle);
    if (!element || !element->isHTMLInputElement()) {
        return false;
    }
    return element->asHTMLInputElement()->checked();
}

float A11yAtspiTreeSource::devicePixelRatio() const
{
    return webView()->screenInfo().devicePixelRatio;
}

void A11yAtspiTreeSource::highlight(void* handle)
{
    Element* element = toElement(handle);
    if (!element) {
        return;
    }
    // Only keep the target visible. No DOM focus and no engine-side TTS:
    // the screen-reader daemon announces the target itself from GetName.
    element->scrollIntoViewIfNeeded();
}

void A11yAtspiTreeSource::activate(void* handle)
{
    Element* element = toElement(handle);
    if (!element || !element->isHTMLElement()) {
        return;
    }
    element->scrollIntoViewIfNeeded();
    // Chromium's default AX action (HTMLElement::AccessKeyAction): focus
    // the element, then dispatch a simulated click on it. Acting on the
    // element directly is immune to overlays and post-scroll layout shifts
    // that a synthesized coordinate tap could hit instead.
    if (element->isFocusable()) {
        element->focus();
    }
    static_cast<HTMLElement*>(element)->click();
}

} // namespace Starfish

#endif
