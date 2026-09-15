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
#include "Starfish.h"
#include "core/dom/HTMLDetailsElement.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLDocument.h"
#include "core/dom/HTMLSlotElement.h"
#include "core/dom/ShadowRoot.h"
#include "core/dom/ToggleEvent.h"
#include "core/dom/Traverse.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "binding/ScriptEngineInstance.h"
#include "binding/ScriptBindingInstance.h"

namespace Starfish {
HTMLDetailsElement::HTMLDetailsElement(Document* document,
                                       const QualifiedName& qname)
    : HTMLElement(document, qname)
{
    // HTML rendering: separate summary and content slots keep author DOM
    // intact, including text preceding the first summary. The parts are
    // created as HTML elements explicitly: Document::createElement() yields
    // generic elements when an HTML details lives in an XML document.
    auto ss = starfish()->staticStrings();
    auto root = internalEnsureShadowRoot();
    auto summarySlot =
        HTMLDocument::createHTMLElement(document, ss->m_slotTagName);
    auto fallback =
        HTMLDocument::createHTMLElement(document, ss->m_summaryTagName);
    fallback->setTextContent(String::createASCIIString("Details"));
    fallback->setAttribute(
        ss->m_style,
        String::createASCIIString(
            "display: list-item; list-style-type: disclosure-closed; "
            "list-style-position: inside; counter-increment: list-item 0"));
    summarySlot->appendChild(fallback);
    root->appendChild(summarySlot);
    auto contentSlot =
        HTMLDocument::createHTMLElement(document, ss->m_slotTagName);
    contentSlot->setAttribute(ss->m_style,
                              String::createASCIIString("display: none"));
    root->appendChild(contentSlot);
}

void* HTMLDetailsElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLDetailsElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLDetailsElement)] = { 0 };
        HTMLElement::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLDetailsElement, m_pendingToggle));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLDetailsElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

bool HTMLDetailsElement::open()
{
    return hasAttribute(starfish()->staticStrings()->m_open.toString());
}

void HTMLDetailsElement::setOpen(bool value)
{
    if (value) {
        setAttribute(starfish()->staticStrings()->m_open, String::emptyString);
    } else {
        removeAttribute(starfish()->staticStrings()->m_open);
    }
}

Optional<Element*> HTMLDetailsElement::firstSummary()
{
    for (Node* child = firstChild(); child; child = child->nextSibling()) {
        if (child->isHTMLElement() &&
            child->asElement()->name() ==
                starfish()->staticStrings()->m_summaryTagName) {
            return child->asElement();
        }
    }
    return NullOption;
}

Optional<HTMLDetailsElement*> HTMLDetailsElement::summaryOwner(Node* summary)
{
    if (!summary->isHTMLElement() ||
        summary->asElement()->name() !=
            summary->starfish()->staticStrings()->m_summaryTagName) {
        return NullOption;
    }
    Node* parent = summary->parentNode();
    if (!parent) {
        return NullOption;
    }
    if (parent->isHTMLDetailsElement()) {
        auto details = parent->asHTMLDetailsElement();
        if (details->firstSummary().value() == summary) {
            return details;
        }
    } else if (parent->isHTMLSlotElement()) {
        Node* root = parent->parentNode();
        if (root && root->isShadowRoot() &&
            root->asShadowRoot()->host()->isHTMLDetailsElement()) {
            auto details = root->asShadowRoot()->host()->asHTMLDetailsElement();
            if (!details->firstSummary()) {
                return details;
            }
        }
    }
    return NullOption;
}

HTMLSlotElement* HTMLDetailsElement::slotFor(Node* child)
{
    // Only a summary-named child can be the first summary; skip the scan for
    // everything else.
    bool summaryNamed = child->isHTMLElement() &&
                        child->asElement()->name() ==
                            starfish()->staticStrings()->m_summaryTagName;
    return slotFor(child, summaryNamed ? firstSummary() : NullOption);
}

HTMLSlotElement* HTMLDetailsElement::slotFor(Node* child,
                                             Optional<Element*> firstSummary)
{
    // The summary slot precedes the content slot in the shadow root. Only
    // the first summary child is assigned to it; a later summary, text and
    // any other element are content.
    ShadowRoot* root = internalShadowRoot().value();
    bool isFirstSummary = firstSummary && firstSummary.value() == child;
    return (isFirstSummary ? root->firstChild() : root->lastChild())
        ->asHTMLSlotElement();
}

void HTMLDetailsElement::ensureExclusivity(bool closeOthers)
{
    if (!open()) {
        return;
    }
    auto name = getAttributeOrEmpty(starfish()->staticStrings()->m_name);
    if (name->isEmpty()) {
        return;
    }
    Traverse::traverse(getRootNode(), [&](Node* node) {
        if (node != this && node->isHTMLDetailsElement()) {
            auto other = node->asHTMLDetailsElement();
            if (other->open() &&
                other->getAttributeOrEmpty(starfish()->staticStrings()->m_name)
                    ->equals(name)) {
                (closeOthers ? other : this)->setOpen(false);
            }
        }
    });
}

bool HTMLDetailsElement::isDocumentDisposed()
{
    BrowsingContext* context = document()->browsingContext();
    return context && context->isDisposed();
}

// The idler payload of a queued toggle. Replacing a pending task does not
// remove its idler -- the window it lives on may already have freed it -- but
// cancels the payload, and the idler then fires once as a no-op. So the
// element never holds an idler handle that has to stay valid across document
// moves and window teardown.
struct HTMLDetailsElement::ToggleTask : public gc {
    HTMLDetailsElement* element;
    bool oldOpen;
    bool cancelled{ false };
};

void HTMLDetailsElement::queueToggle(bool oldOpen)
{
    // HTML "details notification task steps": a still-pending task is
    // replaced while its old state is kept. A destroyed document runs no
    // tasks (HTML "destroy a document"), so nothing is queued for it; a
    // document that merely has no browsing context (DOMParser,
    // createHTMLDocument) still gets its toggle, as the WPT toggleEvent tests
    // expect.
    if (m_pendingToggle) {
        m_pendingToggle->cancelled = true;
        oldOpen = m_pendingToggle->oldOpen;
        m_pendingToggle = nullptr;
    }
    if (isDocumentDisposed()) {
        return;
    }
    auto task = new ToggleTask;
    task->element = this;
    task->oldOpen = oldOpen;
    m_pendingToggle = task;
    window()->webView()->messageLoop()->addIdler(
        window(),
        [](size_t handle, void* data) {
            auto task = static_cast<ToggleTask*>(data);
            if (task->cancelled) {
                return;
            }
            auto details = task->element;
            details->m_pendingToggle = nullptr;
            if (details->isDocumentDisposed()) {
                return;
            }
            auto ss = details->starfish()->staticStrings();
            MicroTaskExecutionManager microtasks(
                details->scriptBindingInstance()->engineInstance());
            ToggleEventInit init;
            init.setOldState(
                (task->oldOpen ? ss->m_open : ss->m_closed).toString());
            init.setNewState(
                (details->open() ? ss->m_open : ss->m_closed).toString());
            details->dispatchEventByUA(new ToggleEvent(
                details->executionContext(), ss->m_toggle.toString(), init));
        },
        task);
}

void HTMLDetailsElement::didAttributeChanged(QualifiedName name,
                                             Optional<String*> old,
                                             String* value,
                                             bool attributeCreated,
                                             bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    auto ss = starfish()->staticStrings();
    if (name == ss->m_open && (attributeCreated || attributeRemoved)) {
        // Flip only the two properties that depend on open; the rest of the
        // internal declarations were set once in the constructor.
        const char* display = open() ? "block" : "none";
        internalShadowRoot()
            ->lastChild()
            ->asElement()
            ->inlineStyle()
            ->setPropertyInternal(CSSStyleValuePair::KeyKind::Display, display,
                                  strlen(display), false);
        const char* marker = open() ? "disclosure-open" : "disclosure-closed";
        internalShadowRoot()
            ->firstChild()
            ->firstChild()
            ->asElement()
            ->inlineStyle()
            ->setPropertyInternal(CSSStyleValuePair::KeyKind::ListStyleType,
                                  marker, strlen(marker), false);
        queueToggle(attributeRemoved);
        if (attributeCreated) {
            ensureExclusivity(true);
        }
    } else if (name == ss->m_name) {
        ensureExclusivity(false);
    }
}

} // namespace Starfish
