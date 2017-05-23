/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include <ft2build.h>
#include FT_FREETYPE_H

namespace StarFish {

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

class Font : public gc {
    friend class FontSelector;

protected:
    Font()
    {
    }

public:
    virtual ~Font()
    {
    }

    virtual LayoutUnit measureText(const StringView& sv) = 0;
    virtual void* unwrap() = 0;

    float spaceWidth()
    {
        return m_spaceWidth;
    }

    char weight()
    {
        return m_weight;
    }

    float size()
    {
        return m_size;
    }

    char style()
    {
        return m_style;
    }

    String* familyName()
    {
        return m_fontFamily;
    }

    struct FontMetrics {
        LayoutUnit m_ascender;
        LayoutUnit m_descender;
        LayoutUnit m_fontHeight;
        float m_xheightRate;
    };

    const FontMetrics& metrics()
    {
        return m_metrics;
    }

    FT_Face FTFace()
    {
        return m_FTFace;
    }

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
    FontMetrics m_metrics;
    float m_size;
    float m_spaceWidth;
    char m_weight;
    char m_style;
    String* m_fontFamily;

    FT_Face m_FTFace;
};

class FontSelector {
protected:
    friend class StarFish;
    FontSelector()
    {
    }
    ~FontSelector()
    {
    }
    Font* loadFont(String* familyName, float size, char style = 0,
                   char weight = 4);
    void clearCache()
    {
        m_fontCache.clear();
        m_fontCache.shrink_to_fit();
    }

public:
    GCVector<std::tuple<Font*, String*, float, char, char>> m_fontCache;
};
};
#endif
