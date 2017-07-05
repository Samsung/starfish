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
