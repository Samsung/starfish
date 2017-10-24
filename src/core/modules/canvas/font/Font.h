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

#ifndef __Font__
#define __Font__

#ifndef STARFISH_DEFAULT_FONT_FAMILY
#define STARFISH_DEFAULT_FONT_FAMILY "sans-serif"
#endif

#ifdef PORT_CANVAS_BACKEND_CAIRO
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

namespace StarFish {

class FontFace;
class Font;
class FontSelector;

enum FontStyle {
    FontStyleNormal,
    FontStyleItalic,
    FontStyleOblique,
};

enum FontWeight {
    FontWeightStart = 1,
    FontWeightNormal = 4,
    FontWeightEnd = 9,
};

struct FontMetrics {
    LayoutUnit m_ascender;
    LayoutUnit m_descender;
    LayoutUnit m_fontHeight;
    float m_xheightRate;
};

class FontFace : public gc {
    friend class Font;

public:
    virtual ~FontFace()
    {
    }

    String* familyName()
    {
        return m_familyName;
    }

    bool supportsKerning()
    {
        return m_supportsKerning;
    }

    char style()
    {
        return m_style;
    }

    char weight()
    {
        return m_weight;
    }

    virtual FontMetrics metrics(float size) = 0;

protected:
    bool m_supportsKerning;
    char m_weight;
    char m_style;
    String* m_familyName;
};

class FontFaceList : public GCVector<FontFace*>, public gc {
};

class Font : public gc {
    friend class FontSelector;
#if defined(PORT_CANVAS_BACKEND_EFL)
    friend class FontSelectorImplEFL;
#endif

protected:
    Font()
    {
        m_size = 0;
        m_spaceWidth = 0;
        m_fontFaceList = nullptr;
    }

public:
    virtual ~Font()
    {
    }

    virtual LayoutUnit measureText(const StringView& sv) = 0;

#if defined(PORT_CANVAS_BACKEND_EFL)
    virtual void* unwrap()
    {
        return nullptr;
    }
#endif

    float spaceWidth()
    {
        return m_spaceWidth;
    }

    char weight()
    {
        return m_fontFaceList->front()->weight();
    }

    float size()
    {
        return m_size;
    }

    int intSize()
    {
        return int(size() + 0.5f);
    }

    char style()
    {
        return m_fontFaceList->front()->style();
    }

    FontMetrics metrics()
    {
        return m_fontFaceList->front()->metrics(m_size);
    }

    const FontFaceList& fontFaceList()
    {
        return *m_fontFaceList;
    }

#if defined(PORT_CANVAS_BACKEND_EFL)
    virtual bool isGenericFont() const
    {
        return true;
    }
#endif

#ifdef STARFISH_ENABLE_TEST
#define SPACE_SIZE_DENOMINATOR 60
    static inline size_t spaceSizeNumerator(char32_t c)
    {
        size_t count = 60;
        if (c == 0x2000 || c == 0x2002) {
            count = 30;
        } else if (c == 0x2004) {
            count = 20;
        } else if (c == 0x2005) {
            count = 15;
        } else if (c == 0x205F) {
            count = 13;
        } else if (c == 0x2009) {
            count = 12;
        } else if (c == 0x2006) {
            count = 10;
        } else if (c == 0x200A) {
            count = 6;
        } else if (String::isZeroWidthChar(c)) {
            count = 0;
        }
        return count;
    }
#endif

protected:
    static Font* createEmptyFont(FontSelector* s);
#if defined(PORT_CANVAS_BACKEND_EFL)
    static Font* createGenericEmptyFont(FontSelector* s);
#endif

    FontFaceList* m_fontFaceList;
    float m_size;
    float m_spaceWidth;
};

class FontSelector : public gc {
    friend class StarFish;
    friend class Font;

protected:
    GCUnorderedMap<UTF8StringDataNonGCStd, FontFace*> m_fontFaceCache;
    GCUnorderedMap<UTF8StringDataNonGCStd, FontFaceList*> m_fontFaceListCache;
    GCUnorderedMap<UTF8StringDataNonGCStd, Font*> m_fontCache;

    FontSelector()
    {
    }
    virtual ~FontSelector()
    {
    }
    virtual FontFace* loadFontFaceImpl(const UTF8StringDataNonGCStd& familyName,
                                       bool isGenericName, char style = 0,
                                       char weight = 4)
    {
        return nullptr;
    }

#if !defined(PORT_CANVAS_BACKEND_EFL)
    Font* loadFont(String* familyNameArray[], size_t familyNameArraySize,
                   float size, char style = 0, char weight = 4);
#else
    virtual Font* loadFont(String* familyNameArray[],
                           size_t familyNameArraySize, float size,
                           char style = 0, char weight = 4);
#endif
    void clearCache()
    {
        m_fontFaceCache.clear();
    }

    FontFace* lookupFaceCache(const UTF8StringDataNonGCStd& familyName,
                              char style, char weight, bool& exist);
    void insertFaceCache(const UTF8StringDataNonGCStd& familyName, char style,
                         char weight, FontFace* face);
    static FontSelector* createFontSelector();
#if defined(PORT_CANVAS_BACKEND_EFL)
    static FontSelector* createGenericFontSelector();
    virtual bool isGenericFontSelector() const
    {
        return true;
    }
#endif
};
};
#endif
