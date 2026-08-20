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

#ifdef STARFISH_ENABLE_A11Y_ATSPI
#ifndef __StarfishA11yAtspiTreeSource__
#define __StarfishA11yAtspiTreeSource__

#include "binding/WebViewHoldable.h"

namespace Starfish {

class Document;
class Element;
class Node;

// Engine-side data source for the AT-SPI2 (ATK) bridge in the EFL shell
// (src/public/bridge/efl/A11yAtspiBridge.cpp). The bridge runs on the same
// glib/ecore main loop as the engine (glib backend), so calls here are
// direct and synchronous.
//
// Chromium-style model:
//   - The accessibility tree is hierarchical: interactive/labelled targets
//     are leaves, and every DOM ancestor on the way to the (main) document
//     element is exposed as a container (Section / Document), mirroring how
//     Blink exposes its unignored AXObject hierarchy.
//   - Updates are event-driven: engine hooks (DOM mutation, attribute
//     change, control state change, scroll) call notifyPageChanged, which
//     marks the tree dirty and pings the registered change listener. The
//     bridge coalesces pings and re-diffs the tree - no polling.
//   - Activation is a semantic default action on the element (focus +
//     simulated click), not a synthesized coordinate tap.
//
// Accessibility nodes are handed out as opaque handles (Element*). The
// snapshot vector keeps them GC-alive between rebuilds, and every accessor
// revalidates its handle against the current snapshot by pointer comparison
// (no dereference of a stale handle), so a handle the daemon kept across a
// DOM change degrades to "invalid" instead of dangling.
class A11yAtspiTreeSource : public gc, public WebViewHoldable {
public:
    explicit A11yAtspiTreeSource(WebView* webView);

    // The most recently created instance (single-webview devices). Never
    // cleared: the webview outlives the bridge on the shell teardown path.
    static A11yAtspiTreeSource* current();

    // --- Tree structure -------------------------------------------------
    // Accessors rebuild the tree lazily after a change notification;
    // handles stay valid across rebuilds as long as the element is still
    // part of the tree.
    size_t rootChildCount();
    void* rootChildAt(size_t index);
    // nullptr means the node sits directly under the bridge's root (the
    // ATK plug window).
    void* parentOf(void* handle);
    size_t childCountOf(void* handle);
    void* childAt(void* handle, size_t index);
    // Position among siblings, or SIZE_MAX for a stale handle.
    size_t indexInParentOf(void* handle);
    bool isValid(void* handle);
    // True for the nodes the screen reader can land on (interactive
    // controls and labelled/text leaves). Containers only give the tree
    // its shape and must not be highlightable.
    bool isTarget(void* handle);

    // Viewport-relative CSS px in, handle out (nullptr on miss). Always
    // resolves to a target, never a container.
    void* hitTest(double clientX, double clientY);

    // Accessible name (computed text alternative), UTF-8.
    UTF8StringDataNonGCStd nameOf(void* handle);
    // Editable text content (input/textarea value) for Entry targets, UTF-8.
    // Empty for non-editable targets.
    UTF8StringDataNonGCStd textOf(void* handle);
    // Two-finger pan gesture: scroll the main document by CSS px.
    void scrollBy(double dx, double dy);
    // Border box in CSS px relative to the top-level viewport, CLIPPED by
    // every ancestor overflow box and each enclosing frame's viewport
    // (chromium reports clipped bounds the same way). A fully clipped
    // target yields an empty (zero-size) rect. False for a stale handle.
    bool rectOf(void* handle, double& x, double& y, double& width,
                double& height);

    // Coarse role for the daemon's announcements. Section/Document are
    // structural containers (never targets).
    enum class Role {
        Label,
        Button,
        Link,
        Entry,
        CheckBox,
        RadioButton,
        ComboBox,
        Image,
        Heading,
        List,
        ListItem,
        Dialog,
        ProgressBar,
        Slider,
        ToggleButton,
        Section,
        Document,
    };
    Role roleOf(void* handle);

    // Heading level (1-6): aria-level wins, else the h1-h6 tag digit, else 2
    // for a bare role="heading" (chromium's default). 0 for non-headings.
    int headingLevelOf(void* handle);
    // List item position within its list (1-based) and the list size, for
    // the daemon's "x of n" announcements. aria-posinset/aria-setsize win
    // over the DOM sibling ordinal. Both 0 when not a list item.
    void posInSetOf(void* handle, int& position, int& setSize);
    // Range widget value (slider / progress bar): aria-valuenow/min/max for
    // ARIA widgets, the control's own value/min/max for input[type=range]
    // and <progress>. False when the node has no value semantics.
    bool valueOf(void* handle, double& current, double& minimum,
                 double& maximum);

    // Live widget states: native control state plus the ARIA state
    // attributes (aria-checked/disabled/expanded/selected), the same subset
    // chromium's AXNodeObject maps for these roles. For native
    // checkboxes/radios the native checkedness wins over aria-checked.
    struct States {
        bool checkable{ false };
        bool checked{ false };
        bool mixed{ false }; // aria-checked="mixed"
        bool disabled{ false };
        bool expandable{ false };
        bool expanded{ false };
        bool selectable{ false };
        bool selected{ false };
        // Fully scrolled/clipped out of view (drops SHOWING, as chromium
        // marks offscreen objects).
        bool offscreen{ false };
        // aria-modal="true" (Dialog role): the daemon confines navigation
        // to the modal subtree, per the DA popup/scrim principle.
        bool modal{ false };
    };
    States statesOf(void* handle);

    // aria-labelledby / aria-describedby IDREF targets, filtered to
    // elements that are themselves exposed in the tree (a reference to an
    // unexposed node is dropped).
    std::vector<void*> relationTargetsOf(void* handle, bool describedBy);

    float devicePixelRatio() const;

    // Screen-reader highlight moved onto the target: keep it on screen.
    void highlight(void* handle);
    // Default action (double-tap), chromium-style: focus the element and
    // dispatch a simulated click on it (HTMLElement::AccessKeyAction
    // semantics), instead of replaying a coordinate tap.
    void activate(void* handle);

    // --- Change notification ---------------------------------------------
    // Class-level (not per-instance) so the registration survives the
    // re-creation of the tree source on cross-document navigation.
    typedef void (*ChangeListener)(void* userData);
    static void setChangeListener(ChangeListener listener, void* userData);
    // Engine hook: something in the given document's page changed (DOM
    // mutation, attribute, control state, scroll). Marks the current tree
    // dirty and pings the listener; coalescing is the listener's job.
    static void notifyPageChanged(Document* document);

private:
    Element* toElement(void* handle);
    // Rebuild m_snapshot/m_parent/m_isTarget from the DOM if dirty.
    void ensureTree();
    // Post-order collection; returns the node indices from this subtree
    // whose parent link is still pending (assigned when the nearest
    // included ancestor is pushed; roots keep SIZE_MAX).
    std::vector<size_t> collectFrom(Node* node);
    size_t indexOfInternal(void* handle) const;

    bool m_dirty{ true };
    // Node i: element, parent node index (SIZE_MAX for root level), and
    // whether it is a target (vs a structural container).
    GCVector<Element*> m_snapshot;
    GCAtomicVector<size_t> m_parent;
    GCAtomicVector<uint8_t> m_isTarget;
};

} // namespace Starfish

#endif
#endif
