#include "StarFishConfig.h"
#include "Font.h"

#include <Evas.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <fontconfig/fontconfig.h>

#include "style/UnitHelper.h"

namespace StarFish {

Evas* internalCanvas();

class FontImplEFL : public Font {
public:
    FontImplEFL(String* familyName,float size, char style, char weight)
    {
        m_size = size;
        m_weight = weight;
        m_style = style;
        /*
        if(textStyle & EireneTextStyleBold) {
            WTF::CString cs = m_fontFamily.utf8();
            familyName.append(":style=Bold");
        }*/
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
        m_fontFamily = familyName;
        loadFont();
        m_spaceWidth = measureText(String::spaceString);
    }
    ~FontImplEFL()
    {
        // TODO unref evas_text_object
    }

    void loadFont()
    {
        m_text = evas_object_text_add(internalCanvas());
        evas_object_text_font_set(m_text,m_fontFamily->utf8Data(), convertFromPxToPt(m_size));
    }

    void unloadFont()
    {
        evas_object_del(m_text);
    }

    virtual float measureText(String* str)
    {
        evas_object_text_text_set(m_text,str->utf8Data());
        Evas_Coord minw, minh;
        evas_object_geometry_get(m_text,0,0,&minw,&minh);
        return minw;
    }
    virtual void* unwrap()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return m_text;
    }

protected:
    Evas_Object* m_text;
};

#define CHECK_ERROR if (error) STARFISH_RELEASE_ASSERT_NOT_REACHED();

Font::FontMetrics loadFontMetrics(String* familyName, double size)
{
    FcConfig* config = FcInitLoadConfigAndFonts();

    FcPattern* pattern = FcPatternCreate();
    FcPatternAddString(pattern, FC_FAMILY, (FcChar8*)familyName->utf8Data());

    FcConfigSubstitute(config, pattern, FcMatchPattern);
    FcDefaultSubstitute (pattern);

    FcResult res;
    FcFontSet* set = FcFontSort(config, pattern, FcTrue, NULL, &res);

    if (!set) {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    std::string fontPath;
    for (int i = 0; i < set->nfont; i++) {
        FcPattern* font = set->fonts[i];
        FcChar8 *file;
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

    Font::FontMetrics met;
    met.m_ascender = ((float)face->ascender / (float)face->units_per_EM) * size;
    met.m_descender = ((float)face->descender / (float)face->units_per_EM) * size;
    met.m_fontHeight = ((float)face->height / (float)face->units_per_EM) * size;

    FT_Done_Face(face);
    FT_Done_FreeType(library);
    return met;

}

Font* FontSelector::loadFont(String* familyName, float size, char style, char weight)
{
    FontImplEFL* f = nullptr;

    for (unsigned i = 0; i < m_fontCache.size(); i ++) {
        if (std::get<1>(m_fontCache[i])->equals(familyName)) {
            if (std::get<2>(m_fontCache[i]) == size && std::get<3>(m_fontCache[i]) == style && std::get<4>(m_fontCache[i]) == weight) {
                return std::get<0>(m_fontCache[i]);
            }
        }
    }

    f = new FontImplEFL(familyName, size, style, weight);
    f->m_metrics = loadFontMetrics(familyName, size);
    m_fontCache.push_back(std::make_tuple(f, familyName, size, style, weight));
    return f;
}

}
