/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "core/dom/Document.h"
#include "core/dom/HTMLIFrameElement.h"
#include "HistoryManager.h"
#include "core/page/WebView.h"
#include "platform/loader/ResourceURL.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Serializer.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/dom/WebOrigin.h"
#include "core/dom/DOMException.h"

namespace Starfish {

HistoryManager::HistoryManager(WebView* webView)
    : m_owner(HistoryManagerOwner::OwnerIsWebView)
    , m_webView(webView)
    , m_curEntry(m_historyEntries.end())
{
}

HistoryManager::HistoryManager(HTMLIFrameElement* element)
    : m_owner(HistoryManagerOwner::OwnerIsHTMLIFrame)
    , m_iframe(element)
    , m_curEntry(m_historyEntries.end())
{
}

HistoryManager* HistoryManager::create(WebView* webView)
{
    return new HistoryManager(webView);
}

HistoryManager* HistoryManager::create(HTMLIFrameElement* iframe)
{
    return new HistoryManager(iframe);
}

void HistoryManager::push(Document* document, ResourceURL* url)
{
    auto serializedState =
        Serializer::serialize(document->executionContext(), scriptNull());
    ScriptValue state = scriptNull();
    if (!m_historyEntries.empty()) {
        m_historyEntries.erase(std::next(m_curEntry, 1),
                               m_historyEntries.end());
    }
    addHistoryEntry(
        new HistoryEntry(serializedState, String::emptyString, url));
}

void HistoryManager::replace(Document* document, ResourceURL* url)
{
    HistoryEntry* entry = currentEntry();
    if (entry) {
        auto serializedState =
            Serializer::serialize(document->executionContext(), scriptNull());
        entry->init(serializedState, String::emptyString, url);
    }
}

bool HistoryManager::checkHistoryEntry(int delta, bool changeCurrentEntry)
{
    STARFISH_ASSERT(delta != 0);

    if (m_historyEntries.size() == 0) {
        return false;
    }

    int count = delta;
    auto itr = m_curEntry;
    if (delta > 0) {
        auto lastItem = --(m_historyEntries.end());
        while (itr != lastItem) {
            if (count == 0) {
                break;
            } else {
                count--;
                ++itr;
            }
        }
    } else {
        while (itr != m_historyEntries.begin()) {
            if (count == 0) {
                break;
            } else {
                count++;
                --itr;
            }
        }
    }

    if (count != 0) {
        // delta is out of range
        return false;
    }

    if (changeCurrentEntry) {
        m_curEntry = itr;
    }

    return true;
}

bool HistoryManager::go(int delta)
{
    if (checkHistoryEntry(delta, true)) {
        switch (m_owner) {
        case OwnerIsWebView:
            m_webView->messageLoop()->invokeNavigate(
                m_webView, currentEntry()->url(),
                new ReferrerURL(m_webView->mainBrowsingContext()
                                    ->document()
                                    ->documentURI()),
                HistoryManagerAction::Intact);
            break;
        case OwnerIsHTMLIFrame:
            m_iframe->navigate(
                currentEntry()->url(), HistoryManagerAction::Intact,
                new ReferrerURL(
                    m_iframe->browsingContext()->document()->documentURI()));
            break;
        }
        return true;
    }
    return false;
}

bool HistoryManager::canGo(int delta)
{
    return checkHistoryEntry(delta, false);
}

void HistoryManager::clear()
{
    m_historyEntries.clear();
    m_curEntry = m_historyEntries.end();
}

uint32_t HistoryManager::length()
{
    return m_historyEntries.size();
}

ScriptValue HistoryManager::state(Document* document)
{
    HistoryEntry* entry = currentEntry();
    if (entry) {
        return Serializer::deserialize(document->executionContext(),
                                       currentEntry()->state());
    } else {
        return scriptNull();
    }
}

void HistoryManager::pushState(Document* document, ScriptValue state,
                               String* title, Nullable<String*> url)
{
    pushReplaceStateInternal(document, state, title, url, OperationType::kPush);
}

void HistoryManager::replaceState(Document* document, ScriptValue state,
                                  String* title, Nullable<String*> url)
{
    pushReplaceStateInternal(document, state, title, url,
                             OperationType::kReplace);
}

// https://html.spec.whatwg.org/multipage/nav-history-apis.html#shared-history-push/replace-state-steps
void HistoryManager::pushReplaceStateInternal(Document* document,
                                              ScriptValue state, String* title,
                                              Nullable<String*> url,
                                              OperationType type)
{
    ResourceURL* newURL = resolveURL(document, url);

    if (!document->webOrigin()->canRewritten(
            WebOrigin::createDocumentOrigin(newURL))) {
        throw new DOMException(document->executionContext(),
                               DOMException::Code::SECURITY_ERR, "Invalid URL");
        return;
    }

    auto serializedState =
        Serializer::serialize(document->executionContext(), state);
    if (type == OperationType::kPush) {
        m_historyEntries.erase(std::next(m_curEntry, 1),
                               m_historyEntries.end());
        addHistoryEntry(new HistoryEntry(serializedState, title, newURL));
    } else {
        if (currentEntry()) {
            currentEntry()->init(serializedState, title, newURL);
        }
    }
}

ResourceURL* HistoryManager::resolveURL(Document* document,
                                        Nullable<String*> url)
{
    ResourceURL* resolvedURL = nullptr;
    if (url.hasValue()) {
        String* maybeRelativURL = url.value();
        if (maybeRelativURL->startsWith("#")) {
            // Change only hash.
            resolvedURL = document->documentURI()->setHash(maybeRelativURL);
        } else if (maybeRelativURL->startsWith("?")) {
            // Change only search params.
            resolvedURL = document->documentURI()->setSearch(maybeRelativURL);
        } else {
            resolvedURL = new ResourceURL(maybeRelativURL,
                                          document->baseURL()->baseURI());
        }
    } else {
        resolvedURL = new ResourceURL(*(currentEntry()->url()));
    }
    return resolvedURL;
}

HistoryManager::HistoryEntry* HistoryManager::currentEntry()
{
    if (m_curEntry == m_historyEntries.end()) {
        return nullptr;
    } else {
        return *m_curEntry;
    }
}

void HistoryManager::addHistoryEntry(HistoryEntry* entry)
{
    if (m_historyEntries.size() == MAX_ENTRY_SIZE) {
        m_historyEntries.pop_front();
    }
    m_historyEntries.push_back(entry);

    if (m_historyEntries.size() == 1) {
        m_curEntry = m_historyEntries.end();
        --m_curEntry;
    } else {
        ++m_curEntry;
    }
}

} // namespace Starfish
