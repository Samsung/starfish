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

#ifndef __FontImplCairo__
#define __FontImplCairo__

#include <cairo.h>
#include <cairo/cairo-ft.h>
#include <fontconfig/fontconfig.h>
#include <hb.h>
#include <hb-ft.h>
#include <hb-icu.h>

#include "core/modules/canvas/font/Font.h"

namespace StarFish {

class FontSelectorImplCairo;

class FontFaceImplCairo : public FontFace {
public:
    FontFaceImplCairo(String* familyName, FT_Face face, hb_font_t* hbFace,
                      char style, char weight)
    {
        m_familyName = familyName;
        m_face = face;
        m_hbFace = hbFace;
        m_weight = weight;
        m_style = style;
        m_supportsKerning = m_face->face_flags & FT_FACE_FLAG_KERNING;

        FT_Error error;
        FT_UInt glyph_index = FT_Get_Char_Index(m_face, 'x');
        if (glyph_index) {
            error = FT_Load_Glyph(m_face, glyph_index, FT_LOAD_NO_SCALE);
            m_xHeight = m_face->glyph->metrics.height;
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
    }

    virtual FontMetrics metrics(float size)
    {
        int intSize = int(size + 0.5f);

        FontMetrics met;
        met.m_fontHeight =
            ((m_ascender - m_descender) * intSize) / m_unitsPerEM;
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

    FT_Face m_face;
    hb_font_t* m_hbFace;
    unsigned m_xHeight;
    unsigned m_unitsPerEM;
    unsigned m_ascender;
    unsigned m_descender;
};

class FontCairoTextRun {
public:
    StringView m_text;
    FT_Face m_ftFace;
    hb_font_t* m_hbFont;
    hb_script_t m_script;
    std::vector<unsigned> m_glyphs;
    std::vector<LayoutLocation> m_glyphPositions;
    LayoutUnit m_runWidth;
};

class FontImplCairo : public Font {
public:
    friend class FontSelectorImplCairo;

    FontImplCairo(FontSelectorImplCairo* fontSelector)
    {
        m_fontSelector = fontSelector;
        m_spaceWidth = 0;
    }

    ~FontImplCairo()
    {
    }

    virtual LayoutUnit measureText(const StringView& str);

    virtual void* unwrap()
    {
        return nullptr;
    }

    FontSelectorImplCairo* m_fontSelector;
};

std::vector<FontCairoTextRun> generateFontCairoTextRuns(const String* text,
                                                        FontImplCairo* font);
bool cairoBackendCanUseSimpleFontPath(Font* f, const StringView& sv);
std::pair<std::pair<FT_Face, hb_font_t*>, std::pair<unsigned, LayoutUnit>>
cairoBackendInternalLoadGlyph(Font* f, char32_t ch);
};

#endif
