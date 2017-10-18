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
#include <hb.h>
#include <hb-ft.h>
#include <hb-icu.h>
#include "core/modules/canvas/font/Font.h"
#include "FontImplCairo.h"
#include "core/style/UnitHelper.h"

namespace StarFish {

#define CHECK_ERROR                            \
    if (error) {                               \
        STARFISH_RELEASE_ASSERT_NOT_REACHED(); \
    }

class FontSelectorImplCairo;

class FontSelectorImplCairo : public FontSelector {
public:
    FcConfig* m_fcconfig;
    FT_Library m_FTFaceLib;
    std::unordered_map<std::string, std::pair<FT_Face, hb_font_t*>>
        m_loadedFonts;

    typedef std::unordered_map<char32_t, std::pair<unsigned, unsigned>>
        GlyphIndexCachePerFace;
    std::unordered_map<FT_Face, std::unique_ptr<GlyphIndexCachePerFace>>
        m_glyphIndexCache;
    std::vector<std::tuple<int, FT_Face, hb_font_t*, char, char>>
        m_fallbackFontCachePerCodeBlock;

    hb_buffer_t* m_hbBuffer;

    FontSelectorImplCairo()
    {
        m_fcconfig = FcInitLoadConfigAndFonts();
        FT_Error error;
        error = FT_Init_FreeType(&m_FTFaceLib);
        CHECK_ERROR;

        m_hbBuffer = hb_buffer_create();
        hb_buffer_set_unicode_funcs(m_hbBuffer, hb_icu_get_unicode_funcs());
    }

    ~FontSelectorImplCairo()
    {
        FcConfigDestroy(m_fcconfig);
        FT_Done_FreeType(m_FTFaceLib);
        hb_buffer_destroy(m_hbBuffer);
    }

    FontFace* loadFontImpl(String* familyName, float size, char style,
                           char weight)
    {
        // http://www.w3.org/TR/css3-fonts/#font-matching-algorithm
        FcPattern* pattern = FcPatternCreate();

        auto u8FamilyName = familyName->toUTF8NonGCString();
        if (!FcPatternAddString(pattern, FC_FAMILY,
                                (const FcChar8*)u8FamilyName.data())) {
            FcPatternDestroy(pattern);
            return nullptr;
        }

        if (style == FontStyleItalic) {
            if (!FcPatternAddInteger(pattern, FC_SLANT, FC_SLANT_ITALIC)) {
                FcPatternDestroy(pattern);
                return nullptr;
            }
        } else if (style == FontStyleOblique) {
            if (!FcPatternAddInteger(pattern, FC_SLANT, FC_SLANT_OBLIQUE)) {
                FcPatternDestroy(pattern);
                return nullptr;
            }
        } else {
            if (!FcPatternAddInteger(pattern, FC_SLANT, FC_SLANT_ROMAN)) {
                FcPatternDestroy(pattern);
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
            FcPatternDestroy(pattern);
            return nullptr;
        }
        if (!FcPatternAddDouble(pattern, FC_PIXEL_SIZE, int(size + 0.5f))) {
            FcPatternDestroy(pattern);
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
            FcPatternDestroy(pattern);
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

    std::pair<FT_Face, hb_font_t*> loadFontFace(const std::string& u8FilePath)
    {
        if (m_loadedFonts.find(u8FilePath) != m_loadedFonts.end()) {
            return m_loadedFonts[u8FilePath];
        } else {
            FT_Face face;
            FT_Error error;
            error =
                FT_New_Face(m_FTFaceLib, (char*)u8FilePath.data(), 0, &face);
            CHECK_ERROR;
            FT_Set_Pixel_Sizes(face, 0, 16);
            auto hbFace = hb_ft_font_create(face, [](void* userData) {});
            m_loadedFonts.insert(
                std::make_pair(u8FilePath, std::make_pair(face, hbFace)));
            m_glyphIndexCache[face] = std::unique_ptr<GlyphIndexCachePerFace>(
                new GlyphIndexCachePerFace);
            return std::make_pair(face, hbFace);
        }
    }

    FontFaceImplCairo* loadFont(const std::string& u8FilePath, float size,
                                char style, char weight)
    {
        FT_Error error;
        auto ff = loadFontFace(u8FilePath);
        FT_Face face = ff.first;

        int intSize = int(size + 0.5f);
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

        FontFaceImplCairo* f =
            new FontFaceImplCairo(String::fromUTF8((char*)face->family_name),
                                  face, ff.second, met, size, style, weight);
        return f;
    }

    bool loadGlyphFromGlyphIndexCachePerFace(
        FT_Face face, hb_font_t* hbFace, int intSize, char32_t ch,
        GlyphIndexCachePerFace* indexCache,
        std::pair<std::pair<FT_Face, hb_font_t*>,
                  std::pair<unsigned, LayoutUnit>>& result)
    {
        auto iter = indexCache->find(ch);
        if (iter != indexCache->end()) {
            if (iter->second.first) {
                LayoutUnit width =
                    LayoutUnit((int)(iter->second.second * intSize)) /
                    LayoutUnit((int)(face->units_per_EM));
                result =
                    std::make_pair(std::make_pair(face, hbFace),
                                   std::make_pair(iter->second.first, width));
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
                result = std::make_pair(std::make_pair(face, hbFace),
                                        std::make_pair(glyphIndex, width));
                return true;
            }
        }
        return false;
    }

    size_t lookFallbackFontCache(int blockCode, char style, char weight)
    {
        for (size_t i = 0; i < m_fallbackFontCachePerCodeBlock.size(); i++) {
            auto a = m_fallbackFontCachePerCodeBlock[i];
            if (std::get<0>(a) == blockCode && std::get<3>(a) == style &&
                std::get<4>(a) == weight) {
                return i;
            }
        }
        return SIZE_MAX;
    }

    std::pair<std::pair<FT_Face, hb_font_t*>, std::pair<unsigned, LayoutUnit>>
    loadGlyph(Font* f, char32_t ch)
    {
        std::pair<std::pair<FT_Face, hb_font_t*>,
                  std::pair<unsigned, LayoutUnit>>
            result = std::make_pair(std::make_pair(nullptr, nullptr),
                                    std::make_pair(0, 0));

        int intSize = int(f->size() + .5f);

        auto& fontFaceList = f->m_fontFaceList;
        for (size_t i = 0; i < fontFaceList.size(); i++) {
            FontFaceImplCairo* impl = (FontFaceImplCairo*)fontFaceList[i];
            GlyphIndexCachePerFace* indexCache =
                m_glyphIndexCache[impl->m_face].get();
            if (loadGlyphFromGlyphIndexCachePerFace(impl->m_face,
                                                    impl->m_hbFace, intSize, ch,
                                                    indexCache, result)) {
                return result;
            }
        }

        auto blockCode = ublock_getCode(ch);

        // search fallback

        size_t c = lookFallbackFontCache(blockCode, f->style(), f->weight());
        if (c != SIZE_MAX) {
            FT_Face face = std::get<1>(m_fallbackFontCachePerCodeBlock[c]);
            auto hbFace = std::get<2>(m_fallbackFontCachePerCodeBlock[c]);
            GlyphIndexCachePerFace* indexCache = m_glyphIndexCache[face].get();
            if (loadGlyphFromGlyphIndexCachePerFace(face, hbFace, intSize, ch,
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

        auto fallbackFace = loadFontFace(u8FilePath);

        if (c == SIZE_MAX) {
            m_fallbackFontCachePerCodeBlock.push_back(
                std::make_tuple(blockCode, fallbackFace.first,
                                fallbackFace.second, f->style(), f->weight()));
        }

        GlyphIndexCachePerFace* indexCache =
            m_glyphIndexCache[fallbackFace.first].get();

        loadGlyphFromGlyphIndexCachePerFace(fallbackFace.first,
                                            fallbackFace.second, intSize, ch,
                                            indexCache, result);
        return result;
    }
};

std::vector<FontCairoTextRun> generateFontCairoTextRuns(const String* text,
                                                        FontImplCairo* font)
{
    std::vector<FontCairoTextRun> result;
    size_t length = text->length();
    auto accessData = text->bufferAccessData();
    UErrorCode errorCode = U_ZERO_ERROR;
    for (size_t i = 0; i < length;) {
        size_t pos = 0;
        FT_Face lastFace = nullptr;
        hb_font_t* hbFace = nullptr;
        UScriptCode lastUnicodeScript;
        while (i + pos < length) {
            size_t idx = i + pos;
            char32_t ch = accessData.charAt(idx);
            if (!U_SUCCESS(errorCode)) {
                return result;
            }
            std::pair<std::pair<FT_Face, hb_font_t*>,
                      std::pair<unsigned, LayoutUnit>>
                glyphData = font->m_fontSelector->loadGlyph(font, ch);
            UScriptCode unicodeScript =
                uscript_getScript(accessData.charAt(idx), &errorCode);

            if (pos == 0) {
                lastFace = glyphData.first.first;
                hbFace = glyphData.first.second;
                lastUnicodeScript = unicodeScript;
            } else {
                if (lastFace != glyphData.first.first ||
                    lastUnicodeScript != unicodeScript ||
                    ((unicodeScript != USCRIPT_INHERITED) &&
                     (!uscript_hasScript(ch, lastUnicodeScript)))) {
                    break;
                }
            }
            pos++;
        }
        size_t startPos = i, endPos = i + pos;

        i = i + pos;
        FontCairoTextRun run;
        run.m_script = hb_icu_script_to_script(lastUnicodeScript);
        run.m_ftFace = lastFace;
        run.m_hbFont = hbFace;
        run.m_text = StringView((String*)text, startPos, endPos);
        result.push_back(run);
    }

    hb_buffer_t* hbBuffer = hb_buffer_create();
    hb_buffer_set_unicode_funcs(hbBuffer, hb_icu_get_unicode_funcs());

    const hb_tag_t kernTag = HB_TAG('k', 'e', 'r', 'n');
    hb_feature_t hbFeature = { kernTag, 0, 0, static_cast<unsigned>(-1) };

    int intSize(font->size() + .5f);

    for (size_t i = 0; i < result.size(); i++) {
        FontCairoTextRun& run = result[i];
        float totalAdvance = 0;

        hb_buffer_set_script(hbBuffer, run.m_script);
        hb_buffer_guess_segment_properties(hbBuffer);
        // hb_buffer_set_direction(hbBuffer, HB_DIRECTION_LTR);
        auto buf = run.m_text.bufferAccessData();
        if (buf.hasASCIIContent) {
            hb_buffer_add_utf8(hbBuffer, buf.asciiData(), buf.length, 0,
                               buf.length);
        } else {
            hb_buffer_add_utf32(hbBuffer, (const uint32_t*)buf.utf32Data(),
                                buf.length, 0, buf.length);
        }

        if (run.m_ftFace) {
            int ftSize = run.m_ftFace->size->metrics.y_ppem;
            hb_font_t* hbfont = run.m_hbFont;

            hb_shape(hbfont, hbBuffer, &hbFeature, 1);

            hb_buffer_content_type_t t = hb_buffer_get_content_type(hbBuffer);
            hb_glyph_info_t* glyphInfos =
                hb_buffer_get_glyph_infos(hbBuffer, 0);
            hb_glyph_position_t* glyphPositions =
                hb_buffer_get_glyph_positions(hbBuffer, 0);
            size_t glyphCount = hb_buffer_get_length(hbBuffer);

            run.m_glyphs.reserve(glyphCount);
            run.m_glyphPositions.reserve(glyphCount);

            for (size_t k = 0; k < glyphCount; k++) {
                uint16_t glyph = glyphInfos[k].codepoint;
                float advance = glyphPositions[k].x_advance / 64.f *
                                (float)intSize / (float)ftSize;
                float xOffset = glyphPositions[k].x_offset / 64.f *
                                (float)intSize / (float)ftSize;
                float yOffset = glyphPositions[k].y_offset / 64.f *
                                (float)intSize / (float)ftSize;

                run.m_glyphs.push_back(glyph);
                run.m_glyphPositions.push_back(
                    LayoutLocation(xOffset + totalAdvance, yOffset));

                totalAdvance += advance;
            }
        } else {
            totalAdvance += font->spaceWidth() * run.m_text.length();
        }

        run.m_runWidth = totalAdvance;
        hb_buffer_reset(hbBuffer);
    }

    return result;
}

LayoutUnit FontImplCairo::measureText(const StringView& str)
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

    bool isSimpleCase = cairoBackendCanUseSimpleFontPath(this, str);

    if (isSimpleCase) {
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

    auto runs = generateFontCairoTextRuns(&str, this);
    for (size_t i = 0; i < runs.size(); i++) {
        FontCairoTextRun& run = runs[i];
        result += run.m_runWidth;
    }

    return result;
}

std::pair<std::pair<FT_Face, hb_font_t*>, std::pair<unsigned, LayoutUnit>>
cairoBackendInternalLoadGlyph(Font* f, char32_t ch)
{
    FontImplCairo* cairoF = (FontImplCairo*)f;
    return cairoF->m_fontSelector->loadGlyph(cairoF, ch);
}

bool cairoBackendCanUseSimpleFontPath(Font* f, const StringView& sv)
{
    if (sv.length() == 1) {
        return true;
    }
    return false;
}

#if !defined(PORT_CANVAS_BACKEND_EFL)
FontSelector* FontSelector::createFontSelector()
{
    return new FontSelectorImplCairo();
}

Font* Font::createEmptyFont(FontSelector* s)
{
    return new FontImplCairo((FontSelectorImplCairo*)s);
}

#else
FontSelector* FontSelector::createGenericFontSelector()
{
    return new FontSelectorImplCairo();
}

Font* Font::createGenericEmptyFont(FontSelector* s)
{
    return new FontImplCairo((FontSelectorImplCairo*)s);
}

#endif
}

#endif
