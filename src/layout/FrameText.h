#ifndef __StarFishFrameText__
#define __StarFishFrameText__

#include "layout/Frame.h"

namespace StarFish {

struct FrameTextTextDecorationData : public gc {
    bool m_hasUnderLine;
    bool m_hasLineThrough;
    Color m_underLineColor;
    Color m_lineThroughColor;
};

class FrameText : public Frame {
public:
    FrameText(Node* node, ComputedStyle* style, FrameTextTextDecorationData* deco)
        : Frame(node, style)
    {
        m_textDecorationData = deco;
    }

    virtual bool isFrameText()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameText";
    }

    FrameTextTextDecorationData* textDecorationData()
    {
        return m_textDecorationData;
    }

    String* text()
    {
        return node()->asCharacterData()->data();
    }

    static std::string replaceAll(const std::string& str, const std::string& pattern, const std::string& replace)
    {
        std::string result = str;
        std::string::size_type pos = 0;
        std::string::size_type offset = 0;

        while ((pos = result.find(pattern, offset)) != std::string::npos) {
            result.replace(result.begin() + pos, result.begin() + pos + pattern.size(), replace);
            offset = pos + replace.size();
        }

        return result;
    }
#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth)
    {
        std::string str = text()->utf8Data();
        str = replaceAll(str, "\n", "\\n");
        printf("text-> %s", str.data());
    }
#endif
    virtual void layout(LayoutContext& ctx)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

protected:
    FrameTextTextDecorationData* m_textDecorationData;
};
}

#endif
