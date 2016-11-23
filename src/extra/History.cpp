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
#include "platform/window/Window.h"

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
        starFish()->window()->navigateAsyncWithoutSetHistory(getURL(0)); // refresh
}

int& History::offset()
{
    return starFish()->offset();
}

std::vector<HistoryEntry*, gc_allocator_ignore_off_page<HistoryEntry*>>& History::history()
{
    return starFish()->history();
}

URL* History::getURL(int delta)
{
    return history()[offset() + delta]->url();
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

    starFish()->window()->navigateAsyncWithoutSetHistory(getURL(delta));
    offset() += delta;

    return true;
}

int History::length()
{
    return history().size();
}

void History::setHistory(URL* url)
{
    if (offset() != length() - 1) {
        auto it = history().begin() + (offset() + 1);
        while (it != history().end()) {
            it = history().erase(it);
        }
    }
    offset()++;
    history().push_back(new HistoryEntry(nullptr, nullptr, url));

}

} /* namespace StarFish */
