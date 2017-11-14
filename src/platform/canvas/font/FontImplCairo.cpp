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

FT_Library g_freeTypeInstance;

PlatformFontSelector* PlatformFontSelector::create(StarFish* sf)
{
    return new PlatformFontSelectorImplCairo();
}

PlatformFontCache* PlatformFontCache::create(StarFish* sf)
{
    return new PlatformFontCacheImplCairo();
}

FontFace* FontFace::create(const uint8_t* data, size_t dataLen)
{
    uint8_t* newBuf = (uint8_t*)malloc(dataLen);
    memcpy(newBuf, data, dataLen);
    FT_Face face;
    FT_Error error =
        FT_New_Memory_Face(g_freeTypeInstance, newBuf, dataLen, 0, &face);
    if (error) {
        delete newBuf;
        return nullptr;
    }
    FT_Set_Pixel_Sizes(face, 0, 16);
    auto hbFace = hb_ft_font_create(face, [](void* userData) {});
    return new FontFaceImplCairo(face, hbFace, newBuf, dataLen);
}

std::pair<std::pair<FontFaceImplCairo*, size_t>,
          std::pair<unsigned, LayoutUnit>>
FontImplCairo::loadGlyph(char32_t ch)
{
    std::pair<std::pair<FontFaceImplCairo*, size_t>,
              std::pair<unsigned, LayoutUnit>>
        result = std::make_pair(std::make_pair(nullptr, SIZE_MAX),
                                std::make_pair(0, 0));

    std::pair<FontFaceImplCairo*, std::pair<unsigned, LayoutUnit>> glyphResult =
        std::make_pair(nullptr, std::make_pair(0, 0));

    int intSize = int(size() + .5f);

    const auto& faceList = fontFaceList();
    for (size_t i = 0; i < faceList.size(); i++) {
        FontFaceImplCairo* impl = ((FontFaceImplCairo*)faceList[i]);

        if (impl->loadGlyph(intSize, ch, glyphResult)) {
            return std::make_pair(std::make_pair(impl, i), glyphResult.second);
        }
    }

    auto blockCode = ublock_getCode(ch);
    auto& fallbackFontFaceCachePerCodeBlock =
        ((PlatformFontCacheImplCairo*)(m_fontSelector->platformFontCache()))
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

    FontFaceImplCairo* face =
        (FontFaceImplCairo*)m_fontSelector->platformFontSelector()
            ->loadFontFace(u8FilePath);

    if (!hasCacheItem) {
        fallbackFontFaceCachePerCodeBlock.push_back(
            std::make_tuple(blockCode, face, fontStyle, fontWeight));
    }

    face->loadGlyph(intSize, ch, glyphResult);
    return std::make_pair(std::make_pair(face, SIZE_MAX), glyphResult.second);
}

std::vector<FontCairoTextRun> generateFontCairoTextRuns(const String* text,
                                                        FontImplCairo* font)
{
    std::vector<FontCairoTextRun> result;
    size_t length = text->length();
    auto accessData = text->bufferAccessData();
    UErrorCode errorCode = U_ZERO_ERROR;
    for (size_t i = 0; i < length;) {
        size_t pos = 0;
        size_t faceIndex = SIZE_MAX;
        FT_Face lastFace = nullptr;
        hb_font_t* hbFace = nullptr;
        UScriptCode lastUnicodeScript;
        bool failedToFindFont = false;
        while (i + pos < length) {
            size_t idx = i + pos;
            char32_t ch = accessData.charAt(idx);

            UScriptCode unicodeScript =
                uscript_getScript(accessData.charAt(idx), &errorCode);
            if (!U_SUCCESS(errorCode)) {
                return result;
            }

            std::pair<std::pair<FontFaceImplCairo*, size_t>,
                      std::pair<unsigned, LayoutUnit>>
                glyphData = font->loadGlyph(ch);

            if (pos == 0 && glyphData.first.first == nullptr) {
                failedToFindFont = true;
                break;
            }

            faceIndex = glyphData.first.second;

            if (pos == 0) {
                lastFace = glyphData.first.first->freetypeFace();
                hbFace = glyphData.first.first->harfbuzzFace();
                lastUnicodeScript = unicodeScript;
            } else {
                if (glyphData.first.first ||
                    lastFace != glyphData.first.first->freetypeFace() ||
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
        FontCairoTextRun run;
        run.m_script = hb_icu_script_to_script(lastUnicodeScript);
        run.m_faceIndex = faceIndex;
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

    hb_buffer_destroy(hbBuffer);

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
            auto g = loadGlyph(ch);
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

std::pair<std::pair<FontFaceImplCairo*, size_t>,
          std::pair<unsigned, LayoutUnit>>
cairoBackendInternalLoadGlyph(Font* f, char32_t ch)
{
    FontImplCairo* cairoF = (FontImplCairo*)f;
    return cairoF->loadGlyph(ch);
}

bool cairoBackendCanUseSimpleFontPath(Font* f, const StringView& sv)
{
    if (sv.length() == 1) {
        return true;
    }
    return false;
}

#if !defined(PORT_CANVAS_BACKEND_EFL)
FontSelector* FontSelector::create(Document* document,
                                   PlatformFontSelector* platformFontSelector,
                                   PlatformFontCache* platformFontCache)
{
    return new FontSelectorImplCairo(document, platformFontSelector,
                                     platformFontCache);
}

Font* Font::createEmptyFont(FontSelector* s)
{
    return new FontImplCairo((FontSelectorImplCairo*)s);
}

#else
FontSelector* FontSelector::createGenericFontSelector(
    Document* document, PlatformFontSelector* platformFontSelector,
    PlatformFontCache* platformFontCache)
{
    return new FontSelectorImplCairo(document, platformFontSelector,
                                     platformFontCache);
}

Font* Font::createGenericEmptyFont(FontSelector* s)
{
    return new FontImplCairo((FontSelectorImplCairo*)s);
}

#endif
}

#endif
