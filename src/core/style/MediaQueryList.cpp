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
#include "core/style/MediaQueryEvaluator.h"
#include "core/style/MediaQueryList.h"
#include "core/style/MediaQueryListMatcher.h"
#include "core/style/MediaQuerySet.h"
#include "core/page/Window.h"

namespace StarFish {

MediaQueryList::MediaQueryList(Document* document,
                               MediaQueryListMatcher* matcher,
                               MediaQuerySet* media)
    : EventTarget(document)
    , m_matcher(matcher)
    , m_media(media)
    , m_matches(false)
{
    m_matcher->addMediaQueryList(this);
}

String* MediaQueryList::media() const
{
    return m_media->mediaText();
}

bool MediaQueryList::matches()
{
    if (m_matches !=
        document()->styleResolver().mediaQueryEvaluator().eval(m_media)) {
        m_matches = !m_matches;
    }

    return m_matches;
}

void MediaQueryList::addListener(EventListener* listener)
{
    // https://drafts.csswg.org/cssom-view/#dom-mediaquerylist-addlistener
    if (!listener) {
        return;
    }

    String* changeEvent = starFish()->staticStrings()->m_change.localName();
    addEventListener(changeEvent, listener, false);
}

void MediaQueryList::removeListener(EventListener* listener)
{
    // https://drafts.csswg.org/cssom-view/#dom-mediaquerylist-removelistener
    if (!listener) {
        return;
    }

    String* changeEvent = starFish()->staticStrings()->m_change.localName();
    removeEventListener(changeEvent, listener, false);
}

DEFINE_EVENT_LISTENER(MediaQueryList, change);

} /* namespace StarFish */
