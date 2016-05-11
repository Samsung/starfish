#include "StarFishConfig.h"
#include "StarFish.h"
#include "platform/message_loop/MessageLoop.h"
#include "platform/window/Window.h"
#include "platform/canvas/image/ImageData.h"
#include "dom/binding/ScriptBindingInstance.h"

#include <Elementary.h>
#ifdef ESCARGOT_TIZEN3
#include <Ecore.h>
#else
#include <Ecore_X.h>
#endif

namespace StarFish {

#ifdef STARFISH_ENABLE_PIXEL_TEST
bool g_enablePixelTest = false;
#endif

#ifndef STARFISH_TIZEN_WEARABLE_APP
StarFish::StarFish(StarFishStartUpFlag flag, String* currentPath, const char* locale, const char* timezoneID, int w, int h)
    : m_locale(icu::Locale::createFromName(locale))
    , m_lineBreaker(nullptr)
    , m_timezoneID(String::fromUTF8(timezoneID))
{
    GC_set_on_collection_event([](GC_EventType evtType) {
        if (GC_EVENT_PRE_START_WORLD == evtType) {
            STARFISH_LOG_INFO("did GC. GC heapSize...%f MB , %f MB\n", GC_get_memory_use() / 1024.f / 1024.f, GC_get_heap_size() / 1024.f / 1024.f);
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
    ScriptBindingInstanceEnterer enter(m_scriptBindingInstance);
    m_window = Window::create(this, w, h);
}
#else
StarFish::StarFish(StarFishStartUpFlag flag, String* currentPath, const char* locale, const char* timezoneID, void* win, int w, int h)
    : m_locale(icu::Locale::createFromName(locale))
    , m_lineBreaker(nullptr)
    , m_timezoneID(String::fromUTF8(timezoneID))
{
    GC_set_on_collection_event([](GC_EventType evtType) {
        if (GC_EVENT_PRE_START_WORLD == evtType) {
            STARFISH_LOG_INFO("did GC. GC heapSize...%f MB , %f MB\n", GC_get_memory_use() / 1024.f / 1024.f, GC_get_heap_size() / 1024.f / 1024.f);
        }
    });
    GC_set_free_space_divisor(64);
    // STARFISH_LOG_INFO("GC_get_free_space_divisor is %d\n", (int)GC_get_free_space_divisor());
    m_deviceKind = deviceKindUseTouchScreen;
    m_startUpFlag = flag;
    m_currentPath = currentPath;

    init(w, h);
    ScriptBindingInstanceEnterer enter(m_scriptBindingInstance);
    m_window = Window::create(this, w, h, win);
}
#endif

void StarFish::init(int w, int h)
{
    String* s = String::emptyString;
    AtomicString emptyAtom(s);
    m_atomicStringMap.insert(std::make_pair(std::string(), emptyAtom));
    m_staticStrings = new StaticStrings(this);
    UErrorCode code = U_ZERO_ERROR;
    m_lineBreaker = icu::BreakIterator::createLineInstance(m_locale, code);
    STARFISH_RELEASE_ASSERT(code <= U_ZERO_ERROR);
    GC_add_roots(String::emptyString, String::emptyString + sizeof(String*));
    GC_add_roots(String::spaceString, String::spaceString + sizeof(String*));
    m_messageLoop = new MessageLoop(this);
    m_scriptBindingInstance = new ScriptBindingInstance();
    m_scriptBindingInstance->initBinding(this);
}

void StarFish::run()
{
    m_messageLoop->run();
}

void StarFish::loadPreprocessedXMLDocument(String* filePath)
{
    ScriptBindingInstanceEnterer enter(m_scriptBindingInstance);
    m_window->loadPreprocessedXMLDocument(filePath);
}

void StarFish::loadHTMLDocument(String* filePath)
{
    ScriptBindingInstanceEnterer enter(m_scriptBindingInstance);
    m_window->navigate(filePath);
}

void StarFish::resume()
{
    ScriptBindingInstanceEnterer enter(m_scriptBindingInstance);
    m_window->resume();
}

void StarFish::pause()
{
    ScriptBindingInstanceEnterer enter(m_scriptBindingInstance);
    m_window->pause();
    GC_gcollect_and_unmap();
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
    : m_starFish(sf)
    , m_xhtmlNamespaceURI(AtomicString::createAtomicString(sf, "http://www.w3.org/1999/xhtml"))
    , m_documentLocalName(AtomicString::createAtomicString(sf, "#document"))
    , m_documentFragmentLocalName(AtomicString::createAtomicString(sf, "#document-fragment"))
    , m_textLocalName(AtomicString::createAtomicString(sf, "#text"))
    , m_commentLocalName(AtomicString::createAtomicString(sf, "#comment"))
{
#define DEFINE_HTML_LOCAL_NAMES(name) \
    m_##name##TagName = QualifiedName(m_xhtmlNamespaceURI, AtomicString::createAtomicString(sf, #name));
    STARFISH_ENUM_HTML_TAG_NAMES(DEFINE_HTML_LOCAL_NAMES)
#undef DEFINE_HTML_LOCAL_NAMES
    m_id = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(sf, "id"));
    m_name = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(sf, "name"));
    m_class = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(sf, "class"));
    m_localName = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(sf, "localName"));
    m_style = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(sf, "style"));
    m_src = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(sf, "src"));
    m_width = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(sf, "width"));
    m_height = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(sf, "height"));
    m_rel = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(sf, "rel"));
    m_href = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(sf, "href"));
    m_type = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(sf, "type"));
    m_dir = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(sf, "dir"));
    m_color = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(sf, "color"));
    m_face = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(sf, "face"));
    m_size = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(sf, "size"));

    m_click = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(sf, "click"));
    m_onclick = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(sf, "onclick"));
    m_load = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(sf, "load"));
    m_onload = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(sf, "onload"));
    m_unload = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(sf, "unload"));
    m_onunload = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(sf, "onunload"));
    m_visibilitychange = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(sf, "visibilitychange"));
}

}
