/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#ifndef __StarfishMediaQueryEvaluator__
#define __StarfishMediaQueryEvaluator__

#include <string>

namespace Starfish {

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

    // CDP Emulation.setEmulatedMedia override hooks. These are process-static
    // so a single Emulation override applies across documents, and they are
    // read at media query evaluation time (i.e. on each matchMedia() call), so
    // no re-evaluation plumbing is needed.
    //
    // Media type override: empty string means "no override" (use the real media
    // type). prefers-color-scheme override: 0 = no override, 1 = light, 2 =
    // dark. prefers-reduced-motion override: 0 = no override, 1 =
    // no-preference, 2 = reduce.
    static void setMediaTypeOverride(const std::string& mediaType);
    static const std::string& mediaTypeOverride();
    static void setPrefersColorSchemeOverride(int v);
    static int prefersColorSchemeOverride();
    static void setPrefersReducedMotionOverride(int v);
    static int prefersReducedMotionOverride();
    static void clearEmulatedMediaOverrides();

private:
    String* m_mediaType;
    MediaValues* m_mediaValues;
};

} /* namespace Starfish */

#endif /* __StarfishMediaQueryEvaluator__ */
