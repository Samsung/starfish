#ifndef __Font__
#define __Font__

#include "layout/LayoutUtil.h"

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

    virtual LayoutUnit measureText(String*) = 0;
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
    };

    const FontMetrics& metrics()
    {
        return m_metrics;
    }

protected:
    FontMetrics m_metrics;
    float m_size;
    float m_spaceWidth;
    char m_weight;
    char m_style;
    String* m_fontFamily;
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
    Font* loadFont(String* familyName, float size, char style = 0, char weight = 4);
    void clearCache()
    {
        m_fontCache.clear();
        m_fontCache.shrink_to_fit();
    }
public:
    std::vector<std::tuple<Font*, String*, float, char, char>, gc_allocator<std::tuple<Font*, String*, float, char, char>>> m_fontCache;
};

};
#endif
