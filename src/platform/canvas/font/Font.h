#ifndef __Font__
#define __Font__

namespace StarFish {

enum FontStyle {
    FontStyleNormal = 1,
    FontStyleBold = 1 << 1,
    FontStyleItalic = 1 << 2,
    FontStyleUnderline = 1 << 3,
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
    virtual ~Font() {

    }

    virtual float measureText(String*) = 0;
    virtual void* unwrap() = 0;

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
        float m_ascender;
        float m_descender;
        float m_fontHeight;
        float m_lineGap;
    };

    const FontMetrics& metrics()
    {
        return m_metrics;
    }

protected:
    FontMetrics m_metrics;
    float m_size;
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
    Font* loadFont(String* familyName, float size, char style = 1, char weight = 4);
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
