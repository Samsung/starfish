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

#ifdef PORT_CANVAS_BACKEND_SKIA
#ifndef __FontImplSkia__
#define __FontImplSkia__

#include "SkTypeface.h"

#include <fontconfig/fontconfig.h>
#include <hb.h>
#include <hb-ft.h>
#include <hb-icu.h>

#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/modules/canvas/font/Font.h"

class SkPaint;

namespace StarFish {
class FontSelectorImplSkia : public FontSelector {
public:
    FontSelectorImplSkia(Document* document,
                         PlatformFontSelector* platformFontData,
                         PlatformFontCache* platformFontCache)
        : FontSelector(document, platformFontData, platformFontCache)
    {
    }
};

class FontFaceImplSkia : public FontFace {
public:
    FontFaceImplSkia(FT_Face face, sk_sp<SkTypeface> skTypeface,
                     hb_font_t* hbFace, uint8_t* dataBuffer = nullptr,
                     size_t dataBufferSize = 0);

    virtual FontMetrics metrics(float size);
    virtual void clearCache() override;
    bool loadGlyph(
        int intSize, char32_t ch,
        std::pair<FontFaceImplSkia*, std::pair<unsigned, LayoutUnit>>& result);

    virtual size_t dataSize()
    {
        return m_dataBufferSize;
    }

    FT_Face freetypeFace()
    {
        ensureFonts();
        return m_face;
    }

    sk_sp<SkTypeface> skTypeFace()
    {
        return m_skTypeFace;
    }

    SkPaint* skPaint()
    {
        return m_skPaint;
    }

    hb_font_t* harfbuzzFace()
    {
        ensureFonts();
        return m_hbFace;
    }

    uint8_t* m_dataBuffer;
    size_t m_dataBufferSize;
    int m_xHeight;
    int m_unitsPerEM;
    int m_ascender;
    int m_descender;
    bool m_supportsKerning;

    typedef std::unordered_map<char32_t, std::pair<unsigned, unsigned>>
        GlyphIndexCache;
    GlyphIndexCache m_glyphIndexCache;

private:
    void ensureFonts();

    FT_Face m_face;
    sk_sp<SkTypeface> m_skTypeFace;
    SkPaint* m_skPaint;
    hb_font_t* m_hbFace;
};

class FontSkiaTextRun {
public:
    StringView m_text;
    size_t m_faceIndex;
    FT_Face m_ftFace;
    sk_sp<SkTypeface> m_skTypeFace;
    SkPaint* m_skPaint;
    hb_font_t* m_hbFont;
    hb_script_t m_script;
    std::vector<unsigned> m_glyphs;
    std::vector<LayoutLocation> m_glyphPositions;
    LayoutUnit m_runWidth;
};

class FontImplSkia : public Font {
public:
    friend class FontSelectorImplSkia;

    FontImplSkia(FontSelectorImplSkia* fontSelector)
    {
        m_fontSelector = fontSelector;
        m_spaceWidth = 0;
    }

    ~FontImplSkia()
    {
    }

    virtual LayoutUnit measureText(const StringView& str) override;

    std::pair<std::pair<FontFaceImplSkia*, size_t>,
              std::pair<unsigned, LayoutUnit>>
    loadGlyph(char32_t ch);
};

class PlatformFontSelectorImplSkia : public PlatformFontSelector {
public:
    PlatformFontSelectorImplSkia(StarFish* sf);

    ~PlatformFontSelectorImplSkia()
    {
    }

    virtual UTF8StringDataNonGCStd findFont(
        const UTF8StringDataNonGCStd& familyName, bool isGenericName,
        char style, char weight) override;
    virtual FontFace* loadFontFace(const UTF8StringDataNonGCStd& path) override;

    GCUnorderedMap<UTF8StringDataNonGCStd, FontFaceImplSkia*> m_fontPathToFace;
};

class PlatformFontCacheImplSkia : public PlatformFontCache {
public:
    GCVector<std::tuple<int, FontFaceImplSkia*, char, char>>
        m_fallbackFontFaceCachePerCodeBlock;
};

std::pair<std::pair<FontFaceImplSkia*, size_t>, std::pair<unsigned, LayoutUnit>>
skiaBackendInternalLoadGlyph(Font* f, char32_t ch);
std::vector<FontSkiaTextRun> generateFontSkiaTextRuns(const String* text,
                                                      FontImplSkia* font);
bool skiaBackendCanUseSimpleFontPath(Font* f, const StringView& sv);
};

#endif
#endif
