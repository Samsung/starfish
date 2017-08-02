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

#ifndef __StarFishStaticStrings__
#define __StarFishStaticStrings__

namespace StarFish {

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
    AtomicString m_xmlNamespaceURI;
    AtomicString m_xmlnsNamespaceURI;
    AtomicString m_xml;
    AtomicString m_xmlns;
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
    QualifiedName m_cellspacing;
    QualifiedName m_cellpadding;
    QualifiedName m_char;
    QualifiedName m_rowspan;
    QualifiedName m_bgColor;
    QualifiedName m_span;
    QualifiedName m_scope;
    QualifiedName m_disabled;
    QualifiedName m_media;
    QualifiedName m_value;
    QualifiedName m_title;
    QualifiedName m_align;
    QualifiedName m_tabindex;
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

#ifdef STARFISH_ENABLE_TEST
extern bool g_enablePixelTest;
extern bool g_memLogDump;
#endif
}

#endif
