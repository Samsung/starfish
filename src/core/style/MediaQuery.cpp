/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
/*
 * CSS Media Query
 *
 * Copyright (C) 2005, 2006 Kimmo Kinnunen <kimmo.t.kinnunen@nokia.com>.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "StarFishConfig.h"
#include "MediaQuery.h"
#include "CSSParser.h"
#include "core/util/String.h"

namespace StarFish {

MediaQuery* MediaQuery::createNotAll()
{
    return new MediaQuery(MediaQuery::Not, String::createASCIIString("all"),
                          GCVector<MediaQueryExp*>());
}

MediaQuery* MediaQuery::create(RestrictorType restrictor, String* mediaType,
                               GCVector<MediaQueryExp*> expressions)
{
    return new MediaQuery(restrictor, mediaType, std::move(expressions));
}

MediaQuery::MediaQuery(RestrictorType restrictor, String* mediaType,
                       GCVector<MediaQueryExp*> expressions)
    : m_restrictor(restrictor)
    , m_mediaType(mediaType->toASCIILower())
    , m_expressions(std::move(expressions))
{
}

MediaQuery::MediaQuery(const MediaQuery& o)
    : m_restrictor(o.m_restrictor)
    , m_mediaType(o.m_mediaType)
{
    m_expressions.clear();
    m_expressions.assign(o.m_expressions.begin(), o.m_expressions.end());
}

// https://drafts.csswg.org/cssom/#serializing-media-queries
String* MediaQuery::serialize() const
{
    StringBuilder result;
    switch (m_restrictor) {
    case MediaQuery::Only:
        result.appendString("only ");
        break;
    case MediaQuery::Not:
        result.appendString("not ");
        break;
    case MediaQuery::None:
        break;
    }

    if (m_expressions.empty()) {
        result.appendString(m_mediaType);
        return result.finalize();
    }

    if (!m_mediaType->equals("all") || m_restrictor != None) {
        result.appendString(m_mediaType);
        result.appendString(" and ");
    }

    result.appendString(m_expressions.at(0)->serialize());
    for (size_t i = 1; i < m_expressions.size(); ++i) {
        result.appendString(String::createASCIIString(" and "));
        result.appendString(m_expressions.at(i)->serialize());
    }
    return result.finalize();
}

String* MediaQuery::cssText() const
{
    return serialize();
}

bool MediaQuery::operator==(const MediaQuery& other) const
{
    return cssText()->equals(other.cssText());
}
}
