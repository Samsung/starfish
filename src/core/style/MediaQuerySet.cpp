/*
 * Copyright (C) 2004, 2006, 2010, 2012 Apple Inc. All rights reserved.
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "MediaQuerySet.h"

namespace StarFish {
MediaQuerySet::MediaQuerySet()
{
}

MediaQuerySet::MediaQuerySet(MediaQuerySet& o)
{
    m_queries.clear();
    m_queries.assign(o.queryVector().begin(), o.queryVector().end());
}

void MediaQuerySet::addMediaQuery(MediaQuery* mediaQuery)
{
    m_queries.push_back(mediaQuery);
}

String* MediaQuerySet::mediaText() const
{
    String* text = String::emptyString;

    bool first = true;
    for (size_t i = 0; i < m_queries.size(); ++i) {
        if (!first) {
            text->concat(String::createASCIIString(", "));
        } else {
            first = false;
        }
        text->concat(m_queries[i]->cssText());
    }
    return text;
}
}
