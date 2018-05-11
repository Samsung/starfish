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

#ifndef __StarFishMediaQueryListMatcher__
#define __StarFishMediaQueryListMarcher__

#include "binding/DocumentHoldable.h"

namespace StarFish {

class Document;
class MediaQueryEvaluator;

class MediaQueryListMatcher : public DocumentHoldable, public gc {
public:
    MediaQueryListMatcher(Document* document, MediaQueryEvaluator* evaluator)
        : DocumentHoldable(document)
    {
    }

    MediaQueryList* matchMedia(String* query);
    void mediaFeaturesChanged();

    void addMediaQueryList(MediaQueryList* list);

private:
    GCVector<std::pair<MediaQueryList*, bool>> m_mediaQueryLists;
};

} /* namespace StarFish */

#endif /* __StarFishMediaQueryListMatcher__ */
