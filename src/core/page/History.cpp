/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
