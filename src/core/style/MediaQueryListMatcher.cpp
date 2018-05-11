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
#include "core/dom/EventTarget.h"
#include "core/dom/MediaQueryListEvent.h"
#include "core/style/CSSParser.h"
#include "core/style/MediaQueryEvaluator.h"
#include "core/style/MediaQueryList.h"
#include "core/style/MediaQueryListMatcher.h"

namespace StarFish {

MediaQueryList* MediaQueryListMatcher::matchMedia(String* query)
{
    if (!document()) {
        return nullptr;
    }

    CSSParser parser(document());
    RefPtr<CSSToken> token = parser.makeToken(query);
    MediaQuerySet* mediaQuerySet = parser.parseMediaQuery();
    MediaQueryList* mediaQueryList =
        new MediaQueryList(document(), this, mediaQuerySet);
    return mediaQueryList;
}

void MediaQueryListMatcher::mediaFeaturesChanged()
{
    for (auto& list : m_mediaQueryLists) {
        // TODO: Check that this MediaQueryList has listener or not
        if (list.first->matches() != list.second) {
            // Fire the change event
            const MediaQueryListEventInit mediaQueryListEventInit =
                MediaQueryListEventInit(list.first->media(),
                                        list.first->matches());
            auto mediaQueryListEvent = new MediaQueryListEvent(
                document(),
                document()->starFish()->staticStrings()->m_change.localName(),
                mediaQueryListEventInit);
            document()->dispatchEventByUA(list.first, mediaQueryListEvent,
                                          true);

            // Update dirty flag
            list.second = !list.second;
        }
    }
}

void MediaQueryListMatcher::addMediaQueryList(MediaQueryList* list)
{
    m_mediaQueryLists.push_back(std::make_pair(list, list->matches()));
}

} /* namespace StarFish */
