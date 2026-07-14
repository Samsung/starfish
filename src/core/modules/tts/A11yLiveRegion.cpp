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

#if defined(STARFISH_ENABLE_TTS) && \
    defined(STARFISH_ENABLE_A11Y_TOUCH_EXPLORATION)

#include "core/modules/tts/A11yLiveRegion.h"

#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/tts/TTS.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"

namespace Starfish {

// How often the polite queue re-checks whether the TTS channel went quiet.
static const unsigned kDrainIntervalMs = 400;

// Defined here (not in the per-platform TTS backends) so the lazy-create
// logic exists exactly once, under the same build gate as the class.
A11yLiveRegion* TTS::liveRegion()
{
    if (!m_liveRegion) {
        m_liveRegion = new A11yLiveRegion(webView());
    }
    return m_liveRegion;
}

A11yLiveRegion::A11yLiveRegion(WebView* webView)
    : WebViewHoldable(webView)
{
}

void A11yLiveRegion::nodeInserted(Document* document, Node* newChild)
{
    routeMutation(document, newChild);
}

void A11yLiveRegion::characterDataChanged(Node* node)
{
    routeMutation(node->document(), node);
}

void A11yLiveRegion::routeMutation(Document* document, Node* changed)
{
    if (!document) {
        return;
    }
    Window* window = document->window();
    if (!window) {
        return;
    }
    WebView* webView = window->webView();
    if (!webView) {
        return;
    }
    TTS* tts = webView->tts();
    if (!tts ||
        (!tts->isAccessibilityMode() && tts->mode() != LWE::TTSMode::Forced)) {
        return;
    }
    tts->liveRegion()->onMutation(changed);
}

// aria-live="polite|assertive" wins over the role defaults; "off" (or an
// aria-hidden ancestor) suppresses the whole subtree. role="alert" is an
// implicit assertive region, role="status" an implicit polite one.
Element* A11yLiveRegion::findLiveRoot(Node* node, bool& assertive)
{
    StaticStrings* ss = webView()->starfish()->staticStrings();
    for (Node* n = node; n; n = n->parentNode()) {
        if (!n->isElement()) {
            continue;
        }
        Element* element = n->asElement();
        if (element->getAttributeOrEmpty(ss->m_ariaHidden)
                ->equalsIgnoreCase("true")) {
            return nullptr;
        }
        String* live = element->getAttributeOrEmpty(ss->m_ariaLive);
        if (live->equalsIgnoreCase("off")) {
            return nullptr;
        }
        if (live->equalsIgnoreCase("assertive")) {
            assertive = true;
            return element;
        }
        if (live->equalsIgnoreCase("polite")) {
            assertive = false;
            return element;
        }
        String* role = element->getAttributeOrEmpty(ss->m_role);
        if (role->equalsIgnoreCase("alert")) {
            assertive = true;
            return element;
        }
        if (role->equalsIgnoreCase("status")) {
            assertive = false;
            return element;
        }
    }
    return nullptr;
}

void A11yLiveRegion::onMutation(Node* changed)
{
    bool assertive = false;
    Element* root = findLiveRoot(changed, assertive);
    if (!root) {
        return;
    }
    // The parser fills live regions like any other markup; announce only
    // what changes after the document finished loading (as chromium
    // suppresses live events until load completes).
    if (!changed->document() ||
        !changed->document()->readyState()->equals("complete")) {
        return;
    }
    StaticStrings* ss = webView()->starfish()->staticStrings();
    if (root->getAttributeOrEmpty(ss->m_ariaBusy)->equalsIgnoreCase("true")) {
        return;
    }
    Node* source =
        root->getAttributeOrEmpty(ss->m_ariaAtomic)->equalsIgnoreCase("true")
        ? root
        : changed;
    auto textContent = source->textContent();
    if (!textContent.hasValue()) {
        return;
    }
    String* text = textContent.getValue();
    if (!text || text->containsOnlyWhitespace()) {
        return;
    }
    STARFISH_LOG_INFO("A11yLiveRegion: %s \"%s\"\n",
                      assertive ? "assertive" : "polite",
                      text->toUTF8NonGCString().data());
    if (assertive) {
        // speech() stops the current utterance before playing, which is
        // exactly the assertive contract; pending polite items are stale
        // context by now.
        m_queueRoots.clear();
        m_queueTexts.clear();
        m_queueHead = 0;
        webView()->tts()->speech(root, text);
        return;
    }
    enqueuePolite(root, text);
}

void A11yLiveRegion::enqueuePolite(Element* root, String* text)
{
    if (m_queueHead >= m_queueRoots.size() &&
        !webView()->tts()->isSpeaking()) {
        webView()->tts()->speech(root, text);
        return;
    }
    m_queueRoots.push_back(root);
    m_queueTexts.push_back(text);
    if (m_drainTimerId == SIZE_MAX) {
        m_drainTimerId = webView()->timer()->addTimer(
            kDrainIntervalMs, nullptr,
            [](void* data) {
                static_cast<A11yLiveRegion*>(data)->drain();
            },
            this, true);
    }
}

void A11yLiveRegion::drain()
{
    // Cross-document navigation recreates TTS (and its live region); a
    // stale instance kept alive by this repeating timer must not speak
    // the previous page's leftovers.
    TTS* tts = webView()->tts();
    if (!tts || tts->liveRegion() != this) {
        m_queueHead = m_queueRoots.size();
    }
    if (m_queueHead >= m_queueRoots.size()) {
        m_queueRoots.clear();
        m_queueTexts.clear();
        m_queueHead = 0;
        if (m_drainTimerId != SIZE_MAX) {
            webView()->timer()->removeTimer(m_drainTimerId);
            m_drainTimerId = SIZE_MAX;
        }
        return;
    }
    if (webView()->tts()->isSpeaking()) {
        return;
    }
    Element* root = m_queueRoots[m_queueHead];
    String* text = m_queueTexts[m_queueHead];
    m_queueHead++;
    webView()->tts()->speech(root, text);
}

} // namespace Starfish

#endif
