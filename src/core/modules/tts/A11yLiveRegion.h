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

#if defined(STARFISH_ENABLE_TTS) && \
    defined(STARFISH_ENABLE_A11Y_TOUCH_EXPLORATION)
#ifndef __StarfishA11yLiveRegion__
#define __StarfishA11yLiveRegion__

#include "binding/WebViewHoldable.h"

namespace Starfish {

class Document;
class Element;
class Node;

// ARIA live region announcements, spoken by the engine's own TTS backend.
//
// Chromium exposes live regions as object attributes + events and lets the
// screen reader do the announcing; the Tizen daemon does not implement that
// protocol, so this engine announces directly (product decision): DOM
// mutations inside an aria-live subtree (or role="alert"/"status") are
// spoken via TTS. "assertive" interrupts the current utterance (the Tizen
// speech(...) path stops before playing); "polite" waits for the channel
// to go quiet on a short drain timer.
//
// Supported: aria-live polite/assertive/off, role alert/status,
// aria-atomic (speak the whole region), aria-busy (suppress). Not
// supported: aria-relevant (additions/text only - removals are silent).
class A11yLiveRegion : public gc, public WebViewHoldable {
public:
    explicit A11yLiveRegion(WebView* webView);

    // Engine hooks. Cheap no-ops while accessibility TTS is inactive or
    // the document is still loading.
    static void nodeInserted(Document* document, Node* newChild);
    static void characterDataChanged(Node* node);

private:
    static void routeMutation(Document* document, Node* changed);
    void onMutation(Node* changed);
    // Nearest ancestor establishing a live region; nullptr when none, when
    // aria-live="off" intervenes, or when the subtree is aria-hidden.
    Element* findLiveRoot(Node* node, bool& assertive);
    void enqueuePolite(Element* root, String* text);
    void drain();

    // Polite queue: head-index ring so we never need erase() on GC vectors.
    GCVector<Element*> m_queueRoots;
    GCVector<String*> m_queueTexts;
    size_t m_queueHead{ 0 };
    size_t m_drainTimerId{ SIZE_MAX };
};

} // namespace Starfish

#endif
#endif
