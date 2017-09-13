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

class FontImplCAIRO : public Font {
public:
    FontImplCAIRO(String* familyName, float size, char style, char weight,
                  FontMetrics met, FontSelector* fontSelector)
    {
        m_metrics = met;
        m_size = size;
        m_weight = weight;
        m_style = style;
        m_fontFamily = familyName;
        m_fontSelector = fontSelector;
#ifdef STARFISH_ENABLE_TEST
// if (!g_enablePixelTest) {
//     m_metrics.m_ascender = evas_object_text_max_ascent_get(m_text);
//     m_metrics.m_descender = -evas_object_text_max_descent_get(m_text);
//     m_metrics.m_fontHeight =
//         m_metrics.m_ascender - m_metrics.m_descender;
//     m_metrics.m_xheightRate = met.m_xheightRate;
// } else {
//     // Set the FontMetrics as if font is Ahem.
//     m_metrics.m_ascender = m_size * 0.8;
//     m_metrics.m_descender = m_metrics.m_ascender - m_size;
//     m_metrics.m_fontHeight =
//         m_metrics.m_ascender - m_metrics.m_descender;
//     m_metrics.m_xheightRate = 0.8f;
// }
#else
// m_metrics.m_ascender = evas_object_text_max_ascent_get(m_text);
// m_metrics.m_descender = -evas_object_text_max_descent_get(m_text);
// m_metrics.m_fontHeight = m_metrics.m_ascender - m_metrics.m_descender;
// m_metrics.m_xheightRate = met.m_xheightRate;
#endif

        m_spaceWidth = measureText(StringView(String::spaceString, 0, 1));

        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           // STARFISH_LOG_INFO("FontImplCAIRO::~FontImplCAIRO\n");
                                           FontImplCAIRO* m =
                                               (FontImplCAIRO*)obj;
                                           FontMetrics fm = m->metrics();

                                       },
                                       NULL, NULL, NULL);
    }
    ~FontImplCAIRO()
    {
    }

    virtual LayoutUnit measureText(const StringView& str)
    {
        if (str.length() == 0) {
            return 0;
        }
#ifdef STARFISH_ENABLE_TEST
        if (g_enablePixelTest) {
            size_t count = 0;
            for (size_t i = str.start(); i < str.end(); i++) {
                count += Font::spaceSizeNumerator((*str.originalString())[i]);
            }
            return m_size * ((float)count / SPACE_SIZE_DENOMINATOR);
        }
#endif
        FT_Int x_bias = 0;
        FT_Int y_bias = 0;
        FT_UInt glyph_index = 0;

        FT_Face face = findFCChar(str.charAt(0), &glyph_index);

        int size = m_size;
        int glyph_count = str.length();
        int result = 0;
        int advanceX = 0;
        char32_t unicode;

        for (int i = 0; i < glyph_count; i++) {
            unicode = str.charAt(i);
            advanceX = getGlaphAdvanceX(unicode);
            if (advanceX >= 0) {
                result += advanceX;
            } else {
                glyph_index = FT_Get_Char_Index(face, unicode);
                if (glyph_index != 0) {
                    FT_Set_Pixel_Sizes(face, 0, size);
                    FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER);
                    m_GlaphCaches.emplace(unicode, face->glyph->advance.x >> 6);
                    result += face->glyph->advance.x >> 6;
                } else {
                    face = findFCChar(unicode, &glyph_index);
                    if (glyph_index != 0) {
                        FT_Set_Pixel_Sizes(face, 0, size);
                        FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER);
                        m_GlaphCaches.emplace(unicode,
                                              face->glyph->advance.x >> 6);
                        result += face->glyph->advance.x >> 6;
                    } else {
                        // DO nothing?
                    }
                }
            }
        }
        return result;
    }
    virtual int getGlaphAdvanceX(char32_t uniCode)
    {
        auto it = m_GlaphCaches.find(uniCode);
        if (it != m_GlaphCaches.end()) {
            return it->second;
        }
        return -1;
    }

    virtual void* unwrap()
    {
        return nullptr;
    }

    virtual FT_Face findFCChar(char32_t uniCode, uint* glyphIdx)
    {
        // check cache;
        auto it = m_fontSelector->m_FTFaceCaches.find(uniCode);
        if (it != m_fontSelector->m_FTFaceCaches.end()) {
            *glyphIdx = std::get<1>(it->second);
            return std::get<0>(it->second);
        }

        // load FTFace;
        FT_Face face;
        FT_Error error;
        auto iter = m_fontSelector->m_systemFonts.begin();
        while (iter != m_fontSelector->m_systemFonts.end()) {
            std::pair<std::string, FT_Face>& font = *iter;

            if (font.second == nullptr) {
                error = FT_New_Face(m_fontSelector->m_FTFaceLib,
                                    font.first.data(), 0, &face);
                FT_UInt glyph_index = FT_Get_Char_Index(face, uniCode);
                if (glyph_index != 0) {
                    font.second = face;
                    *glyphIdx = glyph_index;
                    m_fontSelector->m_FTFaceCaches.emplace(
                        uniCode, std::make_tuple(face, glyph_index));
                    return face;
                } else {
                    FT_Done_Face(face);
                }

            } else {
                face = font.second;
                FT_UInt glyph_index = FT_Get_Char_Index(face, uniCode);
                if (glyph_index != 0) {
                    *glyphIdx = glyph_index;
                    m_fontSelector->m_FTFaceCaches.emplace(
                        uniCode, std::make_tuple(face, glyph_index));
                    return face;
                }
            }
            iter++;
        }
        // There is no font which contains this unicode
        // Use FallBack;
        {
            iter = m_fontSelector->m_systemFonts.begin();
            *glyphIdx = 0;
            std::pair<std::string, FT_Face> font = *iter;
            return font.second;
        }

        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return nullptr;
    }

private:
    FontSelector* m_fontSelector;
    std::unordered_map<char32_t, int> m_GlaphCaches;
};

#define CHECK_ERROR                            \
    if (error) {                               \
        STARFISH_RELEASE_ASSERT_NOT_REACHED(); \
    }

static Font::FontMetrics loadFontMetrics(String* familyName, double size)
{
    typedef std::unordered_map<std::string,
                               std::pair<double, Font::FontMetrics>>
        MetricsMap;
    static MetricsMap metricsMap;
    static FcConfig* config = FcInitLoadConfigAndFonts();

    std::string u8FontName = familyName->toUTF8NonGCString();

    auto iter = metricsMap.find(u8FontName);
    if (iter != metricsMap.end()) {
        double factor = size / iter->second.first;

        Font::FontMetrics met;
        met.m_ascender = iter->second.second.m_ascender * factor;
        met.m_descender = iter->second.second.m_descender * factor;
        met.m_fontHeight = iter->second.second.m_fontHeight * factor;
        met.m_xheightRate = iter->second.second.m_xheightRate * factor;

        return met;
    }

    FcPattern* pattern = FcNameParse((const FcChar8*)(u8FontName.data()));

    if (!pattern) {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    FcConfigSubstitute(config, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    FcResult res;
    FcFontSet* set = FcFontSort(config, pattern, FcTrue, NULL, &res);

    if (!set) {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    std::string fontPath;
    for (int i = 0; i < set->nfont; i++) {
        FcPattern* font = set->fonts[i];
        FcChar8* file;
        if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
            fontPath = (char*)file;
            break;
        }
    }

    FcFontSetDestroy(set);
    FcPatternDestroy(pattern);

    FT_Library library;
    FT_Error error;
    error = FT_Init_FreeType(&library);
    CHECK_ERROR;

    FT_Face face;
    error = FT_New_Face(library, fontPath.data(), 0, &face);
    CHECK_ERROR;

    error = FT_Set_Pixel_Sizes(face, 0, size);
    CHECK_ERROR;
    FT_UInt glyph_index = FT_Get_Char_Index(face, 'x');
    error = FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER);
    CHECK_ERROR;
    FT_Int xheight = face->glyph->bitmap_top;

    Font::FontMetrics met;
    met.m_fontHeight =
        ((face->ascender - face->descender) * size) / face->units_per_EM;
    met.m_ascender = ((face->ascender * size) / (face->units_per_EM));
    met.m_descender = met.m_ascender - met.m_fontHeight;
    met.m_xheightRate = xheight / size;

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    metricsMap[u8FontName] = std::make_pair(size, met);
    return met;
}

class FontSelectorImplCairo : public FontSelector {
public:
    FontSelectorImplCairo()
    {
        FcConfig* config = FcInitLoadConfigAndFonts();

#ifdef STARFISH_TIZEN_TV
        std::string fallbackFont = "SamsungOneFallback";
#else
        std::string fallbackFont = "";
#endif

        FcPattern* pattern = FcNameParse((const FcChar8*)(fallbackFont.data()));

        if (!pattern) {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }

#ifdef STARFISH_TIZEN_TV
        FcPatternAddString(pattern, FC_FAMILY, (const FcChar8*)"SamsungOneUI");
        FcPatternAddString(pattern, FC_FAMILY,
                           (const FcChar8*)"SamsungOneUIKoreanH");
#endif

        FcConfigSubstitute(config, pattern, FcMatchPattern);
        FcDefaultSubstitute(pattern);

        FcResult res;
        FcFontSet* set = FcFontSort(config, pattern, FcTrue, NULL, &res);

        if (!set) {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }

        std::string fontPath;
        for (int i = 0; i < set->nfont; i++) {
            FcPattern* font = set->fonts[i];
            FcChar8* file;
            if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
                fontPath = (char*)file;

                // Add every font which's in system.
                m_systemFonts.emplace_back(std::make_pair(fontPath, nullptr));
            }
        }

        FcFontSetDestroy(set);
        FcPatternDestroy(pattern);

        FT_Error error;
        error = FT_Init_FreeType(&m_FTFaceLib);
        CHECK_ERROR;

        FcConfigDestroy(config);
    }

    ~FontSelectorImplCairo()
    {
        auto iter = m_systemFonts.begin();
        while (iter != m_systemFonts.end()) {
            std::pair<std::string, FT_Face> font = *iter;
            if (font.second != nullptr) {
                FT_Done_Face(font.second);
            }
            iter++;
        }
        FT_Done_FreeType(m_FTFaceLib);
    }

    Font* loadFont(String* familyName, float size, char style, char weight)
    {
        FontImplCAIRO* f = nullptr;

        for (unsigned i = 0; i < m_fontCache.size(); i++) {
            if (std::get<1>(m_fontCache[i])->equals(familyName)) {
                if (std::get<2>(m_fontCache[i]) == size &&
                    std::get<3>(m_fontCache[i]) == style &&
                    std::get<4>(m_fontCache[i]) == weight) {
                    return std::get<0>(m_fontCache[i]);
                }
            }
        }

        f = new FontImplCAIRO(familyName, size, style, weight,
                              loadFontMetrics(familyName, size), this);
        m_fontCache.push_back(
            std::make_tuple(f, familyName, size, style, weight));
        return f;
    }
};
#if !defined(PORT_CANVAS_BACKEND_EFL)
FontSelector* FontSelector::createFontSelector()
{
    return new FontSelectorImplCairo();
}
#else
FontSelector* FontSelector::createGenericFontSelector()
{
    return new FontSelectorImplCairo();
}
#endif
}

#endif
