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

#ifndef __StarFish__
#define __StarFish__

#include "StarFishConfig.h"

namespace StarFish {

class MessageLoop;
class TimerWrapper;
class Window;
class ScriptBindingInstance;
class ImageData;
class ThreadPool;
class Blob;
class MediaSource;
class Console;
class Inspector;
class HistoryEntry;

#define STARFISH_ENUM_HTML_TAG_NAMES(F) \
    F(a)                                \
    F(abbr)                             \
    F(acronym)                          \
    F(address)                          \
    F(applet)                           \
    F(area)                             \
    F(article)                          \
    F(aside)                            \
    F(audio)                            \
    F(b)                                \
    F(base)                             \
    F(basefont)                         \
    F(bdi)                              \
    F(bdo)                              \
    F(bgsound)                          \
    F(big)                              \
    F(blockquote)                       \
    F(body)                             \
    F(br)                               \
    F(button)                           \
    F(canvas)                           \
    F(caption)                          \
    F(center)                           \
    F(cite)                             \
    F(code)                             \
    F(col)                              \
    F(colgroup)                         \
    F(command)                          \
    F(content)                          \
    F(datalist)                         \
    F(dd)                               \
    F(del)                              \
    F(details)                          \
    F(dfn)                              \
    F(dialog)                           \
    F(dir)                              \
    F(div)                              \
    F(dl)                               \
    F(dt)                               \
    F(em)                               \
    F(embed)                            \
    F(fieldset)                         \
    F(figcaption)                       \
    F(figure)                           \
    F(font)                             \
    F(footer)                           \
    F(form)                             \
    F(frame)                            \
    F(frameset)                         \
    F(h1)                               \
    F(h2)                               \
    F(h3)                               \
    F(h4)                               \
    F(h5)                               \
    F(h6)                               \
    F(head)                             \
    F(header)                           \
    F(hgroup)                           \
    F(hr)                               \
    F(html)                             \
    F(i)                                \
    F(iframe)                           \
    F(image)                            \
    F(img)                              \
    F(input)                            \
    F(ins)                              \
    F(kbd)                              \
    F(keygen)                           \
    F(label)                            \
    F(layer)                            \
    F(legend)                           \
    F(li)                               \
    F(link)                             \
    F(listing)                          \
    F(main)                             \
    F(map)                              \
    F(mark)                             \
    F(marquee)                          \
    F(menu)                             \
    F(meta)                             \
    F(meter)                            \
    F(nav)                              \
    F(nobr)                             \
    F(noembed)                          \
    F(noframes)                         \
    F(nolayer)                          \
    F(noscript)                         \
    F(object)                           \
    F(ol)                               \
    F(optgroup)                         \
    F(option)                           \
    F(output)                           \
    F(p)                                \
    F(param)                            \
    F(plaintext)                        \
    F(pre)                              \
    F(progress)                         \
    F(q)                                \
    F(rp)                               \
    F(rt)                               \
    F(ruby)                             \
    F(s)                                \
    F(samp)                             \
    F(script)                           \
    F(section)                          \
    F(select)                           \
    F(shadow)                           \
    F(small)                            \
    F(source)                           \
    F(span)                             \
    F(strike)                           \
    F(strong)                           \
    F(style)                            \
    F(sub)                              \
    F(summary)                          \
    F(sup)                              \
    F(table)                            \
    F(tbody)                            \
    F(td)                               \
    F(template)                         \
    F(textarea)                         \
    F(tfoot)                            \
    F(th)                               \
    F(thead)                            \
    F(title)                            \
    F(tr)                               \
    F(track)                            \
    F(tt)                               \
    F(u)                                \
    F(ul)                               \
    F(var)                              \
    F(video)                            \
    F(wbr)                              \
    F(xmp)

class StaticStrings : public gc {
    friend class QualifiedName;
    friend class AtomicString;

public:
    StaticStrings(StarFish* sf);
    StarFish* m_starFish;
    AtomicString m_xhtmlNamespaceURI;
    AtomicString m_documentLocalName;
    AtomicString m_documentFragmentLocalName;
    AtomicString m_textLocalName;
    AtomicString m_cdataSectionLocalName;
    AtomicString m_commentLocalName;

    // Pseudo Selector Name Tokens
    AtomicString m_firstChildSelector;
    AtomicString m_firstOfTypeSelector;
    AtomicString m_lastChildSelector;
    AtomicString m_lastOfTypeSelector;
    AtomicString m_onlyChildSelector;
    AtomicString m_onlyOfTypeSelector;
    AtomicString m_emptySelector;
    AtomicString m_firstLineSelector;
    AtomicString m_firstLetterSelector;
    AtomicString m_nthChildPSelector;
    AtomicString m_nthLastChildPSelector;
    AtomicString m_nthOfTypePSelector;
    AtomicString m_nthLastOfTypePSelector;
    AtomicString m_linkSelector;
    AtomicString m_hoverSelector;
    AtomicString m_focusSelector;
    AtomicString m_activeSelector;
    AtomicString m_enabledSelector;
    AtomicString m_disabledSelector;
    AtomicString m_targetSelector;
    AtomicString m_beforeSelector;
    AtomicString m_afterSelector;
    AtomicString m_langPSelector;
    AtomicString m_notPSelector;
    AtomicString m_selectionSelector;
    AtomicString m_rootSelector;

// HTML Tag Names
#define DEFINE_HTML_LOCAL_NAMES(name) QualifiedName m_##name##TagName;
    STARFISH_ENUM_HTML_TAG_NAMES(DEFINE_HTML_LOCAL_NAMES)
#undef DEFINE_HTML_LOCAL_NAMES

    // Attribute Names
    QualifiedName m_id;
    QualifiedName m_name;
    QualifiedName m_class;
    QualifiedName m_localName;
    QualifiedName m_style;
    QualifiedName m_src;
    QualifiedName m_width;
    QualifiedName m_height;
    QualifiedName m_rel;
    QualifiedName m_href;
    QualifiedName m_type;
    QualifiedName m_dir;
    QualifiedName m_color;
    QualifiedName m_face;
    QualifiedName m_size;
    QualifiedName m_charset;
    QualifiedName m_content;
    QualifiedName m_lang;
    QualifiedName m_colspan;
    QualifiedName m_rowspan;
    QualifiedName m_bgColor;
    QualifiedName m_span;
    QualifiedName m_scope;
    QualifiedName m_disabled;
    QualifiedName m_media;
#ifdef STARFISH_ENABLE_MULTIMEDIA
    QualifiedName m_default;
    QualifiedName m_loop;
    QualifiedName m_autoplay;
    QualifiedName m_preload;
    QualifiedName m_controls;
    QualifiedName m_kind;
    QualifiedName m_label;
    QualifiedName m_srclang;
#endif

    // Event Names
    QualifiedName m_click;
    QualifiedName m_onclick;
    QualifiedName m_mousedown;
    QualifiedName m_onmousedown;
    QualifiedName m_mousemove;
    QualifiedName m_onmousemove;
    QualifiedName m_mouseout;
    QualifiedName m_onmouseout;
    QualifiedName m_mouseover;
    QualifiedName m_onmouseover;
    QualifiedName m_mouseup;
    QualifiedName m_onmouseup;
    QualifiedName m_touchstart;
    QualifiedName m_ontouchstart;
    QualifiedName m_touchmove;
    QualifiedName m_ontouchmove;
    QualifiedName m_touchend;
    QualifiedName m_ontouchend;
    QualifiedName m_load;
    QualifiedName m_onload;
    QualifiedName m_error;
    QualifiedName m_onerror;
    QualifiedName m_unload;
    QualifiedName m_onunload;
    QualifiedName m_visibilitychange;
    QualifiedName m_DOMContentLoaded;
    QualifiedName m_readystatechange;
    QualifiedName m_progress;
    QualifiedName m_abort;
    QualifiedName m_timeout;
    QualifiedName m_loadend;
    QualifiedName m_loadstart;
    QualifiedName m_enter;
    QualifiedName m_exit;
    QualifiedName m_blur;
    QualifiedName m_onblur;
    QualifiedName m_focus;
    QualifiedName m_onfocus;
    QualifiedName m_focusin;
    QualifiedName m_onfocusin;
    QualifiedName m_focusout;
    QualifiedName m_onfocusout;
#ifdef STARFISH_ENABLE_MULTIMEDIA
    QualifiedName m_cuechange;
    QualifiedName m_sourceopen;
    QualifiedName m_sourceended;
    QualifiedName m_sourceclose;
    QualifiedName m_removesourcebuffer;
    QualifiedName m_addsourcebuffer;
    QualifiedName m_updatestart;
    QualifiedName m_update;
    QualifiedName m_updateend;
    QualifiedName m_suspend;
    QualifiedName m_emptied;
    QualifiedName m_stalled;
    QualifiedName m_loadedmetadata;
    QualifiedName m_loadeddata;
    QualifiedName m_canplay;
    QualifiedName m_canplaythrough;
    QualifiedName m_playing;
    QualifiedName m_waiting;
    QualifiedName m_seeking;
    QualifiedName m_seeked;
    QualifiedName m_ended;
    QualifiedName m_closed;
    QualifiedName m_open;
    QualifiedName m_durationchange;
    QualifiedName m_timeupdate;
    QualifiedName m_play;
    QualifiedName m_pause;
    QualifiedName m_ratechange;
    QualifiedName m_volumechange;
#endif
    QualifiedName m_keydown;
    QualifiedName m_onkeydown;
    QualifiedName m_keyup;

protected:
};

enum StarFishStartUpFlag {
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

struct BlobURLStore {
#ifdef STARFISH_32
    void* m_blob;
    uint32_t m_a;
    uint32_t m_b;
    uint32_t m_c;
#else
    void* m_blob;
    uint32_t m_a;
    uint32_t m_b;
#endif
};
}

namespace std {
template <>
struct hash<StarFish::BlobURLStore> {
    size_t operator()(StarFish::BlobURLStore const& x) const
    {
        return (size_t)x.m_blob;
    }
};

template <>
struct equal_to<StarFish::BlobURLStore> {
    bool operator()(StarFish::BlobURLStore const& a,
                    StarFish::BlobURLStore const& b) const
    {
        return a.m_blob == b.m_blob;
    }
};
}

namespace StarFish {

void addGCCollectionListener(void (*fn)(GC_EventType));

// you must call delete
// StarFish::StarFish function is NOT THREAD-SAFE
class StarFish : public gc {
    friend class AtomicString;
    friend class StaticStrings;
    friend class StarFishEnterer;

public:
    StarFish(StarFishStartUpFlag flag, const char* locale,
             const char* timezoneID, void* platformHandle, int w, int h,
             float defaultFontSizeMultiplier, float devicePixelRatio = 1);
    ~StarFish();
    void run();

    Window* window()
    {
        return m_window;
    }

    void loadHTMLDocument(String* filePath);

    void resume();
    void pause();
    void close();
    String* evaluate(String* s);

    Font* fetchFont(String* familyName, float size,
                    char style = FontStyle::FontStyleNormal,
                    char weight = FontWeight::FontWeightNormal)
    {
        Font* f = nullptr;
        f = m_fontSelector.loadFont(familyName, size, style, weight);
        return f;
    }

    StaticStrings* staticStrings()
    {
        return m_staticStrings;
    }

    MessageLoop* messageLoop()
    {
        return m_messageLoop;
    }

    TimerWrapper* timer()
    {
        return m_timer;
    }

    StarFishStartUpFlag startUpFlag()
    {
        return (StarFishStartUpFlag)m_startUpFlag;
    }

    StarFishDeviceKind deviceKind()
    {
        return m_deviceKind;
    }

    const icu::Locale& locale()
    {
        return m_locale;
    }

    icu::BreakIterator* lineBreaker()
    {
        return m_lineBreaker;
    }

    String* timezoneID()
    {
        return m_timezoneID;
    }

    ThreadPool* threadPool()
    {
        return m_threadPool;
    }

    float defaultFontSizeMultiplier()
    {
        return m_defaultFontSizeMultiplier;
    }

    void* nativeHandle()
    {
        return m_nativeHandle;
    }

    float devicePixelRatio()
    {
        // TODO: consider page zoom factor.
        return m_devicePixelRatio;
    }

    void addPointerInRootSet(void* ptr);
    void removePointerFromRootSet(void* ptr);
#ifndef NDEBUG
    size_t countPointersInRootSet(void* ptr);
#endif

    static bool stringToBlobURLString(String* url, BlobURLStore& result);
    static String* blobURLStoreToString(BlobURLStore store, String* origin);

    BlobURLStore addBlobInBlobURLStore(Blob* ptr);
    void removeBlobFromBlobURLStore(Blob* ptr);
    bool isValidBlobURL(BlobURLStore ptr);
    bool isValidBlobURL(Blob* ptr);
    BlobURLStore findBlobURL(Blob* ptr);

    BlobURLStore addMediaSourceInBlobURLStore(MediaSource* ptr);
    void removeMediaSourceFromBlobURLStore(MediaSource* ptr);
    bool isValidMediaSourceBlobURL(BlobURLStore ptr);
    bool isValidMediaSourceBlobURL(MediaSource* ptr);
    BlobURLStore findMediaSourceBlobURL(MediaSource* ptr);
    void clearBlobURLStore();

    Console* console()
    {
        return m_console;
    }

#if defined(STARFISH_ENABLE_INSPECTOR)
    Inspector* inspector()
    {
        return m_inspector;
    }

    void setupInspector(uint32_t portNumber = 23888);
#endif
protected:
    void enter();
    void exit();

    size_t posPrefix(std::string str, std::string prefix)
    {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return str.find(prefix);
    }
    StaticStrings* m_staticStrings;
    icu::Locale m_locale;
    icu::BreakIterator* m_lineBreaker;
    String* m_timezoneID;
    unsigned int m_startUpFlag;
    float m_defaultFontSizeMultiplier;
    StarFishDeviceKind m_deviceKind;
    MessageLoop* m_messageLoop;
    TimerWrapper* m_timer;
    void* m_nativeHandle;
    Window* m_window;
    FontSelector m_fontSelector;
    ThreadPool* m_threadPool;
    Console* m_console;
#if defined(STARFISH_ENABLE_INSPECTOR)
    Inspector* m_inspector;
#endif
    size_t m_enterCount;
    unsigned int m_seed;
    float m_devicePixelRatio;

    GCUnorderedMap<void*, size_t> m_rootMap;
    GCUnorderedSet<BlobURLStore> m_urlBlobStore;
    GCUnorderedSet<BlobURLStore> m_urlMediaSourceBlobStore;
    GCUnorderedMap<std::string, AtomicString> m_atomicStringMap;
};

class StarFishEnterer {
public:
    StarFishEnterer(StarFish* instance)
        : m_instance(instance)
    {
        m_instance->enter();
    }

    ~StarFishEnterer()
    {
        m_instance->exit();
    }

protected:
    StarFish* m_instance;
};

#ifdef STARFISH_ENABLE_TEST
extern bool g_enablePixelTest;
extern bool g_memLogDump;
#endif
}

#endif
