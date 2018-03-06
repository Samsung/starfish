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
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/style/MediaQueryEvaluator.h"
#include "core/style/MediaQueryList.h"
#include "core/style/MediaQuerySet.h"

namespace StarFish {

ScriptBindingInstance* MediaQueryList::scriptBindingInstance()
{
    return m_scriptBindingInstance;
}

String* MediaQueryList::media() const
{
    return m_media->mediaText();
}

bool MediaQueryList::matches()
{
    return m_evaluator->eval(m_media);
}

void MediaQueryList::addListener(EventListener* listener)
{
    // https://drafts.csswg.org/cssom-view/#dom-mediaquerylist-addlistener
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void MediaQueryList::removeListener(EventListener* listener)
{
    // https://drafts.csswg.org/cssom-view/#dom-mediaquerylist-removelistener
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

} /* namespace StarFish */
