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

#ifndef __FontImplCairo__
#define __FontImplCairo__

#include <cairo.h>
#include <cairo/cairo-ft.h>
#include <fontconfig/fontconfig.h>
#include <hb.h>
#include <hb-ft.h>
#include <hb-icu.h>

#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/modules/canvas/font/Font.h"

namespace StarFish {

extern FT_Library g_freeTypeInstance;
#define CHECK_ERROR                            \
    if (error) {                               \
        STARFISH_RELEASE_ASSERT_NOT_REACHED(); \
    }

class FontSelectorImplCairo : public FontSelector {
public:
    typedef std::unordered_map<char32_t, std::pair<unsigned, unsigned>>
        GlyphIndexCachePerFace;
    std::unordered_map<FT_Face, std::unique_ptr<GlyphIndexCachePerFace>>
        m_glyphIndexCache;

    FontSelectorImplCairo(Document* document,
                          PlatformFontSelector* platformFontData,
                          PlatformFontCache* platformFontCache)
        : FontSelector(document, platformFontData, platformFontCache)
    {
    }
};

class FontFaceImplCairo : public FontFace {
public:
    FontFaceImplCairo(FT_Face face, hb_font_t* hbFace,
                      uint8_t* dataBuffer = nullptr, size_t dataBufferSize = 0)
    {
        m_dataBuffer = dataBuffer;
        m_dataBufferSize = dataBufferSize;
        m_face = face;
        m_hbFace = hbFace;

        FT_Error error;
        FT_UInt glyph_index = FT_Get_Char_Index(m_face, 'x');
        if (glyph_index) {
            error = FT_Load_Glyph(m_face, glyph_index, FT_LOAD_NO_SCALE);
            m_xHeight = (int)m_face->glyph->metrics.height;
        } else {
            m_xHeight = 0;
        }

        m_unitsPerEM = m_face->units_per_EM;
        if (m_unitsPerEM == 0) {
            m_unitsPerEM = 2048; // fallback
        }
        m_ascender = m_face->ascender;
        m_descender = m_face->descender;
        if (m_ascender == 0 || m_descender == 0) {
            m_ascender = m_unitsPerEM; // fallback
            m_descender = 0;
        }

        m_supportsKerning = m_face->face_flags & FT_FACE_FLAG_KERNING;
        m_xHeight += m_descender;
        if (m_xHeight < 0) {
            m_xHeight = 0;
        }

        GC_REGISTER_FINALIZER_NO_ORDER(
            this,
            [](void* obj, void* cd) {
                FontFaceImplCairo* m = (FontFaceImplCairo*)obj;
                if (m->m_hbFace) {
                    hb_font_destroy(m->m_hbFace);
                    FT_Done_Face(m->m_face);
                }
                GlyphIndexCache().swap(m->m_glyphIndexCache);
                free(m->m_dataBuffer);
            },
            NULL, NULL, NULL);

        clearCache();
    }

    virtual FontMetrics metrics(float size)
    {
        int intSize = int(size + 0.5f);

        FontMetrics met;
        met.m_fontHeight =
            ((m_ascender - m_descender) * intSize) / m_unitsPerEM;
        met.m_ascender = ((m_ascender * intSize) / (m_unitsPerEM));
        met.m_descender = met.m_ascender - met.m_fontHeight;
        met.m_xheightRate = (float)m_xHeight / (float)m_unitsPerEM;

#ifdef STARFISH_ENABLE_TEST
        if (g_enablePixelTest) {
            // Set the FontMetrics as if font is Ahem.
            met.m_ascender = size * 0.8;
            met.m_descender = met.m_ascender - size;
            met.m_fontHeight = met.m_ascender - met.m_descender;
            met.m_xheightRate = 0.8f;
        }
#endif
        return met;
    }

    virtual void clearCache()
    {
        if (m_face && m_dataBuffer) {
            hb_font_destroy(m_hbFace);
            FT_Done_Face(m_face);
            m_face = nullptr;
            m_hbFace = nullptr;
            GlyphIndexCache().swap(m_glyphIndexCache);
        }
    }

    bool loadGlyph(
        int intSize, char32_t ch,
        std::pair<FontFaceImplCairo*, std::pair<unsigned, LayoutUnit>>& result)
    {
        ensureFonts();
        FT_Face face = m_face;
        hb_font_t* hbFace = m_hbFace;

        auto iter = m_glyphIndexCache.find(ch);
        if (iter != m_glyphIndexCache.end()) {
            if (iter->second.first) {
                LayoutUnit width =
                    LayoutUnit((int)(iter->second.second * intSize)) /
                    LayoutUnit((int)(m_unitsPerEM));
                result = std::make_pair(
                    this, std::make_pair(iter->second.first, width));
                return true;
            }
        } else {
            FT_UInt glyphIndex = FT_Get_Char_Index(face, ch);
            FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_SCALE);

            m_glyphIndexCache.insert(std::make_pair(
                ch,
                std::make_pair(glyphIndex, face->glyph->metrics.horiAdvance)));

            if (glyphIndex) {
                LayoutUnit width =
                    LayoutUnit(
                        (int)(face->glyph->metrics.horiAdvance * intSize)) /
                    LayoutUnit((int)(m_unitsPerEM));
                result =
                    std::make_pair(this, std::make_pair(glyphIndex, width));
                return true;
            }
        }
        return false;
    }

    virtual size_t dataSize()
    {
        return m_dataBufferSize;
    }

    FT_Face freetypeFace()
    {
        ensureFonts();
        return m_face;
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
    void ensureFonts()
    {
        if (m_face == nullptr) {
            FT_Error error = FT_New_Memory_Face(
                g_freeTypeInstance, m_dataBuffer, m_dataBufferSize, 0, &m_face);
            if (error) {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            FT_Set_Pixel_Sizes(m_face, 0, 16);

            FT_UInt glyph_index = FT_Get_Char_Index(m_face, ' ');
            if (glyph_index) {
                FT_Load_Glyph(m_face, glyph_index, FT_LOAD_RENDER);
            }

            m_hbFace = hb_ft_font_create(m_face, [](void* userData) {});
        }
    }
    FT_Face m_face;
    hb_font_t* m_hbFace;
};

class FontCairoTextRun {
public:
    StringView m_text;
    size_t m_faceIndex;
    FT_Face m_ftFace;
    hb_font_t* m_hbFont;
    hb_script_t m_script;
    std::vector<unsigned> m_glyphs;
    std::vector<LayoutLocation> m_glyphPositions;
    LayoutUnit m_runWidth;
};

class FontImplCairo : public Font {
public:
    friend class FontSelectorImplCairo;

    FontImplCairo(FontSelectorImplCairo* fontSelector)
    {
        m_fontSelector = fontSelector;
        m_spaceWidth = 0;
    }

    ~FontImplCairo()
    {
    }

    virtual LayoutUnit measureText(const StringView& str);

    std::pair<std::pair<FontFaceImplCairo*, size_t>,
              std::pair<unsigned, LayoutUnit>>
    loadGlyph(char32_t ch);
};

class PlatformFontSelectorImplCairo : public PlatformFontSelector {
public:
    PlatformFontSelectorImplCairo(StarFish* sf)
        : PlatformFontSelector(sf)
    {
        if (!g_freeTypeInstance) {
            FT_Error error;
            error = FT_Init_FreeType(&g_freeTypeInstance);
            CHECK_ERROR;
        }
    }

    ~PlatformFontSelectorImplCairo()
    {
    }

    virtual UTF8StringDataNonGCStd findFont(
        const UTF8StringDataNonGCStd& familyName, bool isGenericName,
        char style, char weight) override
    {
        // http://www.w3.org/TR/css3-fonts/#font-matching-algorithm
        FcPattern* pattern = FcPatternCreate();
        auto u8FamilyName = familyName;

        if (!FcPatternAddString(pattern, FC_FAMILY,
                                (const FcChar8*)u8FamilyName.data())) {
            FcPatternDestroy(pattern);
            return UTF8StringDataNonGCStd();
        }

        if (style == FontStyleItalic) {
            if (!FcPatternAddInteger(pattern, FC_SLANT, FC_SLANT_ITALIC)) {
                FcPatternDestroy(pattern);
                return UTF8StringDataNonGCStd();
            }
        } else if (style == FontStyleOblique) {
            if (!FcPatternAddInteger(pattern, FC_SLANT, FC_SLANT_OBLIQUE)) {
                FcPatternDestroy(pattern);
                return UTF8StringDataNonGCStd();
            }
        } else {
            if (!FcPatternAddInteger(pattern, FC_SLANT, FC_SLANT_ROMAN)) {
                FcPatternDestroy(pattern);
                return UTF8StringDataNonGCStd();
            }
        }

        int fontWeight = FC_WEIGHT_MEDIUM;
        switch (weight) {
        case 1:
            fontWeight = FC_WEIGHT_THIN;
            break;
        case 2:
            fontWeight = FC_WEIGHT_ULTRALIGHT;
            break;
        case 3:
            fontWeight = FC_WEIGHT_LIGHT;
            break;
        case 4:
            fontWeight = FC_WEIGHT_REGULAR;
            break;
        case 5:
            fontWeight = FC_WEIGHT_MEDIUM;
            break;
        case 6:
            fontWeight = FC_WEIGHT_SEMIBOLD;
            break;
        case 7:
            fontWeight = FC_WEIGHT_BOLD;
            break;
        case 8:
            fontWeight = FC_WEIGHT_ULTRABOLD;
            break;
        case 9:
            fontWeight = FC_WEIGHT_ULTRABLACK;
            break;
        default:
            STARFISH_ASSERT_NOT_REACHED();
        }

        if (!FcPatternAddInteger(pattern, FC_WEIGHT, fontWeight)) {
            FcPatternDestroy(pattern);
            return UTF8StringDataNonGCStd();
        }

        FcPatternAddBool(pattern, FC_SCALABLE, FcTrue);

        // The strategy is originally from Skia
        // (src/ports/SkFontHost_fontconfig.cpp):
        // Allow Fontconfig to do pre-match substitution. Unless we are
        // accessing a "fallback"
        // family like "sans," this is the only time we allow Fontconfig to
        // substitute one
        // family name for another (i.e. if the fonts are aliased to each
        // other).
        FcConfigSubstitute(NULL, pattern, FcMatchPattern);
        FcDefaultSubstitute(pattern);

        FcResult fontConfigResult;
        FcPattern* resultPattern =
            FcFontMatch(NULL, pattern, &fontConfigResult);
        if (!resultPattern) {
            FcPatternDestroy(pattern);
            return UTF8StringDataNonGCStd();
        }

        FcChar8* fontNameAfterMatch;
        FcPatternGetString(resultPattern, FC_FAMILY, 0, &fontNameAfterMatch);
        UTF8StringDataNonGCStd after = (char*)fontNameAfterMatch;
        std::transform(after.begin(), after.end(), after.begin(), ::tolower);

        if (familyName !=
            m_starfish->initialFontFamilyDatas()[1]
                .m_familyName->toUTF8NonGCString()) {
            if (after != familyName) {
                if (!isGenericName) {
                    return UTF8StringDataNonGCStd();
                }
            }
        }

        FcChar8* filePath = NULL;
        if (!FcPatternGetString(resultPattern, FC_FILE, 0, &filePath) ==
            FcResultMatch) {
            return UTF8StringDataNonGCStd();
        }
        std::string u8FilePath = (char*)filePath;

        FcPatternDestroy(resultPattern);
        FcPatternDestroy(pattern);

        return u8FilePath;
    }

    virtual FontFace* loadFontFace(const UTF8StringDataNonGCStd& path) override
    {
        auto iter = m_fontPathToFace.find(path);
        if (iter != m_fontPathToFace.end()) {
            return iter->second;
        }
        FT_Face face;
        FT_Error error;
        error = FT_New_Face(g_freeTypeInstance, (char*)path.data(), 0, &face);
        CHECK_ERROR;
        FT_Set_Pixel_Sizes(face, 0, 16);
        auto hbFace = hb_ft_font_create(face, [](void* userData) {});

        auto impl = new (PointerFreeGC) FontFaceImplCairo(face, hbFace);
        m_fontPathToFace.insert(std::make_pair(path, impl));
        return impl;
    }

    GCUnorderedMap<UTF8StringDataNonGCStd, FontFaceImplCairo*> m_fontPathToFace;
};

class PlatformFontCacheImplCairo : public PlatformFontCache {
public:
    GCVector<std::tuple<int, FontFaceImplCairo*, char, char>>
        m_fallbackFontFaceCachePerCodeBlock;
};

std::pair<std::pair<FontFaceImplCairo*, size_t>,
          std::pair<unsigned, LayoutUnit>>
cairoBackendInternalLoadGlyph(Font* f, char32_t ch);
std::vector<FontCairoTextRun> generateFontCairoTextRuns(const String* text,
                                                        FontImplCairo* font);
bool cairoBackendCanUseSimpleFontPath(Font* f, const StringView& sv);
};

#endif
