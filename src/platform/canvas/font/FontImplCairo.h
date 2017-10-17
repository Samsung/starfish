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
                      FontMetrics met, float size, char style, char weight)
    {
        m_familyName = familyName;
        m_face = face;
        m_hbFace = hbFace;
        m_metrics = met;
        m_size = size;
        m_weight = weight;
        m_style = style;
        m_supportsKerning = m_face->face_flags & FT_FACE_FLAG_KERNING;
    }

    FT_Face m_face;
    hb_font_t* m_hbFace;
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
