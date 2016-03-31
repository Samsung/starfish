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
    QualifiedName m_linkLocalName;
    QualifiedName m_scriptLocalName;
    QualifiedName m_metaLocalName;
    QualifiedName m_bodyLocalName;
    QualifiedName m_divLocalName;
    QualifiedName m_pLocalName;
    QualifiedName m_imageLocalName;
    QualifiedName m_spanLocalName;
    QualifiedName m_brLocalName;
    QualifiedName m_audioLocalName;

    // Attribute Names
    QualifiedName m_id;
    QualifiedName m_class;
    QualifiedName m_localName;
    QualifiedName m_style;
    QualifiedName m_src;
    QualifiedName m_width;
    QualifiedName m_height;
    QualifiedName m_rel;
    QualifiedName m_href;
    QualifiedName m_type;

    // Event Names
    QualifiedName m_click;
    QualifiedName m_onclick;
    QualifiedName m_load;
    QualifiedName m_onload;
    QualifiedName m_unload;
    QualifiedName m_onunload;
    QualifiedName m_visibilitychange;

protected:
    std::unordered_map<std::string, QualifiedName,
    std::hash<std::string>, std::equal_to<std::string>, gc_allocator<std::pair<std::string, QualifiedName>>> m_staticStringMap;
};

enum StarFishStartUpFlag {
    enableBlackTheme = 1,
    enableComputedStyleDump = 1 << 1,
    enableFrameTreeDump = 1 << 2,
    enableStackingContextDump = 1 << 3,
    enableHitTestDump = 1 << 4,
    enableRegressionTest = 1 << 5,
};


enum StarFishDeviceKind {
    deviceKindUseMouse = 0,
    deviceKindUseTouchScreen = 1 << 0,
};

class StarFish : public gc {
public:
#ifndef STARFISH_TIZEN_WEARABLE
    StarFish(StarFishStartUpFlag flag, String* currentPath = String::createASCIIString(""), int w = 360, int h = 360);
#else
    StarFish(StarFishStartUpFlag flag, String* currentPath, void* win, int w = 360, int h = 360);
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

    size_t posPrefix(std::string str, std::string prefix)
    {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return str.find(prefix);
    }

    String* makeResourcePath(String* src)
    {
        std::string p = m_currentPath->asASCIIString()->data();
        std::string strSrc = src->asASCIIString()->data();

        if (posPrefix(strSrc, "file://") != std::string::npos)
            p = strSrc.substr(7);
        else if (posPrefix(strSrc, "blob:") != std::string::npos)
            p = strSrc;
        else
            p.append(strSrc);

        return String::createASCIIString(p.data());
    }

    void loadXMLDocument(String* filePath);

    void resume();
    void pause();
    void close();
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

    MessageLoop* messageLoop()
    {
        return m_messageLoop;
    }

    StarFishStartUpFlag startUpFlag()
    {
        return (StarFishStartUpFlag)m_startUpFlag;
    }

    StarFishDeviceKind deviceKind()
    {
        return m_deviceKind;
    }
protected:
    unsigned int m_startUpFlag;
    StarFishDeviceKind m_deviceKind;
    MessageLoop* m_messageLoop;
    ScriptBindingInstance* m_scriptBindingInstance;
    Window* m_window;
    String* m_currentPath;
    std::unordered_map<std::string, ImageData*, std::hash<std::string>, std::equal_to<std::string>,
        gc_allocator<std::pair<std::string, ImageData*>>> m_imageCache;
    FontSelector m_fontSelector;
    StaticStrings m_staticStrings;
};

extern bool g_enablePixelTest;

}
#endif
