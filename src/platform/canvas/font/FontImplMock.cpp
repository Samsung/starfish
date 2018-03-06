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
