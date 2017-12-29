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

#include "binding/DocumentHoldable.h"

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
    virtual FontMetrics metrics(float size) = 0;
    virtual size_t dataSize()
    {
        return 0;
    }
    virtual void clearCache()
    {
    }

    static FontFace* create(const uint8_t* data, size_t dataLen);

protected:
};

class FontFaceList : public GCVector<FontFace*> {
    friend class Font;
    friend class FontSelector;

public:
    FontFaceList()
    {
        m_seenUnresolvedWebFontIndex = SIZE_MAX;
    }

protected:
    size_t m_seenUnresolvedWebFontIndex;
};

class Font : public gc {
    friend class FontSelector;
#if defined(PORT_CANVAS_BACKEND_EFL)
    friend class FontSelectorImplEFL;
#endif

protected:
    Font()
    {
        m_weight = m_style = 0;
        m_size = 0;
        m_spaceWidth = 0;
        m_fontFaceList = nullptr;
    }

public:
    virtual ~Font()
    {
    }

    virtual LayoutUnit measureText(const StringView& sv) = 0;

    virtual void* unwrap()
    {
        return nullptr;
    }

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

    int intSize()
    {
        return int(size() + 0.5f);
    }

    char style()
    {
        return m_style;
    }

    size_t seenUnresolvedWebFontIndex()
    {
        return m_fontFaceList->m_seenUnresolvedWebFontIndex;
    }

    FontMetrics metrics()
    {
        return m_fontFaceList->front()->metrics(m_size);
    }

    const FontFaceList& fontFaceList()
    {
        return *m_fontFaceList;
    }

    FontSelector* fontSelector()
    {
        return m_fontSelector;
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
    FontSelector* m_fontSelector;
    size_t m_seenUnresolvedWebFontIndex;
    char m_weight;
    char m_style;
    float m_size;
    float m_spaceWidth;
};

class PlatformFontSelector : public gc {
public:
    PlatformFontSelector(StarFish* sf)
    {
        m_starfish = sf;
    }
    virtual ~PlatformFontSelector()
    {
    }
    static PlatformFontSelector* create(StarFish* sf);
    virtual UTF8StringDataNonGCStd findFont(
        const UTF8StringDataNonGCStd& familyName, bool isGenericName,
        char style = 0, char weight = 4)
    {
        return UTF8StringDataNonGCStd();
    }

    virtual FontFace* loadFontFace(const UTF8StringDataNonGCStd& path)
    {
        return nullptr;
    }

protected:
    StarFish* m_starfish;
};

class PlatformFontCache : public gc {
    friend class FontSelector;

public:
    static PlatformFontCache* create(StarFish* sf);
    virtual ~PlatformFontCache()
    {
    }

    FontFace* lookupFaceCache(const UTF8StringDataNonGCStd& mergredFamilyName,
                              bool& exist);
    void insertFaceCache(const UTF8StringDataNonGCStd& mergredFamilyName,
                         FontFace* face);

protected:
    std::unordered_set<UTF8StringDataNonGCStd> m_absencePlatformFontNames;
    std::unordered_map<UTF8StringDataNonGCStd, FontFace*> m_loadedPlatformFonts;
};

class FontSelector : public DocumentHoldable, public gc {
    friend class StarFish;
    friend class Font;

public:
#if !defined(PORT_CANVAS_BACKEND_EFL)
    Font* loadFont(String* familyNameArray[], size_t familyNameArraySize,
                   float size, char style = 0, char weight = 4);
#else
    virtual Font* loadFont(String* familyNameArray[],
                           size_t familyNameArraySize, float size,
                           char style = 0, char weight = 4);
#endif

    PlatformFontSelector* platformFontSelector()
    {
        return m_platformFontSelector;
    }

    PlatformFontCache* platformFontCache()
    {
        return m_platformFontCache;
    }

    void clearCache(String* relatedFamilyName);

    static FontSelector* create(Document* document,
                                PlatformFontSelector* platformFontSelector,
                                PlatformFontCache* platformFontCache);

#if defined(PORT_CANVAS_BACKEND_EFL)
    static FontSelector* createGenericFontSelector(
        Document* document, PlatformFontSelector* platformFontSelector,
        PlatformFontCache* platformFontCache);
    virtual bool isGenericFontSelector() const
    {
        return true;
    }
#endif

protected:
    PlatformFontSelector* m_platformFontSelector;
    PlatformFontCache* m_platformFontCache;

    GCUnorderedMap<UTF8StringDataNonGCStd, FontFaceList*> m_fontFaceListCache;
    GCUnorderedMap<UTF8StringDataNonGCStd, Font*> m_fontCache;
    GCUnorderedMap<UTF8StringDataNonGCStd, FontFace*> m_webFontLocalSrcCache;

    FontSelector(Document* document, PlatformFontSelector* platformFontSelector,
                 PlatformFontCache* platformFontCache)
        : DocumentHoldable(document)
    {
        m_platformFontSelector = platformFontSelector;
        m_platformFontCache = platformFontCache;
    }
    virtual ~FontSelector()
    {
    }

    FontFace* loadFromPlatform(const UTF8StringDataNonGCStd& fm,
                               bool isGenericName, char style, char weight);
};
};
#endif
