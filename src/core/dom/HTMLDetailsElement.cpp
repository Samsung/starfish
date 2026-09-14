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
#include "core/dom/HTMLSlotElement.h"
#include "core/dom/ShadowRoot.h"
#include "core/dom/ToggleEvent.h"
#include "core/dom/Traverse.h"
#include "core/modules/message_loop/MessageLoop.h"
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
    // intact, including text preceding the first summary.
    auto root = internalEnsureShadowRoot();
    auto summarySlot =
        document->createElement(String::createASCIIString("slot"));
    auto fallback =
        document->createElement(String::createASCIIString("summary"));
    fallback->setTextContent(String::createASCIIString("Details"));
    fallback->setAttribute(
        starfish()->staticStrings()->m_style,
        String::createASCIIString(
            "display: list-item; list-style-type: disclosure-closed; "
            "list-style-position: inside; counter-increment: list-item 0"));
    summarySlot->appendChild(fallback);
    root->appendChild(summarySlot);
    auto contentSlot =
        document->createElement(String::createASCIIString("slot"));
    contentSlot->setAttribute(starfish()->staticStrings()->m_style,
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

void HTMLDetailsElement::ensureExclusivity(bool closeOthers)
{
    auto name = getAttributeOrEmpty(starfish()->staticStrings()->m_name);
    if (!open() || !name->length()) {
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

void HTMLDetailsElement::queueToggle(bool oldOpen)
{
    auto loop = window()->webView()->messageLoop();
    if (m_toggleTask != SIZE_MAX) {
        loop->removeIdler(m_toggleTask);
    } else {
        m_toggleOldOpen = oldOpen;
    }
    m_toggleTask = loop->addIdler(
        window(),
        [](size_t handle, void* data) {
            auto details = static_cast<HTMLDetailsElement*>(data);
            MicroTaskExecutionManager microtasks(
                details->scriptBindingInstance()->engineInstance());
            ToggleEventInit init;
            init.setOldState(details->m_toggleOldOpen
                                 ? String::createASCIIString("open")
                                 : String::createASCIIString("closed"));
            init.setNewState(details->open()
                                 ? String::createASCIIString("open")
                                 : String::createASCIIString("closed"));
            details->m_toggleTask = SIZE_MAX;
            details->dispatchEventByUA(
                new ToggleEvent(details->executionContext(),
                                String::createASCIIString("toggle"), init));
        },
        this);
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
        internalShadowRoot()->lastChild()->asElement()->setAttribute(
            ss->m_style, open() ? String::createASCIIString("display: block")
                                : String::createASCIIString("display: none"));
        internalShadowRoot()
            ->firstChild()
            ->firstChild()
            ->asElement()
            ->setAttribute(
                ss->m_style,
                open() ? String::createASCIIString(
                             "display: list-item; list-style-type: "
                             "disclosure-open; list-style-position: inside; "
                             "counter-increment: list-item 0")
                       : String::createASCIIString(
                             "display: list-item; list-style-type: "
                             "disclosure-closed; list-style-position: inside; "
                             "counter-increment: list-item 0"));
        queueToggle(attributeRemoved);
        if (attributeCreated) {
            ensureExclusivity(true);
        }
    } else if (name == ss->m_name) {
        ensureExclusivity(false);
    }
}

} // namespace Starfish
