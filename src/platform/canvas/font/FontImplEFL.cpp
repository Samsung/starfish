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

#if defined(PORT_CANVAS_BACKEND_EFL)
#include "StarFish.h"

#include "StarFish.h"
#include <Evas.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <fontconfig/fontconfig.h>
#include "core/modules/canvas/font/Font.h"
#include "core/style/UnitHelper.h"

#if STARFISH_TIZEN && !(STARFISH_TIZEN_WEARABLE) && \
    defined(PORT_CANVAS_BACKEND_EFL)
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

class FontFaceImplEFL : public FontFace {
public:
    FontFaceImplEFL(String* familyName, FontMetrics met, float size, char style,
                    char weight)
    {
        m_text = nullptr;
        m_metrics = met;
        m_familyName = convertStyleParamStr(familyName, style, weight);
        m_size = size;
        m_weight = weight;
        m_style = style;
        m_supportsKerning = false;

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

        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           FontFaceImplEFL* m =
                                               (FontFaceImplEFL*)obj;
                                           if (m->m_text) {
                                               evas_object_hide(m->m_text);
                                               evas_object_del(m->m_text);
                                           }
                                       },
                                       NULL, NULL, NULL);
    }

    void loadFont(int size)
    {
        if (m_text) {
            unloadFont();
        }
        m_text = evas_object_text_add(internalCanvas());
        auto utf8Data = m_familyName->toUTF8NonGCString();
        evas_object_text_font_set(m_text, utf8Data.data(), size);
    }

    void unloadFont()
    {
        evas_object_del(m_text);
        m_text = nullptr;
    }

    virtual FontMetrics metrics(float size)
    {
        return m_metrics;
    }

    float m_size;
    FontMetrics m_metrics;
    Evas_Object* m_text;
};

class FontImplEFL : public Font {
public:
    FontImplEFL()
    {
    }

    virtual bool isGenericFont() const
    {
        return false;
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
            return size() * ((float)count / SPACE_SIZE_DENOMINATOR);
        }
#endif
        auto textObject = ((FontFaceImplEFL*)m_fontFaceList->at(0))->m_text;
        if (str.originalString()->bufferAccessData().hasASCIIContent) {
            bool isShort = str.length() < 128;
            auto data = str.bufferAccessData();
            char* buf =
                isShort ? (char*)alloca(128) : (char*)malloc(str.length() + 1);
            strncpy(buf, data.asciiData(), data.length);
            buf[str.length()] = 0;
            evas_object_text_text_set(textObject, buf);
            if (!isShort) {
                free(buf);
            }
        } else {
            UTF8StringDataNonGCStd s =
                str.originalString()->toUTF8NonGCString(str.start(), str.end());
            evas_object_text_text_set(textObject, s.c_str());
        }

        Evas_Coord minw, minh;
        evas_object_geometry_get(textObject, 0, 0, &minw, &minh);
        return minw;
    }
    virtual void* unwrap()
    {
        return ((FontFaceImplEFL*)m_fontFaceList->at(0))->m_text;
    }

protected:
};

#define CHECK_ERROR                            \
    if (error) {                               \
        STARFISH_RELEASE_ASSERT_NOT_REACHED(); \
    }

static FontMetrics loadFontMetrics(String* familyName, double size)
{
    typedef std::unordered_map<std::string, std::pair<double, FontMetrics>>
        MetricsMap;
    static MetricsMap metricsMap;
    static FcConfig* config = FcInitLoadConfigAndFonts();

    std::string u8FontName = familyName->toUTF8NonGCString();

    auto iter = metricsMap.find(u8FontName);
    if (iter != metricsMap.end()) {
        double factor = size / iter->second.first;

        FontMetrics met;
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

    FontMetrics met;
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

class FontSelectorImplEFL : public FontSelector {
public:
    FontFace* loadFontFaceImpl(String* familyName, float size, char style,
                               char weight)
    {
        FontFaceImplEFL* f = new FontFaceImplEFL(
            familyName,
            loadFontMetrics(convertStyleParamStr(familyName, style, weight),
                            size),
            size, style, weight);
        return f;
    }

    virtual bool isGenericFontSelector() const
    {
        return false;
    }

    static UTF8StringDataNonGCStd mergeFamilyNames(String* familyNameArray[],
                                                   size_t len, float size,
                                                   char style, char weight)
    {
        UTF8StringDataNonGCStd result;
        result.reserve(128);

        for (size_t i = 0; i < len; i++) {
            auto bad = familyNameArray[i]->bufferAccessData();
            for (size_t j = 0; j < bad.length; j++) {
                char32_t c = bad.charAt(j);
                if (c >= 'A' && c <= 'Z') {
                    c -= ('A' - 'a');
                }

                if (LIKELY(c < 128)) {
                    result += (char)c;
                } else {
                    char buf[16];
                    size_t l = utf32ToUtf8(c, buf);
                    result.append(buf, l);
                }
            }
        }

        result += " s:" + std::to_string(size);
        result += " s:" + (style + 'a');
        result += " w:" + (weight + 'a');
        return result;
    }

    Font* loadFont(String* familyNameArray[], size_t familyNameArraySize,
                   float size, char style, char weight)
    {
        auto cacheFontName = mergeFamilyNames(
            familyNameArray, familyNameArraySize, size, style, weight);
        auto iter = m_fontCache.find(cacheFontName);
        if (iter != m_fontCache.end()) {
            return iter->second;
        }

        Font* result = Font::createEmptyFont(this);

        result->m_fontFaceList = new FontFaceList;
        result->m_fontFaceList->push_back(
            loadFontFaceImpl(familyNameArray[0], size, style, weight));
        result->m_size = size;
        result->m_spaceWidth = result->measureText(String::spaceString);
        m_fontCache.insert(std::make_pair(cacheFontName, result));

        return result;
    }
};

Font* Font::createEmptyFont(FontSelector* s)
{
    return new FontImplEFL();
}

FontSelector* FontSelector::createFontSelector()
{
    return new FontSelectorImplEFL();
}
}
#endif
