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

#include "core/style/CSSParser.h"
#include "core/style/MediaValues.h"

namespace StarFish {

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

    bool eval(MediaQuerySet* mediaQueries) const;
    bool eval(MediaQuery* query) const;
    bool eval(MediaQueryExp*) const;

private:
    String* m_mediaType;
    MediaValues* m_mediaValues;
};

} /* namespace StarFish */

#endif /* __StarFishMediaQueryEvaluator__ */
