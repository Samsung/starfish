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

#if defined(PORT_GRAPHIC_BACKEND_EFL)
#include "StarFish.h"

#include "StarFish.h"
#include <Evas.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <fontconfig/fontconfig.h>
#include "core/modules/canvas/font/Font.h"
#include "core/style/UnitHelper.h"

#if STARFISH_TIZEN && !(STARFISH_TIZEN_WEARABLE) && \
    defined(PORT_GRAPHIC_BACKEND_EFL)
extern "C" Evas_Coord evas_object_text_max_ascent_get(const Evas_Text* obj);
extern "C" Evas_Coord evas_object_text_max_descent_get(const Evas_Text* obj);
#endif

namespace StarFish {

extern int g_screenDpi;
Evas* internalCanvas();

static String* convertStyleParamStr(String* familyName, unsigned char style,
                                    char weight)
{
    switch (weight) {
    case 1:
        familyName =
            familyName->concat(String::createASCIIString(":style=thin"));
        break;
    case 2:
        familyName =
            familyName->concat(String::createASCIIString(":style=ultralight"));
        break;
    case 3:
        familyName =
            familyName->concat(String::createASCIIString(":style=light"));
        break;
    case 4:
        familyName =
            familyName->concat(String::createASCIIString(":style=medium"));
        break;
    case 5:
        familyName =
            familyName->concat(String::createASCIIString(":style=semibold"));
        break;
    case 6:
        familyName =
            familyName->concat(String::createASCIIString(":style=bold"));
        break;
    case 7:
        familyName =
            familyName->concat(String::createASCIIString(":style=ultrabold"));
        break;
    case 8:
        familyName =
            familyName->concat(String::createASCIIString(":style=black"));
        break;
    case 9:
        familyName =
            familyName->concat(String::createASCIIString(":style=extrablack"));
        break;
    }

    if (style == FontStyleItalic) {
        familyName = familyName->concat(String::createASCIIString(" italic"));
    } else if (style == FontStyleOblique) {
        familyName = familyName->concat(String::createASCIIString(" oblique"));
    }
    return familyName;
}

class FontImplEFL : public Font {
public:
    FontImplEFL(String* familyName, float size, char style, char weight,
                FontMetrics met)
    {
        m_text = nullptr;
        m_metrics = met;
        m_size = size;
        m_weight = weight;
        m_style = style;
        m_fontFamily = convertStyleParamStr(familyName, style, weight);

        loadFont(m_size);

#ifdef STARFISH_ENABLE_TEST
        if (!g_enablePixelTest) {
            m_metrics.m_ascender = evas_object_text_max_ascent_get(m_text);
            m_metrics.m_descender = -evas_object_text_max_descent_get(m_text);
            m_metrics.m_fontHeight =
                m_metrics.m_ascender - m_metrics.m_descender;
            m_metrics.m_xheightRate = met.m_xheightRate;
        } else {
            // Set the FontMetrics as if font is Ahem.
            m_metrics.m_ascender = m_size * 0.8;
            m_metrics.m_descender = m_metrics.m_ascender - m_size;
            m_metrics.m_fontHeight =
                m_metrics.m_ascender - m_metrics.m_descender;
            m_metrics.m_xheightRate = 0.8f;
        }
#else
        m_metrics.m_ascender = evas_object_text_max_ascent_get(m_text);
        m_metrics.m_descender = -evas_object_text_max_descent_get(m_text);
        m_metrics.m_fontHeight = m_metrics.m_ascender - m_metrics.m_descender;
        m_metrics.m_xheightRate = met.m_xheightRate;
#endif

        m_spaceWidth = measureText(StringView(String::spaceString, 0, 1));

        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           // STARFISH_LOG_INFO("FontImplEFL::~FontImplEFL\n");
                                           FontImplEFL* m = (FontImplEFL*)obj;
                                           if (m->m_text) {
                                               evas_object_hide(m->m_text);
                                               evas_object_del(m->m_text);
                                           }
                                       },
                                       NULL, NULL, NULL);
    }
    ~FontImplEFL()
    {
    }

    void loadFont(int size)
    {
        if (m_text) {
            unloadFont();
        }
        m_text = evas_object_text_add(internalCanvas());
        evas_object_text_font_set(m_text, m_fontFamily->utf8Data(), size);
    }

    void unloadFont()
    {
        evas_object_del(m_text);
        m_text = nullptr;
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
        if (str.originalString()->bufferAccessData().hasASCIIContent) {
            bool isShort = str.length() < 128;
            auto data = str.bufferAccessData();
            char* buf =
                isShort ? (char*)alloca(128) : (char*)malloc(str.length() + 1);
            strncpy(buf, data.asciiData(), data.length);
            buf[str.length()] = 0;
            evas_object_text_text_set(m_text, buf);
            if (!isShort) {
                free(buf);
            }
        } else {
            UTF8StringDataNonGCStd s =
                str.originalString()->toUTF8NonGCString(str.start(), str.end());
            evas_object_text_text_set(m_text, s.c_str());
        }

        Evas_Coord minw, minh;
        evas_object_geometry_get(m_text, 0, 0, &minw, &minh);
        return minw;
    }
    virtual void* unwrap()
    {
        return m_text;
    }

protected:
    Evas_Object* m_text;
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

Font* FontSelector::loadFont(String* familyName, float size, char style,
                             char weight)
{
    FontImplEFL* f = nullptr;

    for (unsigned i = 0; i < m_fontCache.size(); i++) {
        if (std::get<1>(m_fontCache[i])->equals(familyName)) {
            if (std::get<2>(m_fontCache[i]) == size &&
                std::get<3>(m_fontCache[i]) == style &&
                std::get<4>(m_fontCache[i]) == weight) {
                return std::get<0>(m_fontCache[i]);
            }
        }
    }

    f = new FontImplEFL(
        familyName, size, style, weight,
        loadFontMetrics(convertStyleParamStr(familyName, style, weight), size));
    m_fontCache.push_back(std::make_tuple(f, familyName, size, style, weight));
    return f;
}
}
#endif
