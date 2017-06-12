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
                  FontMetrics met)
    {
        m_metrics = met;
        m_size = size;
        m_weight = weight;
        m_style = style;
        m_fontFamily = familyName;

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
                                           FT_Done_Face(fm.m_FTFace);
                                           FT_Done_FreeType(fm.m_FTFaceLib);
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
        cairo_surface_t* surface;
        cairo_t* cr;

        surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 120, 120);
        cr = cairo_create(surface);
        FT_Face face = metrics().m_FTFace;
        cairo_font_face_t* fontFace;
        fontFace = cairo_ft_font_face_create_for_ft_face(face, 0);

        cairo_set_font_face(cr, fontFace);
        cairo_set_font_size(cr, m_size);
        auto scaled_face = cairo_get_scaled_font(cr);
        cairo_glyph_t* glyphs = NULL;
        int glyph_count;
        cairo_text_extents_t extents;

        if (str.originalString()->hasASCIIContent()) {
            bool isShort = str.length() < 128;
            auto data = str.bufferAccessData();
            char* buf =
                isShort ? (char*)alloca(128) : (char*)malloc(str.length() + 1);
            strncpy(buf, data.asciiData(), data.length);
            buf[str.length()] = 0;

            cairo_scaled_font_text_to_glyphs(scaled_face, 0, 0, buf,
                                             strlen(buf), &glyphs, &glyph_count,
                                             nullptr, nullptr, nullptr);

            if (!isShort) {
                free(buf);
            }

        } else {
            UTF8StringDataNonGCStd s =
                str.originalString()->toUTF8NonGCString(str.start(), str.end());
            cairo_scaled_font_text_to_glyphs(scaled_face, 0, 0, s.c_str(),
                                             s.length(), &glyphs, &glyph_count,
                                             nullptr, nullptr, nullptr);
        }
        cairo_scaled_font_glyph_extents(scaled_face, glyphs, glyph_count,
                                        &extents);

        return extents.x_advance;
    }

    virtual void* unwrap()
    {
        return nullptr;
    }
};

#define CHECK_ERROR                            \
    if (error) {                               \
        STARFISH_RELEASE_ASSERT_NOT_REACHED(); \
    }

Font::FontMetrics loadFontMetrics(String* familyName, double size)
{
    FcConfig* config = FcInitLoadConfigAndFonts();

    // FcPattern* pattern = FcNameParse((const
    // FcChar8*)(familyName->utf8Data()));
    // TODO : need to fallback font
    FcPattern* pattern = FcNameParse((const FcChar8*)("NanumGothic"));

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
    FcConfigDestroy(config);

    FT_Library library;
    FT_Face face;
    FT_Error error;
    error = FT_Init_FreeType(&library);
    CHECK_ERROR;

    // FT_Face face;
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

    met.m_FTFace = face;
    met.m_FTFaceLib = library;

    return met;
}

Font* FontSelector::loadFont(String* familyName, float size, char style,
                             char weight)
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

    Font::FontMetrics fontMetrics = loadFontMetrics(familyName, size);
    f = new FontImplCAIRO(familyName, size, style, weight, fontMetrics);
    m_fontCache.push_back(std::make_tuple(f, familyName, size, style, weight));
    return f;
}
}

#endif
