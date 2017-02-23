/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#include "History.h"
#include "HistoryEntry.h"
#include "Location.h"

namespace StarFish {

History::History(StarFish* starFish)
    : ScriptWrappable(this)
    , m_starFish(starFish)
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
    if (delta)
        navigateBackForward(delta); // navigate according to history
    else
        starFish()->window()->navigateAsyncWithoutSetHistory(getURL(0)); // refresh: delta is 0 or empty
}

int& History::offset()
{
    return starFish()->offset();
}

GCVector<HistoryEntry*>& History::history()
{
    return starFish()->history();
}

URL* History::getURL(int delta)
{
    return history()[offset()]->url();
}

int History::historyForwardListCount()
{
    return length() - historyBackListCount() - 1;
}

int History::historyBackListCount()
{
    return offset() < 0 ? 0 : offset();
}

bool History::navigateBackForward(int delta)
{
    if (delta > historyForwardListCount())
        return false;
    if (delta < -historyBackListCount())
        return false;

    offset() += delta;
    if (!isPushState()) {
        starFish()->window()->navigateAsyncWithoutSetHistory(getURL(delta));
    } else {
        starFish()->window()->document()->setDocumentURI(getURL(delta));
    }

    return true;
}

int History::length()
{
    return history().size();
}

String* History::state()
{
    return history()[offset()]->state();
}

bool History::isPushState()
{
    return history()[offset()]->isPushState();
}

void History::pushState(String* state, String* title, String* url)
{
    URL* newURL = URL::createURL(starFish()->window()->document()->documentURI()->urlString(), url);
    setHistory(state, title, newURL, true);
    starFish()->window()->document()->setDocumentURI(newURL);
}

void History::replaceState(String* state, String* title, String* url)
{
    URL* newURL = URL::createURL(starFish()->window()->document()->documentURI()->urlString(), url);
    history()[offset()]->replaceState(state, title, newURL);
    starFish()->window()->document()->setDocumentURI(newURL);
}

void History::setHistory(String* state, String* title, URL* url, bool isPushState)
{
    if (starFish()->window()->document() && starFish()->window()->document()->documentURI()->urlString()->equals(url->urlString()))
        return;

    if (offset() != length() - 1) {
        auto it = history().begin() + (offset() + 1);
        while (it != history().end()) {
            it = history().erase(it);
        }
    }
    offset()++;
    history().push_back(new HistoryEntry(state, title, url, isPushState));

}

} /* namespace StarFish */
