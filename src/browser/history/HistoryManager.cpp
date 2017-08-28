/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"

#include "HistoryManager.h"

#include "core/page/WebView.h"
#include "platform/loader/ResourceURL.h"
#include "core/page/BrowsingContext.h"

namespace StarFish {

HistoryManager::HistoryManager(WebView* webView)
    : m_webView(webView)
    , m_curEntry(m_historyEntries.end())
{
}

HistoryManager* HistoryManager::create(WebView* webView)
{
    return new HistoryManager(webView);
}

void HistoryManager::push(ResourceURL* url)
{
    ScriptValue state = scriptNull();
    if (!m_historyEntries.empty()) {
        m_historyEntries.erase(std::next(m_curEntry, 1),
                               m_historyEntries.end());
    }
    addHistoryEntry(new HistoryEntry(state, String::emptyString, url));
}

void HistoryManager::replace(ResourceURL* url)
{
    HistoryEntry* entry = currentEntry();
    if (entry) {
        entry->init(scriptNull(), String::emptyString, url);
    }
}

void HistoryManager::go(int delta)
{
    STARFISH_ASSERT(delta != 0);

    if (m_historyEntries.size() == 0) {
        return;
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
        return;
    }

    m_curEntry = itr;
    m_webView->navigate(currentEntry()->url(), Intact);
}

uint32_t HistoryManager::length()
{
    return m_historyEntries.size();
}

ScriptValue HistoryManager::state()
{
    HistoryEntry* entry = currentEntry();
    if (entry) {
        return currentEntry()->state();
    } else {
        return scriptNull();
    }
}

void HistoryManager::pushState(ScriptValue state, String* title,
                               Nullable<String*> url)
{
    ResourceURL* newURL = nullptr;
    if (url.hasValue()) {
        if (url.getValue()->startsWith("/")) {
            String* str = ResourceURL::mergeDocumentURIWithURIString(
                m_webView->mainBrowsingContext()->document(), url.getValue());
            newURL = new ResourceURL(str);
        } else {
            newURL = new ResourceURL(url.getValue());
        }

    } else {
        newURL = new ResourceURL(*(currentEntry()->url()));
    }

    m_historyEntries.erase(std::next(m_curEntry, 1), m_historyEntries.end());
    addHistoryEntry(new HistoryEntry(state, title, newURL));
}

void HistoryManager::replaceState(ScriptValue state, String* title,
                                  Nullable<String*> url)
{
    ResourceURL* newURL;
    if (url.hasValue()) {
        newURL = new ResourceURL(url.getValue());
    } else {
        newURL = new ResourceURL(*(currentEntry()->url()));
    }

    currentEntry()->init(state, title, newURL);
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
