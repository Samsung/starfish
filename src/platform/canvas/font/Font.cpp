#include "StarFishConfig.h"
#include "Font.h"

#include <Evas.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <fontconfig/fontconfig.h>

#include "style/UnitHelper.h"

namespace StarFish {

extern int g_screenDpi;
Evas* internalCanvas();

static String* convertStyleParamStr(String* familyName, char style, char weight)
{
    switch (weight) {
    case 1:
        familyName = familyName->concat(String::createASCIIString(":style=thin"));
        break;
    case 2:
        familyName = familyName->concat(String::createASCIIString(":style=ultralight"));
        break;
    case 3:
        familyName = familyName->concat(String::createASCIIString(":style=light"));
        break;
    case 4:
        familyName = familyName->concat(String::createASCIIString(":style=medium"));
        break;
    case 5:
        familyName = familyName->concat(String::createASCIIString(":style=semibold"));
        break;
    case 6:
        familyName = familyName->concat(String::createASCIIString(":style=bold"));
        break;
    case 7:
        familyName = familyName->concat(String::createASCIIString(":style=ultrabold"));
        break;
    case 8:
        familyName = familyName->concat(String::createASCIIString(":style=black"));
        break;
    case 9:
        familyName = familyName->concat(String::createASCIIString(":style=extrablack"));
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
    FontImplEFL(String* familyName, float size, char style, char weight, FontMetrics met)
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
            m_metrics.m_fontHeight = m_metrics.m_ascender - m_metrics.m_descender;
            m_metrics.m_xheightRate = met.m_xheightRate;
        }
#else
        m_metrics.m_ascender = evas_object_text_max_ascent_get(m_text);
        m_metrics.m_descender = -evas_object_text_max_descent_get(m_text);
        m_metrics.m_fontHeight = m_metrics.m_ascender - m_metrics.m_descender;
        m_metrics.m_xheightRate = met.m_xheightRate;
#endif

        m_spaceWidth = measureText(String::spaceString);

        GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
            // STARFISH_LOG_INFO("FontImplEFL::~FontImplEFL\n");
            Evas_Object* m = (Evas_Object*)cd;
            evas_object_hide(m);
            evas_object_del(m);
        }, m_text, NULL, NULL);
    }
    ~FontImplEFL()
    {
    }

    void loadFont(int size)
    {
        if (m_text)
            unloadFont();
        m_text = evas_object_text_add(internalCanvas());
        evas_object_text_font_set(m_text, m_fontFamily->utf8Data(), size);
    }

    void unloadFont()
    {
        evas_object_del(m_text);
        m_text = nullptr;
    }

    virtual LayoutUnit measureText(String* str)
    {
#ifdef STARFISH_ENABLE_TEST
        if (g_enablePixelTest) {
            return str->length() * m_size;
        }
#endif
        evas_object_text_text_set(m_text, str->utf8Data());
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

Font::FontMetrics loadFontMetrics(String* familyName, double size)
{
    FcConfig* config = FcInitLoadConfigAndFonts();

    FcPattern* pattern = FcNameParse((const FcChar8*)(familyName->utf8Data()));

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
    met.m_fontHeight = ((face->ascender - face->descender) * size) / face->units_per_EM;
    met.m_ascender = ((face->ascender * size) / (face->units_per_EM));
    met.m_descender = met.m_ascender - met.m_fontHeight;
    met.m_xheightRate = xheight / size;

    FT_Done_Face(face);
    FT_Done_FreeType(library);
    return met;
}

Font* FontSelector::loadFont(String* familyName, float size, char style, char weight)
{
    FontImplEFL* f = nullptr;

    for (unsigned i = 0; i < m_fontCache.size(); i++) {
        if (std::get<1>(m_fontCache[i])->equals(familyName)) {
            if (std::get<2>(m_fontCache[i]) == size && std::get<3>(m_fontCache[i]) == style && std::get<4>(m_fontCache[i]) == weight) {
                return std::get<0>(m_fontCache[i]);
            }
        }
    }

    f = new FontImplEFL(familyName, size, style, weight, loadFontMetrics(convertStyleParamStr(familyName, style, weight),  size));
    m_fontCache.push_back(std::make_tuple(f, familyName, size, style, weight));
    return f;
}
}
