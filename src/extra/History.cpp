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
#include "StarFish.h"
#include "dom/Document.h"
#include "extra/History.h"
#include "extra/HistoryEntry.h"
#include "extra/Location.h"
#include "platform/window/Window.h"

namespace StarFish {

History::History(StarFish* starFish)
    : ScriptWrappable(this)
    , m_starFish(starFish)
    , m_offset(SIZE_MAX)
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
        starFish()->window()->navigateAsyncWithoutSetHistory(
            currentHistoryEntry()->url());
    }
}

URL* History::getURL()
{
    return m_historyEntries[m_offset]->url();
}

bool History::navigate(int delta)
{
    STARFISH_ASSERT(delta != 0);

    if (delta > 0) {
        STARFISH_ASSERT(length() - 1 >= m_offset);
        if (static_cast<size_t>(delta) > length() - m_offset - 1) {
            return false;
        }
    } else {
        // delta < 0
        if (m_offset < static_cast<size_t>(-delta)) {
            return false;
        }
    }

    m_offset += delta;

    URL* url = currentHistoryEntry()->url();
    if (!isPushState()) {
        starFish()->window()->navigateAsyncWithoutSetHistory(url);
    } else {
        starFish()->window()->document()->setDocumentURI(url);
    }

    return true;
}

size_t History::length()
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

void History::pushState(ScriptValue state, String* title, String* url)
{
    URL* newURL =
        URL::createURL(starFish()->window()->document()->urlString(), url);
    setHistory(state, title, newURL, true);
    starFish()->window()->document()->setDocumentURI(newURL);
}

void History::replaceState(ScriptValue state, String* title, String* url)
{
    URL* newURL =
        URL::createURL(starFish()->window()->document()->urlString(), url);
    m_historyEntries[m_offset]->replaceState(state, title, newURL);
    starFish()->window()->document()->setDocumentURI(newURL);
}

void History::setHistory(ScriptValue state, String* title, URL* url,
                         bool isPushState)
{
    if (starFish()->window()->document() &&
        starFish()->window()->document()->urlString()->equals(
            url->urlString())) {
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
