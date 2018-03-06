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
#include "core/dom/Touch.h"
#include "core/dom/TouchList.h"
#include "core/dom/TouchEvent.h"

namespace StarFish {

TouchEvent::TouchEvent(Document* document)
    : UIEvent(document)
    , m_touches(new TouchList(document))
{
}

TouchEvent::TouchEvent(Document* document, String* eventType)
    : UIEvent(document, eventType)
    , m_touches(new TouchList(document))
{
}

TouchEvent::TouchEvent(Document* document, String* eventType, TouchData* data,
                       size_t touchCount)
    : TouchEvent(document, eventType)
{
    for (size_t i = 0; i < touchCount; i++) {
        m_touches->push_back(new Touch(document, data[i]));
    }
}
}
