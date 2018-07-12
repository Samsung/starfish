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

#include <hb.h>
#include <hb-ot.h>
#include <hb-icu.h>
#include "core/modules/canvas/font/Font.h"
#include "FontImplSkia.h"
#include "core/style/UnitHelper.h"

static const int FONT_SIZE_SCALE = 64;

namespace StarFish {

static inline hb_font_t* createHarfbuzzFont(sk_sp<SkData> skData,
                                            sk_sp<SkTypeface> skTypeface)
{
    hb_blob_t* blob = hb_blob_create(
        (const char*)skData->data(), (unsigned int)skData->size(),
        HB_MEMORY_MODE_READONLY, nullptr, [](void* d) {});
    hb_blob_make_immutable(blob);
    int index = 0;
    hb_face_t* hf = hb_face_create(blob, index);
    hb_blob_destroy(blob);
    hb_face_set_index(hf, index);
    hb_face_set_upem(hf, skTypeface->getUnitsPerEm());

    auto hbFont = hb_font_create(hf);
    hb_face_destroy(hf);
    hb_font_set_scale(hbFont, FONT_SIZE_SCALE, FONT_SIZE_SCALE);
    hb_ot_font_set_funcs(hbFont);
    return hbFont;
}

static inline SkFontStyle::Slant styleToSkFontStyleSlant(char style)
{
    SkFontStyle::Slant ret = SkFontStyle::Slant::kUpright_Slant;
    if (style == FontStyleItalic) {
        ret = SkFontStyle::Slant::kItalic_Slant;
    } else if (style == FontStyleOblique) {
        ret = SkFontStyle::Slant::kOblique_Slant;
    }
    return ret;
}
static inline char skFontStyleSlantToChar(SkFontStyle::Slant s)
{
    char ret = 'U';
    if (s == SkFontStyle::Slant::kItalic_Slant) {
        ret = 'I';
    } else if (s == SkFontStyle::Slant::kOblique_Slant) {
        ret = 'O';
    }
    return ret;
}

static inline SkFontStyle::Weight fontWeightToSkFontStyleWeight(char weight)
{
    SkFontStyle::Weight ret = SkFontStyle::Weight::kMedium_Weight;
    switch (weight) {
    case 0:
        ret = SkFontStyle::Weight::kInvisible_Weight;
        break;
    case 1:
        ret = SkFontStyle::Weight::kThin_Weight;
        break;
    case 2:
        ret = SkFontStyle::Weight::kExtraLight_Weight;
        break;
    case 3:
        ret = SkFontStyle::Weight::kLight_Weight;
        break;
    case 4:
        ret = SkFontStyle::Weight::kNormal_Weight;
        break;
    case 5:
        ret = SkFontStyle::Weight::kMedium_Weight;
        break;
    case 6:
        ret = SkFontStyle::Weight::kSemiBold_Weight;
        break;
    case 7:
        ret = SkFontStyle::Weight::kBold_Weight;
        break;
    case 8:
        ret = SkFontStyle::Weight::kExtraBold_Weight;
        break;
    case 9:
        ret = SkFontStyle::Weight::kBlack_Weight;
        break;
    case 10:
        ret = SkFontStyle::Weight::kExtraBlack_Weight;
    default:
        STARFISH_ASSERT_NOT_REACHED();
    }
    return ret;
}

static inline char skFontStyleWeightToChar(int w)
{
    char ret = 'M';
    switch (w) {
    case SkFontStyle::Weight::kInvisible_Weight:
        ret = 'I';
        break;
    case SkFontStyle::Weight::kThin_Weight:
        ret = 'T';
        break;
    case SkFontStyle::Weight::kExtraLight_Weight:
        ret = 'l';
        break;
    case SkFontStyle::Weight::kLight_Weight:
        ret = 'L';
        break;
    case SkFontStyle::Weight::kNormal_Weight:
        ret = 'N';
        break;
    case SkFontStyle::Weight::kMedium_Weight:
        ret = 'M';
        break;
    case SkFontStyle::Weight::kSemiBold_Weight:
        ret = 'S';
        break;
    case SkFontStyle::Weight::kBold_Weight:
        ret = 'b';
        break;
    case SkFontStyle::Weight::kExtraBold_Weight:
        ret = 'B';
        break;
    case SkFontStyle::Weight::kBlack_Weight:
        ret = 'k';
        break;
    case SkFontStyle::Weight::kExtraBlack_Weight:
        ret = 'K';
    default:
        STARFISH_ASSERT_NOT_REACHED();
    }
    return ret;
}

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
    sk_sp<SkFontMgr> fm(SkFontMgr::RefDefault());
    sk_sp<SkData> skData = SkData::MakeWithCopy(data, dataLen);
    sk_sp<SkTypeface> skTypeface(fm->createFromData(skData.get()));

    auto impl = new (PointerFreeGC) FontFaceImplSkia(skTypeface);
    impl->setDownLoadFontDataSize(dataLen);

    return impl;
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

FontFaceImplSkia::FontFaceImplSkia(sk_sp<SkTypeface> skTypeface)
{
    m_skTypeface = skTypeface;
    m_downLoadFontDataSize = 0;

    auto paint = skPaint();
    paint.setTextSize(1);

    m_unitsPerEM = skTypeface->getUnitsPerEm();

    SkPaint::FontMetrics fontMetrics;
    paint.getFontMetrics(&fontMetrics);

    m_ascender = -fontMetrics.fAscent * m_unitsPerEM;
    m_descender = -fontMetrics.fDescent * m_unitsPerEM;

    if (-fontMetrics.fAscent < fontMetrics.fXHeight ||
        fontMetrics.fXHeight == 0) {
        // fallback
        m_xHeight = -fontMetrics.fAscent * 0.56 * m_unitsPerEM;
    } else {
        m_xHeight = fontMetrics.fXHeight * m_unitsPerEM;
    }
    m_supportsKerning =
        m_skTypeface->getKerningPairAdjustments(nullptr, 0, nullptr);

    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            FontFaceImplSkia* m = (FontFaceImplSkia*)obj;
            m->m_skTypeface = nullptr;
            GlyphIndexCache().swap(m->m_glyphIndexCache);
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
    GlyphIndexCache().swap(m_glyphIndexCache);
}

bool FontFaceImplSkia::loadGlyph(
    int intSize, char32_t ch,
    std::pair<FontFaceImplSkia*, std::pair<unsigned, LayoutUnit>>& result)
{
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
        SkGlyphID id;
        m_skTypeface->charsToGlyphs(&ch, SkTypeface::kUTF32_Encoding, &id, 1);

        auto paint = skPaint();
        paint.setTextSize(intSize);

        SkScalar w;
        paint.getTextWidths(&id, sizeof(id), &w, nullptr);

        if (id) {
            LayoutUnit width(w);
            result = std::make_pair(this, std::make_pair(id, width));
            return true;
        }
    }
    return false;
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
        sk_sp<SkTypeface> lastSkTypeface = nullptr;

        hb_font_t* hbFont = nullptr;
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
                lastSkTypeface = glyphData.first.first->skTypeface();
                lastUnicodeScript = unicodeScript;
            } else {
                if (glyphData.first.first ||
                    lastSkTypeface != glyphData.first.first->skTypeface() ||
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
        run.m_skTypeface = lastSkTypeface;
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

        if (run.m_skTypeface) {
            int ttcIndex = 0;
            std::unique_ptr<SkStreamAsset> skStream(
                run.m_skTypeface->openStream(&ttcIndex));
            size_t len = skStream->getLength();
            sk_sp<SkData> skData =
                SkData::MakeFromStream(skStream.release(), len);

            hb_font_t* hbfont = createHarfbuzzFont(skData, run.m_skTypeface);

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
                float advance =
                    glyphPositions[k].x_advance * (float)intSize / 64.0f;
                float xOffset =
                    glyphPositions[k].x_offset * (float)intSize / 64.0f;
                float yOffset =
                    glyphPositions[k].y_offset * (float)intSize / 64.0f;

                run.m_glyphs.push_back(glyph);
                run.m_glyphPositions.push_back(
                    LayoutLocation(xOffset + totalAdvance, yOffset));

                totalAdvance += advance;
            }

            hb_font_destroy(hbfont);
            skData = nullptr;
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
    SkFontStyle::Slant skSlant = styleToSkFontStyleSlant(fontStyle);
    SkFontStyle::Weight skWeight = fontWeightToSkFontStyleWeight(fontWeight);
    SkFontStyle skFontStyle(skWeight, SkFontStyle::Width::kNormal_Width,
                            skSlant);
    sk_sp<SkFontMgr> fm(SkFontMgr::RefDefault());
    sk_sp<SkTypeface> skTypeface(
        fm->matchFamilyStyleCharacter(nullptr, skFontStyle, nullptr, 0, ch));

    if (skTypeface == nullptr) {
        return result;
    }

    FontFaceImplSkia* ffimpl =
        (FontFaceImplSkia*)((PlatformFontSelectorImplSkia*)
                                m_fontSelector->platformFontSelector())
            ->loadFontFace(skTypeface);

    if (!hasCacheItem) {
        fallbackFontFaceCachePerCodeBlock.push_back(
            std::make_tuple(blockCode, ffimpl, fontStyle, fontWeight));
    }

    ffimpl->loadGlyph(intSize, ch, glyphResult);
    return std::make_pair(std::make_pair(ffimpl, SIZE_MAX), glyphResult.second);
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
}

sk_sp<SkTypeface> PlatformFontSelectorImplSkia::findAndLoadFontFace(
    const UTF8StringDataNonGCStd& familyName, bool isGenericName, char style,
    char weight)
{
    auto u8FamilyName = familyName;

    SkFontStyle::Slant skSlant = styleToSkFontStyleSlant(style);
    SkFontStyle::Weight skWeight = fontWeightToSkFontStyleWeight(weight);

    SkFontStyle fontStyle(skWeight, SkFontStyle::Width::kNormal_Width, skSlant);

    sk_sp<SkFontMgr> fm(SkFontMgr::RefDefault());

    sk_sp<SkTypeface> result(
        fm->matchFamilyStyle(u8FamilyName.c_str(), fontStyle));

    if (result == nullptr) {
        if (StringUtils::equalsIgnoreCase(
                u8FamilyName.c_str(), m_starfish->initialFontFamilyDatas()[1]
                                          .m_familyName->toUTF8NonGCString()
                                          .c_str())) {
            result = SkTypeface::MakeFromName(u8FamilyName.c_str(), fontStyle);
        }
    }

    return result;
}

static inline void makeSkTypefaceKey(sk_sp<SkTypeface> skTypeface,
                                     SkString& key)
{
    skTypeface->getFamilyName(&key);
    key.append("_");
    auto fs = skTypeface->fontStyle();
    auto c = skFontStyleWeightToChar(fs.weight());
    key.append(&c, 1);
    c = skFontStyleSlantToChar(fs.slant());
    key.append(&c, 1);
}

FontFace* PlatformFontSelectorImplSkia::loadFontFace(
    sk_sp<SkTypeface> skTypeface)
{
    STARFISH_ASSERT(skTypeface != nullptr);
    SkString key;
    makeSkTypefaceKey(skTypeface, key);

    auto iter = m_fontFaceCache.find(key.c_str());
    if (iter != m_fontFaceCache.end()) {
        return iter->second;
    }

    int ttcIndex = 0;
    std::unique_ptr<SkStreamAsset> skStream(skTypeface->openStream(&ttcIndex));
    size_t len = skStream->getLength();
    sk_sp<SkData> skData = SkData::MakeFromStream(skStream.get(), len);

    auto impl = new (PointerFreeGC) FontFaceImplSkia(skTypeface);
    m_fontFaceCache.insert(std::make_pair(key.c_str(), impl));
    return impl;
}

FontFace* FontSelectorImplSkia::loadFromPlatform(
    const UTF8StringDataNonGCStd& fm, bool isGenericName, char style,
    char weight)
{
    if (m_platformFontCache->m_absencePlatformFontNames.find(fm) !=
        m_platformFontCache->m_absencePlatformFontNames.end()) {
        // early give up
        return nullptr;
    } else {
        auto skTypeface =
            ((PlatformFontSelectorImplSkia*)m_platformFontSelector)
                ->findAndLoadFontFace(fm, isGenericName, style, weight);
        if (skTypeface == nullptr) {
            m_platformFontCache->m_absencePlatformFontNames.insert(fm);
            return nullptr;
        } else {
            return ((PlatformFontSelectorImplSkia*)m_platformFontSelector)
                ->loadFontFace(skTypeface);
        }
    }
}
}

#endif
