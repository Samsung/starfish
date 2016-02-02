#ifndef __StarFish__
#define __StarFish__

#include "platform/canvas/font/Font.h"

namespace StarFish {

class MessageLoop;
class Window;
class ScriptBindingInstance;
class ImageData;

class StaticStrings {
    friend class QualifiedName;
public:
    StaticStrings(StarFish* sf);

    // Node Names
    QualifiedName m_documentLocalName;
    QualifiedName m_textLocalName;
    QualifiedName m_commentLocalName;
    QualifiedName m_htmlLocalName;
    QualifiedName m_headLocalName;
    QualifiedName m_styleLocalName;
    QualifiedName m_scriptLocalName;
    QualifiedName m_metaLocalName;
    QualifiedName m_bodyLocalName;
    QualifiedName m_divLocalName;
    QualifiedName m_pLocalName;
    QualifiedName m_imageLocalName;
    QualifiedName m_spanLocalName;
    QualifiedName m_brLocalName;

    // Attribute Names
    QualifiedName m_id;
    QualifiedName m_class;
    QualifiedName m_localName;
    QualifiedName m_src;

    // Event Names
    QualifiedName m_click;
    QualifiedName m_onclick;

protected:
    std::unordered_map<std::string, QualifiedName,
    std::hash<std::string>, std::equal_to<std::string>, gc_allocator<std::pair<std::string, QualifiedName>>> m_staticStringMap;
};

enum StarFishStartUpFlag {
    enableBlackTheme = 1,
    enableComputedStyleDump = 1 << 1,
    enableFrameTreeDump = 1 << 2,
    enableHitTestDump = 1 << 3,
};

class StarFish : public gc {
public:
#ifndef STARFISH_TIZEN_WEARABLE
    StarFish(StarFishStartUpFlag flag, String* currentPath = String::createASCIIString(""));
#else
    StarFish(StarFishStartUpFlag flag, String* currentPath, void* win);
#endif
    void run();

    Window* window()
    {
        return m_window;
    }

    ScriptBindingInstance* scriptBindingInstance()
    {
        return m_scriptBindingInstance;
    }

    String* currentPath()
    {
        return m_currentPath;
    }

    size_t posPrefix(std::string str)
    {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return str.find("file://");
    }

    String* makeResourcePath(String* src)
    {
        std::string p = m_currentPath->asASCIIString()->data();
        std::string strSrc = src->asASCIIString()->data();

        if(posPrefix(strSrc) != std::string::npos)
            p = strSrc.substr(7);
        else
            p.append(strSrc);


        return String::createASCIIString(p.data());
    }

    void loadXMLDocument(String* filePath);

    void resume();
    void pause();
    void evaluate(String* s);

    ImageData* fetchImage(String* str);
    Font* fetchFont(String* familyName, float size, char style = FontStyle::FontStyleNormal, char weight = FontWeight::FontWeightNormal)
    {
        Font* f = nullptr;
        f = m_fontSelector.loadFont(familyName, size, style, weight);
        return f;
    }

    StaticStrings* staticStrings()
    {
        return &m_staticStrings;
    }

    StarFishStartUpFlag startUpFlag()
    {
        return (StarFishStartUpFlag)m_startUpFlag;
    }
protected:
    unsigned int m_startUpFlag;
    MessageLoop* m_messageLoop;
    ScriptBindingInstance* m_scriptBindingInstance;
    Window* m_window;
    String* m_currentPath;
    std::unordered_map<std::string, ImageData*, std::hash<std::string>, std::equal_to<std::string>,
        gc_allocator<std::pair<std::string, ImageData*>>> m_imageCache;
    FontSelector m_fontSelector;
    StaticStrings m_staticStrings;
};

}
#endif
