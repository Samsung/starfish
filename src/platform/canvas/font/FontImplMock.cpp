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

#include "StarFishConfig.h"

#if defined(PORT_CANVAS_BACKEND_MOCK)
#include "StarFish.h"

#include "core/style/UnitHelper.h"

namespace StarFish {

class FontFaceImplMock : public FontFace {
public:
    FontFaceImplMock()
    {
    }

    virtual FontMetrics metrics(float size)
    {
        FontMetrics fm;
        fm.m_ascender = size * 0.8;
        fm.m_ascender = size * 0.2;
        fm.m_fontHeight = size;
        fm.m_xheightRate = 0.8;
        return fm;
    }
};

class FontImplMock : public Font {
public:
    FontImplMock()
    {
    }

    virtual LayoutUnit measureText(const StringView& str)
    {
        return size() * str.length();
    }

    virtual void* unwrap()
    {
        return nullptr;
    }

protected:
};

class FontSelectorImplMock : public FontSelector {
public:
    FontSelectorImplMock(Document* document,
                         PlatformFontSelector* platformFontData,
                         PlatformFontCache* platformFontCache)
        : FontSelector(document, platformFontData, platformFontCache)
    {
    }

    FontFace* loadFontFaceImpl(String* familyName, float size, char style,
                               char weight)
    {
        FontFaceImplMock* f = new FontFaceImplMock();
        return f;
    }
};

class PlatformFontSelectorImplMock : public PlatformFontSelector {
public:
    PlatformFontSelectorImplMock(StarFish* sf)
        : PlatformFontSelector(sf)
    {
        m_face = new FontFaceImplMock();
    }

    ~PlatformFontSelectorImplMock()
    {
    }

    virtual UTF8StringDataNonGCStd findFont(
        const UTF8StringDataNonGCStd& familyName, bool isGenericName,
        char style, char weight) override
    {
        return UTF8StringDataNonGCStd(" ");
    }

    virtual FontFace* loadFontFace(const UTF8StringDataNonGCStd& path) override
    {
        return m_face;
    }

    FontFaceImplMock* m_face;
};

class PlatformFontCacheImplMock : public PlatformFontCache {
public:
};

PlatformFontSelector* PlatformFontSelector::create(StarFish* sf)
{
    return new PlatformFontSelectorImplMock(sf);
}

PlatformFontCache* PlatformFontCache::create(StarFish* sf)
{
    return new PlatformFontCacheImplMock();
}

Font* Font::createEmptyFont(FontSelector* s)
{
    return new FontImplMock();
}

FontSelector* FontSelector::create(Document* document,
                                   PlatformFontSelector* platformFontSelector,
                                   PlatformFontCache* platformFontCache)
{
    return new FontSelectorImplMock(document, platformFontSelector,
                                    platformFontCache);
}
}
#endif
