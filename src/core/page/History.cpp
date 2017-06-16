/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/page/History.h"
#include "core/page/HistoryEntry.h"
#include "core/page/Location.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "platform/window/PlatformWindow.h"
#include "WebView.h"

namespace StarFish {

History::History(Document* doc)
    : ScriptWrappable(this)
    , DocumentHoldable(doc)
    , m_offset(std::numeric_limits<uint32_t>::max())
{
}

void History::back()
{
    go(-1);
}

void History::forward()
{
    go(1);
}

void History::go(int delta)
{
    if (delta != 0) {
        // navigate according to history
        navigate(delta);
    } else {
        starFish()
            ->platformWindow()
            ->webView()
            ->mainBrowsingContext()
            ->navigateAsync(currentHistoryEntry()->url());
        // starFish()->window()->navigateAsyncWithoutSetHistory(
        //    currentHistoryEntry()->url());
    }
}

ResourceURL* History::getURL()
{
    return m_historyEntries[m_offset]->url();
}

bool History::navigate(int delta)
{
    STARFISH_ASSERT(delta != 0);

    if (delta > 0) {
        STARFISH_ASSERT(length() - 1 >= m_offset);
        if (static_cast<uint32_t>(delta) > length() - m_offset - 1) {
            return false;
        }
    } else {
        // delta < 0
        if (m_offset < static_cast<uint32_t>(-delta)) {
            return false;
        }
    }

    m_offset += delta;

    ResourceURL* url = currentHistoryEntry()->url();
    if (!isPushState()) {
        // starFish()->window()->navigateAsyncWithoutSetHistory(url);
        starFish()
            ->platformWindow()
            ->webView()
            ->mainBrowsingContext()
            ->navigateAsync(url);
    } else {
        starFish()
            ->platformWindow()
            ->webView()
            ->mainBrowsingContext()
            ->document()
            ->setDocumentURI(url);
    }

    return true;
}

uint32_t History::length()
{
    return m_historyEntries.size();
}

ScriptValue History::state()
{
    return currentHistoryEntry()->state();
}

bool History::isPushState()
{
    return currentHistoryEntry()->isPushState();
}

void History::pushState(ScriptValue state, String* title, Nullable<String*> url)
{
    String* urlValue = String::emptyString;
    if (url.hasValue()) {
        urlValue = url.getValue();
    }
    ResourceURL* newURL = new ResourceURL(urlValue, starFish()
                                                        ->platformWindow()
                                                        ->webView()
                                                        ->mainBrowsingContext()
                                                        ->document()
                                                        ->urlString());
    setHistory(state, title, newURL, true);
    starFish()
        ->platformWindow()
        ->webView()
        ->mainBrowsingContext()
        ->document()
        ->setDocumentURI(newURL);
}

void History::replaceState(ScriptValue state, String* title,
                           Nullable<String*> url)
{
    String* urlValue = String::emptyString;
    if (url.hasValue()) {
        urlValue = url.getValue();
    }
    ResourceURL* newURL = new ResourceURL(urlValue, starFish()
                                                        ->platformWindow()
                                                        ->webView()
                                                        ->mainBrowsingContext()
                                                        ->document()
                                                        ->urlString());
    m_historyEntries[m_offset]->replaceState(state, title, newURL);
    starFish()
        ->platformWindow()
        ->webView()
        ->mainBrowsingContext()
        ->document()
        ->setDocumentURI(newURL);
}

void History::setHistory(ScriptValue state, String* title, ResourceURL* url,
                         bool isPushState)
{
    if (starFish()
            ->platformWindow()
            ->webView()
            ->mainBrowsingContext()
            ->document() &&
        starFish()
            ->platformWindow()
            ->webView()
            ->mainBrowsingContext()
            ->document()
            ->urlString()
            ->equals(url->urlString())) {
        return;
    }

    STARFISH_ASSERT(m_offset <= length() - 1);
    m_offset++;
    m_historyEntries.erase(m_historyEntries.begin() + m_offset,
                           m_historyEntries.end());

    m_historyEntries.push_back(
        new HistoryEntry(state, title, url, isPushState));
}
} /* namespace StarFish */
