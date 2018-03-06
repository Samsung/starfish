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

#ifndef __StarFishMediaQueryEvaluator__
#define __StarFishMediaQueryEvaluator__

namespace StarFish {

class MediaQuery;
class MediaQueryExp;
class MediaQuerySet;
class MediaQueryResult;
class MediaValues;

using MediaQueryResultList = GCVector<MediaQueryResult*>;

class MediaQueryEvaluator : public gc {
public:
    MediaQueryEvaluator(MediaValues* mediaValues)
        : m_mediaType(String::emptyString)
        , m_mediaValues(mediaValues)
    {
    }

    MediaQueryEvaluator(String* mediaType, MediaValues* mediaValues)
        : m_mediaType(mediaType)
        , m_mediaValues(mediaValues)
    {
    }

    bool mediaTypeMatch(String* mediaType) const;

    bool eval(MediaQuerySet* mediaQueries,
              MediaQueryResultList* viewportDependentResult = nullptr,
              MediaQueryResultList* deviceDependentResult = nullptr) const;
    bool eval(MediaQuery* query, MediaQueryResultList* viewportDependentResult,
              MediaQueryResultList* deviceDependentResult) const;
    bool eval(MediaQueryExp*) const;

private:
    String* m_mediaType;
    MediaValues* m_mediaValues;
};

} /* namespace StarFish */

#endif /* __StarFishMediaQueryEvaluator__ */
