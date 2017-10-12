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

#if defined(PORT_CANVAS_BACKEND_CAIRO)
#include "StarFish.h"

#include <cairo.h>
#include <cairo/cairo-ft.h>
#include <fontconfig/fontconfig.h>
#include "core/modules/canvas/font/Font.h"
#include "core/style/UnitHelper.h"

namespace StarFish {

#define CHECK_ERROR                            \
    if (error) {                               \
        STARFISH_RELEASE_ASSERT_NOT_REACHED(); \
    }

class FontSelectorImplCairo;

class FontFaceImplCAIRO : public FontFace {
public:
    FontFaceImplCAIRO(String* familyName, FT_Face face, FontMetrics met,
                      float size, char style, char weight)
    {
        m_familyName = familyName;
        m_face = face;
        m_metrics = met;
        m_size = size;
        m_weight = weight;
        m_style = style;
    }

    FT_Face m_face;
};

class FontImplCAIRO : public Font {
public:
    friend class FontSelectorImplCairo;

    FontImplCAIRO(FontSelectorImplCairo* fontSelector)
    {
        m_fontSelector = fontSelector;
        m_spaceWidth = 0;
    }

    ~FontImplCAIRO()
    {
    }

    virtual LayoutUnit measureText(const StringView& str);

    virtual void* unwrap()
    {
        return nullptr;
    }

    FontSelectorImplCairo* m_fontSelector;
};

class FontSelectorImplCairo : public FontSelector {
public:
    FcConfig* m_fcconfig;
    FT_Library m_FTFaceLib;
    std::unordered_map<std::string, FT_Face> m_loadedFonts;

    typedef std::unordered_map<char32_t, std::pair<unsigned, unsigned>>
        GlyphIndexCachePerFace;
    std::unordered_map<FT_Face, std::unique_ptr<GlyphIndexCachePerFace>>
        m_glyphIndexCache;
    std::vector<std::tuple<int, FT_Face, char, char>>
        m_fallbackFontCachePerCodeBlock;

    FontSelectorImplCairo()
    {
        m_fcconfig = FcInitLoadConfigAndFonts();
        FT_Error error;
        error = FT_Init_FreeType(&m_FTFaceLib);
        CHECK_ERROR;
    }

    ~FontSelectorImplCairo()
    {
        FcConfigDestroy(m_fcconfig);
        FT_Done_FreeType(m_FTFaceLib);
    }

    FontFace* loadFontImpl(String* familyName, float size, char style,
                           char weight)
    {
        // http://www.w3.org/TR/css3-fonts/#font-matching-algorithm
        FcPattern* pattern = FcPatternCreate();

        auto u8FamilyName = familyName->toUTF8NonGCString();
        if (!FcPatternAddString(pattern, FC_FAMILY,
                                (const FcChar8*)u8FamilyName.data())) {
            return nullptr;
        }

        if (style == FontStyleItalic) {
            if (!FcPatternAddInteger(pattern, FC_SLANT, FC_SLANT_ITALIC)) {
                return nullptr;
            }
        } else if (style == FontStyleOblique) {
            if (!FcPatternAddInteger(pattern, FC_SLANT, FC_SLANT_OBLIQUE)) {
                return nullptr;
            }
        } else {
            if (!FcPatternAddInteger(pattern, FC_SLANT, FC_SLANT_ROMAN)) {
                return nullptr;
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
            return nullptr;
        }
        if (!FcPatternAddDouble(pattern, FC_PIXEL_SIZE, int(size + 0.5f))) {
            return nullptr;
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
        FcConfigSubstitute(m_fcconfig, pattern, FcMatchPattern);
        FcDefaultSubstitute(pattern);

        FcChar8* fontNameAfterMatch;
        FcPatternGetString(pattern, FC_FAMILY, 0, &fontNameAfterMatch);
        String* after = String::fromUTF8((char*)fontNameAfterMatch);

        FcResult fontConfigResult;
        FcPattern* resultPattern =
            FcFontMatch(m_fcconfig, pattern, &fontConfigResult);
        if (!resultPattern) {
            return nullptr;
        }

        if (!familyName->equalsIgnoreCase(STARFISH_DEFAULT_FONT_FAMILY)) {
            if (!after->equalsIgnoreCase(familyName)) {
                // if we got generic name, we can use this result although after
                // != familyName
                if (familyName->equalsIgnoreCase("sans")) {
                } else if (familyName->equalsIgnoreCase("sans-serif")) {
                } else if (familyName->equalsIgnoreCase("serif")) {
                } else if (familyName->equalsIgnoreCase("monospace")) {
                } else if (familyName->equalsIgnoreCase("fantasy")) {
                } else if (familyName->equalsIgnoreCase("cursive")) {
                } else {
                    return nullptr;
                }
            }
        }

        FcChar8* filePath = NULL;
        if (!FcPatternGetString(resultPattern, FC_FILE, 0, &filePath) ==
            FcResultMatch) {
            return nullptr;
        }
        std::string u8FilePath = (char*)filePath;

        FcPatternDestroy(resultPattern);
        FcPatternDestroy(pattern);
        return loadFont(u8FilePath, size, style, weight);
    }

    FontFaceImplCAIRO* loadFont(const std::string& u8FilePath, float size,
                                char style, char weight)
    {
        FT_Error error;
        FT_Face face;

        if (m_loadedFonts.find(u8FilePath) != m_loadedFonts.end()) {
            face = m_loadedFonts[u8FilePath];
        } else {
            error =
                FT_New_Face(m_FTFaceLib, (char*)u8FilePath.data(), 0, &face);
            m_glyphIndexCache[face] = std::unique_ptr<GlyphIndexCachePerFace>(
                new GlyphIndexCachePerFace);
            CHECK_ERROR;
        }

        int intSize = int(size + 0.5f);
        error = FT_Set_Pixel_Sizes(face, 0, intSize);
        CHECK_ERROR;
        FT_UInt glyph_index = FT_Get_Char_Index(face, 'x');
        error = FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_SCALE);
        CHECK_ERROR;
        FT_Int xheight =
            intSize * face->glyph->metrics.height / face->units_per_EM;

        FontMetrics met;
        met.m_fontHeight =
            ((face->ascender - face->descender) * intSize) / face->units_per_EM;
        met.m_ascender = ((face->ascender * intSize) / (face->units_per_EM));
        met.m_descender = met.m_ascender - met.m_fontHeight;
        met.m_xheightRate = xheight / (float)intSize;

#ifdef STARFISH_ENABLE_TEST
        if (g_enablePixelTest) {
            // Set the FontMetrics as if font is Ahem.
            met.m_ascender = size * 0.8;
            met.m_descender = met.m_ascender - size;
            met.m_fontHeight = met.m_ascender - met.m_descender;
            met.m_xheightRate = 0.8f;
        }
#endif

        FontFaceImplCAIRO* f =
            new FontFaceImplCAIRO(String::fromUTF8((char*)face->family_name),
                                  face, met, size, style, weight);
        return f;
    }

    bool loadGlyphFromGlyphIndexCachePerFace(
        FT_Face face, int intSize, char32_t ch,
        GlyphIndexCachePerFace* indexCache,
        std::pair<FT_Face, std::pair<unsigned, LayoutUnit>>& result)
    {
        auto iter = indexCache->find(ch);
        if (iter != indexCache->end()) {
            if (iter->second.first) {
                LayoutUnit width =
                    LayoutUnit((int)(iter->second.second * intSize)) /
                    LayoutUnit((int)(face->units_per_EM));
                result = std::make_pair(
                    face, std::make_pair(iter->second.first, width));
                return true;
            }
        } else {
            FT_UInt glyphIndex = FT_Get_Char_Index(face, ch);
            FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_SCALE);

            indexCache->insert(std::make_pair(
                ch,
                std::make_pair(glyphIndex, face->glyph->metrics.horiAdvance)));

            if (glyphIndex) {
                LayoutUnit width =
                    LayoutUnit(
                        (int)(face->glyph->metrics.horiAdvance * intSize)) /
                    LayoutUnit((int)(face->units_per_EM));
                result =
                    std::make_pair(face, std::make_pair(glyphIndex, width));
                return true;
            }
        }
        return false;
    }

    size_t lookFallbackFontCache(int blockCode, char style, char weight)
    {
        for (size_t i = 0; i < m_fallbackFontCachePerCodeBlock.size(); i++) {
            auto a = m_fallbackFontCachePerCodeBlock[i];
            if (std::get<0>(a) == blockCode && std::get<2>(a) == style &&
                std::get<3>(a) == weight) {
                return i;
            }
        }
        return SIZE_MAX;
    }

    std::pair<FT_Face, std::pair<unsigned, LayoutUnit>> loadGlyph(Font* f,
                                                                  char32_t ch)
    {
        std::pair<FT_Face, std::pair<unsigned, LayoutUnit>> result =
            std::make_pair(nullptr, std::make_pair(0, 0));

        int intSize = int(f->size() + .5f);

        auto& fontFaceList = f->m_fontFaceList;
        for (size_t i = 0; i < fontFaceList.size(); i++) {
            FontFaceImplCAIRO* impl = (FontFaceImplCAIRO*)fontFaceList[i];
            GlyphIndexCachePerFace* indexCache =
                m_glyphIndexCache[impl->m_face].get();
            if (loadGlyphFromGlyphIndexCachePerFace(impl->m_face, intSize, ch,
                                                    indexCache, result)) {
                return result;
            }
        }

        auto blockCode = ublock_getCode(ch);

        // search fallback

        size_t c = lookFallbackFontCache(blockCode, f->style(), f->weight());
        if (c != SIZE_MAX) {
            FT_Face face = std::get<1>(m_fallbackFontCachePerCodeBlock[c]);
            GlyphIndexCachePerFace* indexCache = m_glyphIndexCache[face].get();
            if (loadGlyphFromGlyphIndexCachePerFace(face, intSize, ch,
                                                    indexCache, result)) {
                return result;
            }
        }

        FcPattern* pattern = FcPatternCreate();

        char style = f->style();
        if (style == FontStyleItalic) {
            if (!FcPatternAddInteger(pattern, FC_SLANT, FC_SLANT_ITALIC)) {
                return result;
            }
        } else if (style == FontStyleOblique) {
            if (!FcPatternAddInteger(pattern, FC_SLANT, FC_SLANT_OBLIQUE)) {
                return result;
            }
        } else {
            if (!FcPatternAddInteger(pattern, FC_SLANT, FC_SLANT_ROMAN)) {
                return result;
            }
        }

        int fontWeight = FC_WEIGHT_MEDIUM;
        switch (f->weight()) {
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
            return result;
        }
        if (!FcPatternAddDouble(pattern, FC_PIXEL_SIZE,
                                int(f->size() + 0.5f))) {
            return result;
        }

        FcPatternAddBool(pattern, FC_SCALABLE, FcTrue);

        FcCharSet* fontConfigCharSet = FcCharSetCreate();
        FcCharSetAddChar(fontConfigCharSet, ch);

        FcPatternAddCharSet(pattern, FC_CHARSET, fontConfigCharSet);
        FcCharSetDestroy(fontConfigCharSet);

        FcResult fontConfigResult;
        FcPattern* resultPattern =
            FcFontMatch(m_fcconfig, pattern, &fontConfigResult);
        if (!resultPattern) {
            return result;
        }
        FcChar8* filePath = NULL;
        if (!FcPatternGetString(resultPattern, FC_FILE, 0, &filePath) ==
            FcResultMatch) {
            return result;
        }
        std::string u8FilePath = (char*)filePath;

        FcPatternDestroy(resultPattern);
        FcPatternDestroy(pattern);

        FT_Face fallbackFace;
        FT_Error error;
        if (m_loadedFonts.find(u8FilePath) != m_loadedFonts.end()) {
            fallbackFace = m_loadedFonts[u8FilePath];
        } else {
            error = FT_New_Face(m_FTFaceLib, (char*)u8FilePath.data(), 0,
                                &fallbackFace);
            m_glyphIndexCache[fallbackFace] =
                std::unique_ptr<GlyphIndexCachePerFace>(
                    new GlyphIndexCachePerFace);
            CHECK_ERROR;
        }

        if (c == SIZE_MAX) {
            m_fallbackFontCachePerCodeBlock.push_back(std::make_tuple(
                blockCode, fallbackFace, f->style(), f->weight()));
        }

        GlyphIndexCachePerFace* indexCache =
            m_glyphIndexCache[fallbackFace].get();

        loadGlyphFromGlyphIndexCachePerFace(fallbackFace, intSize, ch,
                                            indexCache, result);
        return result;
    }
};

LayoutUnit FontImplCAIRO::measureText(const StringView& str)
{
    if (str.length() == 0 || size() == 0) {
        return 0;
    }
#ifdef STARFISH_ENABLE_TEST
    if (g_enablePixelTest) {
        size_t count = 0;
        for (size_t i = str.start(); i < str.end(); i++) {
            count += Font::spaceSizeNumerator((*str.originalString())[i]);
        }
        return size() * ((float)count / SPACE_SIZE_DENOMINATOR);
    }
#endif
    LayoutUnit result;
    size_t length = str.length();
    auto accessData = str.bufferAccessData();
    int intSize = int(size() + .5f);
    for (size_t i = 0; i < length; i++) {
        char32_t ch = accessData.charAt(i);
        auto g = m_fontSelector->loadGlyph(this, ch);
        if (g.second.first) {
            result += g.second.second;
        } else {
            result += spaceWidth();
        }
    }
    return result;
}

std::pair<FT_Face, std::pair<unsigned, LayoutUnit>>
cairoBackendInternalloadGlyph(Font* f, char32_t ch)
{
    FontImplCAIRO* cairoF = (FontImplCAIRO*)f;
    return cairoF->m_fontSelector->loadGlyph(cairoF, ch);
}

#if !defined(PORT_CANVAS_BACKEND_EFL)
FontSelector* FontSelector::createFontSelector()
{
    return new FontSelectorImplCairo();
}

Font* Font::createEmptyFont(FontSelector* s)
{
    return new FontImplCAIRO((FontSelectorImplCairo*)s);
}

#else
FontSelector* FontSelector::createGenericFontSelector()
{
    return new FontSelectorImplCairo();
}

Font* Font::createGenericEmptyFont(FontSelector* s)
{
    return new FontImplCAIRO((FontSelectorImplCairo*)s);
}

#endif
}

#endif
