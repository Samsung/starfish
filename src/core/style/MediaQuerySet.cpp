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

#include "StarFishConfig.h"
#include "MediaQuerySet.h"
#include "core/dom/Document.h"
#include "core/style/CSSParser.h"

namespace StarFish {
MediaQuerySet::MediaQuerySet(Document* document)
    : m_document(document)
{
}

MediaQuerySet::MediaQuerySet(MediaQuerySet& o)
{
    m_document = o.document();
    m_queries.clear();
    m_queries.assign(o.queryVector().begin(), o.queryVector().end());
}

void MediaQuerySet::addMediaQuery(MediaQuery* mediaQuery)
{
    m_queries.push_back(mediaQuery);
}

String* MediaQuerySet::mediaText() const
{
    StringBuilder text;

    bool first = true;
    for (size_t i = 0; i < m_queries.size(); ++i) {
        if (!first) {
            text.appendString(", ");
        } else {
            first = false;
        }
        text.appendString(m_queries[i]->cssText());
    }
    return text.finalize();
}

MediaQuerySet* MediaQuerySet::create(String* mediaString)
{
    if (mediaString->equals(String::emptyString)) {
        return MediaQuerySet::create(m_document);
    }

    CSSParser parser(m_document);
    parser.makeToken(mediaString);
    return parser.parseMediaQuery();
}

bool MediaQuerySet::set(String* mediaString)
{
    MediaQuerySet* result = create(mediaString);
    m_queries.swap(result->m_queries);
    return true;
}

void MediaQuerySet::add(String* mediaString)
{
    MediaQuerySet* result = create(mediaString);

    if (result->m_queries.size() != 1) {
        return;
    }

    MediaQuery* newQuery = result->m_queries[0];
    STARFISH_ASSERT(newQuery);

    for (size_t i = 0; i < m_queries.size(); ++i) {
        MediaQuery* query = m_queries[i];
        if (*query == *newQuery) {
            return;
        }
    }

    m_queries.push_back(newQuery);
}

bool MediaQuerySet::remove(String* mediaString)
{
    MediaQuerySet* result = create(mediaString);

    if (result->m_queries.size() != 1) {
        return true;
    }

    MediaQuery* newQuery = result->m_queries[0];
    STARFISH_ASSERT(newQuery);

    bool found = false;
    m_queries.erase(std::remove_if(m_queries.begin(), m_queries.end(),
                                   [newQuery, &found](MediaQuery* query) {
                                       if (*query == *newQuery) {
                                           found = true;
                                           return found;
                                       }
                                       return false;
                                   }),
                    m_queries.end());

    return found;
}

Document* MediaQuerySet::document() const
{
    return m_document;
}
}
