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

#include "core/page/History.h"

#include "browser/history/HistoryManager.h"
#include "core/dom/Document.h"
#include "core/page/Location.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"

namespace StarFish {

History::History(Document* doc)
    : ScriptWrappable(this)
    , DocumentHoldable(doc)
{
}

HistoryManager* History::historyManager()
{
    return document()->window()->browsingContext()->historyManager();
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
    if (delta == 0) {
        document()->window()->location()->reload();
    } else {
        historyManager()->go(delta);
    }
}

bool History::canGoBack()
{
    return historyManager()->canGo(-1);
}

bool History::canGoForward()
{
    return historyManager()->canGo(1);
}

uint32_t History::length()
{
    return historyManager()->length();
}

ScriptValue History::state()
{
    return historyManager()->state(document());
}

void History::pushState(ScriptValue state, String* title, Nullable<String*> url)
{
    historyManager()->pushState(document(), state, title, url);
}

void History::replaceState(ScriptValue state, String* title,
                           Nullable<String*> url)
{
    historyManager()->replaceState(document(), state, title, url);
}
}
