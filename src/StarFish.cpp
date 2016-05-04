#include "StarFishConfig.h"
#include "StarFish.h"
#include "platform/message_loop/MessageLoop.h"
#include "platform/window/Window.h"
#include "platform/canvas/image/ImageData.h"
#include "dom/binding/ScriptBindingInstance.h"

#include <Elementary.h>
#include <Ecore_X.h>

namespace StarFish {

#ifdef STARFISH_ENABLE_PIXEL_TEST
bool g_enablePixelTest = false;
#endif

#ifndef STARFISH_TIZEN_WEARABLE_APP
StarFish::StarFish(StarFishStartUpFlag flag, String* currentPath, const char* locale, int w, int h)
    : m_staticStrings(this)
    , m_locale(icu::Locale::createFromName(locale))
    , m_lineBreaker(nullptr)
{
    GC_set_on_collection_event([](GC_EventType evtType) {
        if (GC_EVENT_PRE_START_WORLD == evtType) {
            /*
#ifdef NDEBUG
            STARFISH_LOG_INFO("did GC. GC heapSize...%f MB\n", GC_get_heap_size() / 1024.f / 1024.f);
#else
            STARFISH_LOG_INFO("did GC. GC heapSize...%f MB / %f MB\n", GC_get_memory_use() / 1024.f / 1024.f, GC_get_heap_size() / 1024.f / 1024.f);
#endif
             */
            STARFISH_LOG_INFO("did GC. GC heapSize...%f MB , %f MB\n", GC_get_memory_use() / 1024.f / 1024.f, GC_get_total_bytes() / 1024.f / 1024.f);
        }
    });

    GC_set_free_space_divisor(64);
    // STARFISH_LOG_INFO("GC_get_free_space_divisor is %d\n", (int)GC_get_free_space_divisor());
    m_deviceKind = deviceKindUseMouse;
    m_startUpFlag = flag;
    m_currentPath = currentPath;
    elm_init(0, 0);
    elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

    init(w, h);
    m_window = Window::create(this, w, h);
}
#else
StarFish::StarFish(StarFishStartUpFlag flag, String* currentPath, const char* locale, void* win, int w, int h)
    : m_staticStrings(this)
    , m_locale(icu::Locale::createFromName(locale))
    , m_lineBreaker(nullptr)
{
    GC_set_on_collection_event([](GC_EventType evtType) {
        if (GC_EVENT_PRE_START_WORLD == evtType) {
#ifdef NDEBUG
            STARFISH_LOG_INFO("did GC. GC heapSize...%f MB\n", GC_get_heap_size() / 1024.f / 1024.f);
#else
            STARFISH_LOG_INFO("did GC. GC heapSize...%f MB / %f MB\n", GC_get_memory_use() / 1024.f / 1024.f, GC_get_heap_size() / 1024.f / 1024.f);
#endif
        }
    });
    GC_set_free_space_divisor(64);
    // STARFISH_LOG_INFO("GC_get_free_space_divisor is %d\n", (int)GC_get_free_space_divisor());
    m_deviceKind = deviceKindUseTouchScreen;
    m_startUpFlag = flag;
    m_currentPath = currentPath;

    init(w, h);
    m_window = Window::create(this, w, h, win);
}
#endif

void StarFish::init(int w, int h)
{
    UErrorCode code = U_ZERO_ERROR;
    m_lineBreaker = icu::BreakIterator::createLineInstance(m_locale, code);
    STARFISH_RELEASE_ASSERT(code <= U_ZERO_ERROR);
    GC_add_roots(String::emptyString, String::emptyString + sizeof(String*));
    GC_add_roots(String::spaceString, String::spaceString + sizeof(String*));
    m_messageLoop = new MessageLoop();
    m_scriptBindingInstance = new ScriptBindingInstance();
    m_scriptBindingInstance->initBinding(this);
}

void StarFish::run()
{
    m_messageLoop->run();
}

void StarFish::loadPreprocessedXMLDocument(String* filePath)
{
    m_window->loadPreprocessedXMLDocument(filePath);
}

void StarFish::loadHTMLDocument(String* filePath)
{
    m_window->navigate(filePath);
}

void StarFish::resume()
{
    m_window->resume();
}

void StarFish::pause()
{
    m_window->pause();
}

void StarFish::close()
{
    m_window->close();
}

void StarFish::evaluate(String* s)
{
    m_scriptBindingInstance->evaluate(s);
}

ImageData* StarFish::fetchImage(String* str)
{
    auto iter = m_imageCache.find(str->utf8Data());
    if (iter == m_imageCache.end()) {
        ImageData* id = ImageData::create(str);
        m_imageCache.insert(std::make_pair(std::string(str->utf8Data()), id));
        return id;
    }
    return iter->second;
}

void StarFish::addPointerInRootSet(void *ptr)
{
    auto iter = m_rootMap.find(ptr);
    if (iter == m_rootMap.end()) {
        m_rootMap.insert(std::make_pair(ptr, 1));
    } else {
        iter->second++;
    }
}

void StarFish::removePointerFromRootSet(void *ptr)
{
    auto iter = m_rootMap.find(ptr);
    if (iter->second == 1) {
        m_rootMap.erase(iter);
    } else {
        iter->second--;
    }
}

StaticStrings::StaticStrings(StarFish* sf)
{
    String* s = String::emptyString;
    QualifiedName emptyAtom(s);
    m_staticStringMap.insert(std::make_pair(std::string(), emptyAtom));

    m_documentLocalName = QualifiedName::fromString(sf, "#document");
    m_textLocalName = QualifiedName::fromString(sf, "#text");
    m_commentLocalName = QualifiedName::fromString(sf, "#comment");
#define DEFINE_HTML_LOCAL_NAMES(name) \
    m_##name##LocalName = QualifiedName::fromString(sf, #name);
    STARFISH_ENUM_HTML_TAG_NAMES(DEFINE_HTML_LOCAL_NAMES)
#undef DEFINE_HTML_LOCAL_NAMES

    m_id = QualifiedName::fromString(sf, "id");
    m_class = QualifiedName::fromString(sf, "class");
    m_localName = QualifiedName::fromString(sf, "localName");
    m_style = QualifiedName::fromString(sf, "style");
    m_src = QualifiedName::fromString(sf, "src");
    m_width = QualifiedName::fromString(sf, "width");
    m_height = QualifiedName::fromString(sf, "height");
    m_rel = QualifiedName::fromString(sf, "rel");
    m_href = QualifiedName::fromString(sf, "href");
    m_type = QualifiedName::fromString(sf, "type");
    m_dir = QualifiedName::fromString(sf, "dir");
    m_color = QualifiedName::fromString(sf, "color");
    m_face = QualifiedName::fromString(sf, "face");
    m_size = QualifiedName::fromString(sf, "size");

    m_click = QualifiedName::fromString(sf, "click");
    m_onclick = QualifiedName::fromString(sf, "onclick");
    m_load = QualifiedName::fromString(sf, "load");
    m_onload = QualifiedName::fromString(sf, "onload");
    m_unload = QualifiedName::fromString(sf, "unload");
    m_onunload = QualifiedName::fromString(sf, "onunload");
    m_visibilitychange = QualifiedName::fromString(sf, "visibilitychange");
}

}
