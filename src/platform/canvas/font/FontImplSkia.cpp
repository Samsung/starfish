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
#if defined(PORT_CANVAS_BACKEND_SKIA)

#include "StarFish.h"

#include "SkTypeface.h"
#include "SkStream.h"
#include "SkFontMgr.h"
#include "SkPaint.h"

#include <fontconfig/fontconfig.h>
#include <hb.h>
#include <hb-ft.h>
#include <hb-icu.h>
#include "core/modules/canvas/font/Font.h"
#include "FontImplSkia.h"
#include "core/style/UnitHelper.h"

#define CHECK_ERROR                            \
    if (error) {                               \
        STARFISH_RELEASE_ASSERT_NOT_REACHED(); \
    }

namespace StarFish {

FT_Library g_freeTypeInstance;

PlatformFontSelector* PlatformFontSelector::create(StarFish* sf)
{
    return new PlatformFontSelectorImplSkia(sf);
}

PlatformFontCache* PlatformFontCache::create(StarFish* sf)
{
    return new PlatformFontCacheImplSkia();
}

FontFace* FontFace::create(const uint8_t* data, size_t dataLen)
{
    uint8_t* newBuf = new uint8_t[dataLen];
    memcpy(newBuf, data, dataLen);
    FT_Face face;
    FT_Error error =
        FT_New_Memory_Face(g_freeTypeInstance, newBuf, dataLen, 0, &face);
    if (error) {
        delete[] newBuf;
        return nullptr;
    }

    sk_sp<SkFontMgr> fm(SkFontMgr::RefDefault());
    sk_sp<SkData> skData = SkData::MakeWithCopy(newBuf, dataLen);
    sk_sp<SkTypeface> skTypeface(fm->createFromData(skData.get()));

    FT_Set_Pixel_Sizes(face, 0, 16);
    auto hbFace = hb_ft_font_create(face, [](void* userData) {});
    return new (PointerFreeGC)
        FontFaceImplSkia(face, skTypeface, hbFace, newBuf, dataLen);
}

FontSelector* FontSelector::create(Document* document,
                                   PlatformFontSelector* platformFontSelector,
                                   PlatformFontCache* platformFontCache)
{
    return new FontSelectorImplSkia(document, platformFontSelector,
                                    platformFontCache);
}

Font* Font::createEmptyFont(FontSelector* s)
{
    return new FontImplSkia((FontSelectorImplSkia*)s);
}

FontFaceImplSkia::FontFaceImplSkia(FT_Face face, sk_sp<SkTypeface> skTypeface,
                                   hb_font_t* hbFace, uint8_t* dataBuffer,
                                   size_t dataBufferSize)
{
    m_dataBuffer = dataBuffer;
    m_dataBufferSize = dataBufferSize;
    m_face = face;
    m_hbFace = hbFace;

    m_skTypeFace = skTypeface;
    m_skPaint = new SkPaint();
    m_skPaint->setTypeface(m_skTypeFace);
    m_skPaint->setAntiAlias(true);
    m_skPaint->setTextEncoding(SkPaint::kGlyphID_TextEncoding);

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
            FontFaceImplSkia* m = (FontFaceImplSkia*)obj;
            if (m->m_hbFace) {
                hb_font_destroy(m->m_hbFace);
                FT_Done_Face(m->m_face);
                m->m_skTypeFace = nullptr;
                delete m->m_skPaint;
                m->m_skPaint = nullptr;
            }
            GlyphIndexCache().swap(m->m_glyphIndexCache);
            free(m->m_dataBuffer);
        },
        NULL, NULL, NULL);

    clearCache();
}

FontMetrics FontFaceImplSkia::metrics(float size)
{
    int intSize = int(size + 0.5f);

    FontMetrics met;
    met.m_fontHeight = ((m_ascender - m_descender) * intSize) / m_unitsPerEM;
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

void FontFaceImplSkia::clearCache()
{
    if (m_face && m_dataBuffer) {
        hb_font_destroy(m_hbFace);
        FT_Done_Face(m_face);
        m_face = nullptr;
        m_hbFace = nullptr;
        m_skTypeFace = nullptr;
        delete m_skPaint;
        m_skPaint = nullptr;
        GlyphIndexCache().swap(m_glyphIndexCache);
    }
}

bool FontFaceImplSkia::loadGlyph(
    int intSize, char32_t ch,
    std::pair<FontFaceImplSkia*, std::pair<unsigned, LayoutUnit>>& result)
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
            result =
                std::make_pair(this, std::make_pair(iter->second.first, width));
            return true;
        }
    } else {
        FT_UInt glyphIndex = FT_Get_Char_Index(face, ch);
        FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_SCALE);

        m_glyphIndexCache.insert(std::make_pair(
            ch, std::make_pair(glyphIndex, face->glyph->metrics.horiAdvance)));

        if (glyphIndex) {
            LayoutUnit width =
                LayoutUnit((int)(face->glyph->metrics.horiAdvance * intSize)) /
                LayoutUnit((int)(m_unitsPerEM));
            result = std::make_pair(this, std::make_pair(glyphIndex, width));
            return true;
        }
    }
    return false;
}

void FontFaceImplSkia::ensureFonts()
{
    if (m_face == nullptr) {
        FT_Error error = FT_New_Memory_Face(g_freeTypeInstance, m_dataBuffer,
                                            m_dataBufferSize, 0, &m_face);
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

    if (m_skTypeFace == nullptr) {
        sk_sp<SkFontMgr> fm(SkFontMgr::RefDefault());
        sk_sp<SkData> skData =
            SkData::MakeWithCopy(m_dataBuffer, m_dataBufferSize);
        sk_sp<SkTypeface> skTypeface(fm->createFromData(skData.get()));

        m_skTypeFace = skTypeface;
    }
    if (m_skPaint == nullptr) {
        m_skPaint = new SkPaint();
        m_skPaint->setTypeface(m_skTypeFace);
        m_skPaint->setAntiAlias(true);
        m_skPaint->setTextEncoding(SkPaint::kGlyphID_TextEncoding);
    }
}

std::vector<FontSkiaTextRun> generateFontSkiaTextRuns(const String* text,
                                                      FontImplSkia* font)
{
    std::vector<FontSkiaTextRun> result;
    size_t length = text->length();
    auto accessData = text->bufferAccessData();
    UErrorCode errorCode = U_ZERO_ERROR;
    for (size_t i = 0; i < length;) {
        size_t pos = 0;
        size_t faceIndex = SIZE_MAX;
        FT_Face lastFace = nullptr;
        sk_sp<SkTypeface> lastSkFontFace = nullptr;
        SkPaint* lastSkPaint = nullptr;

        hb_font_t* hbFace = nullptr;
        UScriptCode lastUnicodeScript = USCRIPT_COMMON;
        bool failedToFindFont = false;
        while (i + pos < length) {
            size_t idx = i + pos;
            char32_t ch = accessData.charAt(idx);

            UScriptCode unicodeScript =
                uscript_getScript(accessData.charAt(idx), &errorCode);
            if (!U_SUCCESS(errorCode)) {
                return result;
            }

            std::pair<std::pair<FontFaceImplSkia*, size_t>,
                      std::pair<unsigned, LayoutUnit>>
                glyphData = font->loadGlyph(ch);

            if (pos == 0 && glyphData.first.first == nullptr) {
                failedToFindFont = true;
                break;
            }

            faceIndex = glyphData.first.second;

            if (pos == 0) {
                lastFace = glyphData.first.first->freetypeFace();
                lastSkFontFace = glyphData.first.first->skTypeFace();
                lastSkPaint = glyphData.first.first->skPaint();
                hbFace = glyphData.first.first->harfbuzzFace();
                lastUnicodeScript = unicodeScript;
            } else {
                if (glyphData.first.first ||
                    lastSkFontFace != glyphData.first.first->skTypeFace() ||
                    lastUnicodeScript != unicodeScript ||
                    ((unicodeScript != USCRIPT_INHERITED) &&
                     (!uscript_hasScript(ch, lastUnicodeScript)))) {
                    break;
                }
            }
            pos++;
        }

        if (failedToFindFont) {
            i++;
            continue;
        }

        size_t startPos = i, endPos = i + pos;

        i = i + pos;
        FontSkiaTextRun run;
        run.m_script = hb_icu_script_to_script(lastUnicodeScript);
        run.m_faceIndex = faceIndex;
        run.m_ftFace = lastFace;
        run.m_skTypeFace = lastSkFontFace;
        run.m_skPaint = lastSkPaint;
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
        FontSkiaTextRun& run = result[i];
        float totalAdvance = 0;

        hb_buffer_set_script(hbBuffer, run.m_script);
        hb_buffer_guess_segment_properties(hbBuffer);
        // hb_buffer_set_direction(hbBuffer, HB_DIRECTION_LTR);
        auto buf = run.m_text.bufferAccessData();
        if (buf.bufferDataKind == StringBufferAccessData::ASCIIData) {
            hb_buffer_add_utf8(hbBuffer, buf.asciiData(), buf.length, 0,
                               buf.length);
        } else if (buf.bufferDataKind == StringBufferAccessData::BMPData) {
            hb_buffer_add_utf16(hbBuffer, (const uint16_t*)buf.utf16Data(),
                                buf.length, 0, buf.length);
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

    hb_buffer_destroy(hbBuffer);

    return result;
}

LayoutUnit FontImplSkia::measureText(const StringView& str)
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
    bool isSimpleCase = skiaBackendCanUseSimpleFontPath(this, str);

    if (isSimpleCase) {
        size_t length = str.length();
        auto accessData = str.bufferAccessData();

        for (size_t i = 0; i < length; i++) {
            char32_t ch = accessData.charAt(i);
            auto g = loadGlyph(ch);
            if (g.second.first) {
                result += g.second.second;
            } else {
                result += spaceWidth();
            }
        }
        result += m_letterSpacing * str.length();
        return result;
    }

    auto runs = generateFontSkiaTextRuns(&str, this);
    for (size_t i = 0; i < runs.size(); i++) {
        FontSkiaTextRun& run = runs[i];
        result += run.m_runWidth;
    }

    result += m_letterSpacing * str.length();

    return result;
}

std::pair<std::pair<FontFaceImplSkia*, size_t>, std::pair<unsigned, LayoutUnit>>
FontImplSkia::loadGlyph(char32_t ch)
{
    std::pair<std::pair<FontFaceImplSkia*, size_t>,
              std::pair<unsigned, LayoutUnit>>
        result = std::make_pair(std::make_pair(nullptr, SIZE_MAX),
                                std::make_pair(0, 0));

    std::pair<FontFaceImplSkia*, std::pair<unsigned, LayoutUnit>> glyphResult =
        std::make_pair(nullptr, std::make_pair(0, 0));

    int intSize = int(size() + .5f);

    const auto& faceList = fontFaceList();
    for (size_t i = 0; i < faceList.size(); i++) {
        FontFaceImplSkia* impl = ((FontFaceImplSkia*)faceList[i]);

        if (impl->loadGlyph(intSize, ch, glyphResult)) {
            return std::make_pair(std::make_pair(impl, i), glyphResult.second);
        }
    }

    auto blockCode = ublock_getCode(ch);
    auto& fallbackFontFaceCachePerCodeBlock =
        ((PlatformFontCacheImplSkia*)(m_fontSelector->platformFontCache()))
            ->m_fallbackFontFaceCachePerCodeBlock;

    bool hasCacheItem = false;
    char fontStyle = style();
    char fontWeight = weight();
    for (size_t i = 0; i < fallbackFontFaceCachePerCodeBlock.size(); i++) {
        auto a = fallbackFontFaceCachePerCodeBlock[i];
        if (std::get<0>(a) == blockCode && std::get<2>(a) == fontStyle &&
            std::get<3>(a) == fontWeight) {
            hasCacheItem = true;
            auto face = std::get<1>(fallbackFontFaceCachePerCodeBlock[i]);
            if (face->loadGlyph(intSize, ch, glyphResult)) {
                return std::make_pair(std::make_pair(face, SIZE_MAX),
                                      glyphResult.second);
            }
            break;
        }
    }

    // finding fallback font
    FcPattern* pattern = FcPatternCreate();

    if (fontStyle == FontStyleItalic) {
        if (!FcPatternAddInteger(pattern, FC_SLANT, FC_SLANT_ITALIC)) {
            return result;
        }
    } else if (fontStyle == FontStyleOblique) {
        if (!FcPatternAddInteger(pattern, FC_SLANT, FC_SLANT_OBLIQUE)) {
            return result;
        }
    } else {
        if (!FcPatternAddInteger(pattern, FC_SLANT, FC_SLANT_ROMAN)) {
            return result;
        }
    }

    int fcFontWeight = FC_WEIGHT_MEDIUM;
    switch (fontWeight) {
    case 1:
        fcFontWeight = FC_WEIGHT_THIN;
        break;
    case 2:
        fcFontWeight = FC_WEIGHT_ULTRALIGHT;
        break;
    case 3:
        fcFontWeight = FC_WEIGHT_LIGHT;
        break;
    case 4:
        fcFontWeight = FC_WEIGHT_REGULAR;
        break;
    case 5:
        fcFontWeight = FC_WEIGHT_MEDIUM;
        break;
    case 6:
        fcFontWeight = FC_WEIGHT_SEMIBOLD;
        break;
    case 7:
        fcFontWeight = FC_WEIGHT_BOLD;
        break;
    case 8:
        fcFontWeight = FC_WEIGHT_ULTRABOLD;
        break;
    case 9:
        fcFontWeight = FC_WEIGHT_ULTRABLACK;
        break;
    default:
        STARFISH_ASSERT_NOT_REACHED();
    }

    if (!FcPatternAddInteger(pattern, FC_WEIGHT, fcFontWeight)) {
        return result;
    }

    FcPatternAddBool(pattern, FC_SCALABLE, FcTrue);

    FcCharSet* fontConfigCharSet = FcCharSetCreate();
    FcCharSetAddChar(fontConfigCharSet, ch);

    FcPatternAddCharSet(pattern, FC_CHARSET, fontConfigCharSet);
    FcCharSetDestroy(fontConfigCharSet);

    FcResult fontConfigResult;

    FcPattern* resultPattern = FcFontMatch(NULL, pattern, &fontConfigResult);
    if (!resultPattern) {
        FcCharSetDestroy(fontConfigCharSet);
        return result;
    }
    FcChar8* filePath = NULL;
    if (!(FcPatternGetString(resultPattern, FC_FILE, 0, &filePath) ==
          FcResultMatch)) {
        return result;
    }

    std::string u8FilePath = (char*)filePath;

    FcPatternDestroy(resultPattern);
    FcPatternDestroy(pattern);

    FontFaceImplSkia* face =
        (FontFaceImplSkia*)m_fontSelector->platformFontSelector()->loadFontFace(
            u8FilePath);

    if (!hasCacheItem) {
        fallbackFontFaceCachePerCodeBlock.push_back(
            std::make_tuple(blockCode, face, fontStyle, fontWeight));
    }

    face->loadGlyph(intSize, ch, glyphResult);
    return std::make_pair(std::make_pair(face, SIZE_MAX), glyphResult.second);
}

std::pair<std::pair<FontFaceImplSkia*, size_t>, std::pair<unsigned, LayoutUnit>>
skiaBackendInternalLoadGlyph(Font* f, char32_t ch)
{
    FontImplSkia* SkiaF = (FontImplSkia*)f;
    return SkiaF->loadGlyph(ch);
}

bool skiaBackendCanUseSimpleFontPath(Font* f, const StringView& sv)
{
    if (sv.length() == 1) {
        return true;
    }

    size_t length = sv.length();
    auto accessData = sv.bufferAccessData();
    for (size_t i = 0; i < length; i++) {
        char32_t ch = accessData.charAt(i);

        auto property = u_getIntPropertyValue(ch, UCHAR_BIDI_CLASS);
        if ((property == U_RIGHT_TO_LEFT) ||
            (property == U_RIGHT_TO_LEFT_ARABIC) ||
            (property == U_RIGHT_TO_LEFT_EMBEDDING) ||
            (property == U_RIGHT_TO_LEFT_OVERRIDE)) {
            return false;
        }
    }

    if (f->fontKerning() == FontKerningAutoValue) {
        if (((FontFaceImplSkia*)(FontImplSkia*)f->fontFaceList()[0])
                ->m_supportsKerning) {
            if (f->size() >= 48) {
                return false;
            }
        }
        return true;
    } else if (f->fontKerning() == FontKerningNormalValue) {
        return ((FontFaceImplSkia*)(FontImplSkia*)f->fontFaceList()[0])
            ->m_supportsKerning;
    } else {
        STARFISH_ASSERT(f->fontKerning() == FontKerningNoneValue);
        return true;
    }
}

PlatformFontSelectorImplSkia::PlatformFontSelectorImplSkia(StarFish* sf)
    : PlatformFontSelector(sf)
{
    if (!g_freeTypeInstance) {
        FT_Error error;
        error = FT_Init_FreeType(&g_freeTypeInstance);
        CHECK_ERROR;
    }
}

UTF8StringDataNonGCStd PlatformFontSelectorImplSkia::findFont(
    const UTF8StringDataNonGCStd& familyName, bool isGenericName, char style,
    char weight)
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
    FcPattern* resultPattern = FcFontMatch(NULL, pattern, &fontConfigResult);
    if (!resultPattern) {
        FcPatternDestroy(pattern);
        return UTF8StringDataNonGCStd();
    }

    FcChar8* fontNameAfterMatch;
    FcPatternGetString(resultPattern, FC_FAMILY, 0, &fontNameAfterMatch);
    UTF8StringDataNonGCStd after = (char*)fontNameAfterMatch;
    std::transform(after.begin(), after.end(), after.begin(), tolower);

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
    if (!(FcPatternGetString(resultPattern, FC_FILE, 0, &filePath) ==
          FcResultMatch)) {
        return UTF8StringDataNonGCStd();
    }
    std::string u8FilePath = (char*)filePath;

    FcPatternDestroy(resultPattern);
    FcPatternDestroy(pattern);

    return u8FilePath;
}

FontFace* PlatformFontSelectorImplSkia::loadFontFace(
    const UTF8StringDataNonGCStd& path)
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

    auto skTypeface = SkTypeface::MakeFromFile((char*)path.data(), 0);

    auto impl = new (PointerFreeGC) FontFaceImplSkia(face, skTypeface, hbFace);
    m_fontPathToFace.insert(std::make_pair(path, impl));
    return impl;
}
}

#endif
