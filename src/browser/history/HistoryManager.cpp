/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "core/dom/Document.h"
#include "core/dom/HTMLIFrameElement.h"
#include "HistoryManager.h"
#include "core/page/WebView.h"
#include "platform/loader/ResourceURL.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Serializer.h"

namespace StarFish {

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
    auto serializedState = Serializer::serialize(document, scriptNull());
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
        auto serializedState = Serializer::serialize(document, scriptNull());
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

    if (changeCurrentEntry)
        m_curEntry = itr;

    return true;
}

bool HistoryManager::go(int delta)
{
    if (checkHistoryEntry(delta, true)) {
        switch (m_owner) {
        case OwnerIsWebView:
            m_webView->navigate(
                currentEntry()->url(), Intact,
                m_webView->mainBrowsingContext()->document()->documentURI());
            break;
        case OwnerIsHTMLIFrame:
            m_iframe->navigate(
                currentEntry()->url(), Intact,
                m_iframe->browsingContext()->document()->documentURI());
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
        return Serializer::deserialize(document, currentEntry()->state());
    } else {
        return scriptNull();
    }
}

void HistoryManager::pushState(Document* document, ScriptValue state,
                               String* title, Nullable<String*> url)
{
    auto serializedState = Serializer::serialize(document, state);
    ResourceURL* newURL = nullptr;
    if (url.hasValue()) {
        newURL =
            new ResourceURL(url.getValue(), document->baseURL()->baseURI());
    } else {
        newURL = new ResourceURL(*(currentEntry()->url()));
    }

    m_historyEntries.erase(std::next(m_curEntry, 1), m_historyEntries.end());
    addHistoryEntry(new HistoryEntry(serializedState, title, newURL));
}

void HistoryManager::replaceState(Document* document, ScriptValue state,
                                  String* title, Nullable<String*> url)
{
    auto serializedState = Serializer::serialize(document, state);

    ResourceURL* newURL;
    if (url.hasValue()) {
        newURL =
            new ResourceURL(url.getValue(), document->baseURL()->baseURI());
    } else {
        newURL = new ResourceURL(*(currentEntry()->url()));
    }

    currentEntry()->init(serializedState, title, newURL);
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
}
