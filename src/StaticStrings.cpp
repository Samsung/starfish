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

#include "StarFishConfig.h"
#include "StaticStrings.h"

namespace StarFish {

StaticStrings::StaticStrings(StarFish* sf)
    : m_starFish(sf)
    , m_xhtmlNamespaceURI(
          AtomicString::createAtomicString(sf, "http://www.w3.org/1999/xhtml"))
    // https://infra.spec.whatwg.org/#xml-namespace
    , m_xmlNamespaceURI(AtomicString::createAtomicString(
          sf, "http://www.w3.org/XML/1998/namespace"))
    // https://infra.spec.whatwg.org/#xmlns-namespace
    , m_xmlnsNamespaceURI(
          AtomicString::createAtomicString(sf, "http://www.w3.org/2000/xmlns/"))
    , m_xml(AtomicString::createAtomicString(sf, "xml"))
    , m_xmlns(AtomicString::createAtomicString(sf, "xmlns"))
    , m_documentLocalName(AtomicString::createAtomicString(sf, "#document"))
    , m_documentFragmentLocalName(
          AtomicString::createAtomicString(sf, "#document-fragment"))
    , m_textLocalName(AtomicString::createAtomicString(sf, "#text"))
    , m_cdataSectionLocalName(
          AtomicString::createAtomicString(sf, "#cdata-section"))
    , m_commentLocalName(AtomicString::createAtomicString(sf, "#comment"))
{
#define DEFINE_HTML_LOCAL_NAMES(name)  \
    m_##name##TagName = QualifiedName( \
        m_xhtmlNamespaceURI, AtomicString::createAtomicString(sf, #name));
    STARFISH_ENUM_HTML_TAG_NAMES(DEFINE_HTML_LOCAL_NAMES)
#undef DEFINE_HTML_LOCAL_NAMES

    m_firstChildSelector = AtomicString::createAtomicString(sf, "first-child");
    m_firstOfTypeSelector =
        AtomicString::createAtomicString(sf, "first-of-type");
    m_lastChildSelector = AtomicString::createAtomicString(sf, "last-child");
    m_lastOfTypeSelector = AtomicString::createAtomicString(sf, "last-of-type");
    m_onlyChildSelector = AtomicString::createAtomicString(sf, "only-child");
    m_onlyOfTypeSelector = AtomicString::createAtomicString(sf, "only-of-type");
    m_emptySelector = AtomicString::createAtomicString(sf, "empty");
    m_firstLineSelector = AtomicString::createAtomicString(sf, "first-line");
    m_firstLetterSelector =
        AtomicString::createAtomicString(sf, "first-letter");
    m_nthChildPSelector = AtomicString::createAtomicString(sf, "nth-child(");
    m_nthLastChildPSelector =
        AtomicString::createAtomicString(sf, "nth-last-child(");
    m_nthOfTypePSelector = AtomicString::createAtomicString(sf, "nth-of-type(");
    m_nthLastOfTypePSelector =
        AtomicString::createAtomicString(sf, "nth-last-of-type(");
    m_linkSelector = AtomicString::createAtomicString(sf, "link");
    m_hoverSelector = AtomicString::createAtomicString(sf, "hover");
    m_focusSelector = AtomicString::createAtomicString(sf, "focus");
    m_activeSelector = AtomicString::createAtomicString(sf, "active");
    m_enabledSelector = AtomicString::createAtomicString(sf, "enabled");
    m_disabledSelector = AtomicString::createAtomicString(sf, "disabled");
    m_targetSelector = AtomicString::createAtomicString(sf, "target");
    m_beforeSelector = AtomicString::createAtomicString(sf, "before");
    m_afterSelector = AtomicString::createAtomicString(sf, "after");
    m_langPSelector = AtomicString::createAtomicString(sf, "lang(");
    m_notPSelector = AtomicString::createAtomicString(sf, "not(");
    m_selectionSelector = AtomicString::createAtomicString(sf, "selection");
    m_rootSelector = AtomicString::createAtomicString(sf, "root");

    m_id = QualifiedName(AtomicString::emptyAtomicString(),
                         AtomicString::createAtomicString(sf, "id"));
    m_name = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "name"));
    m_class = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "class"));
    m_localName =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "localName"));
    m_style = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "style"));
    m_src = QualifiedName(AtomicString::emptyAtomicString(),
                          AtomicString::createAtomicString(sf, "src"));
    m_width = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "width"));
    m_height = QualifiedName(AtomicString::emptyAtomicString(),
                             AtomicString::createAtomicString(sf, "height"));
    m_rel = QualifiedName(AtomicString::emptyAtomicString(),
                          AtomicString::createAtomicString(sf, "rel"));
    m_href = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "href"));
    m_type = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "type"));
    m_dir = QualifiedName(AtomicString::emptyAtomicString(),
                          AtomicString::createAtomicString(sf, "dir"));
    m_title = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "title"));
    m_disabled =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "disabled"));
    m_color = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "color"));
    m_face = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "face"));
    m_size = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "size"));
    m_charset = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "charset"));
    m_content = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "content"));
    m_lang = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "lang"));
    m_colspan = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "colspan"));
    m_char = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "char"));
    m_rowspan = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "rowspan"));
    m_bgColor = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "bgcolor"));
    m_span = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "span"));
    m_scope = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "scope"));
    m_media = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "media"));
    m_value = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "value"));
#ifdef STARFISH_ENABLE_MULTIMEDIA
    m_default = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "default"));
    m_loop = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "loop"));
    m_autoplay =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "autoplay"));
    m_preload = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "preload"));
    m_controls =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "controls"));
    m_kind = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "kind"));
    m_label = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "label"));
    m_srclang = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "srclang"));
#endif

    m_click = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "click"));
    m_onclick = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "onclick"));
    m_mousedown =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "mousedown"));
    m_onmousedown =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onmousedown"));
    m_mousemove =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "mousemove"));
    m_onmousemove =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onmousemove"));
    m_mouseout =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "mouseout"));
    m_onmouseout =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onmouseout"));
    m_mouseover =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "mouseover"));
    m_onmouseover =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onmouseover"));
    m_mouseup = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "mouseup"));
    m_onmouseup =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onmouseup"));
    m_touchstart =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "touchstart"));
    m_ontouchstart =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "ontouchstart"));
    m_touchmove =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "touchmove"));
    m_ontouchmove =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "ontouchmove"));
    m_touchend =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "touchend"));
    m_ontouchend =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "ontouchend"));
    m_load = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "load"));
    m_onload = QualifiedName(AtomicString::emptyAtomicString(),
                             AtomicString::createAtomicString(sf, "onload"));
    m_error = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "error"));
    m_onerror = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "onerror"));
    m_unload = QualifiedName(AtomicString::emptyAtomicString(),
                             AtomicString::createAtomicString(sf, "unload"));
    m_onunload =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onunload"));
    m_visibilitychange =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "visibilitychange"));
    m_DOMContentLoaded =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "DOMContentLoaded"));
    m_readystatechange =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "readystatechange"));
    m_progress =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "progress"));
    m_abort = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "abort"));
    m_timeout = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "timeout"));
    m_loadend = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "loadend"));
    m_loadstart =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "loadstart"));
    m_enter = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "enter"));
    m_exit = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "exit"));
    m_blur = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "blur"));
    m_onblur = QualifiedName(AtomicString::emptyAtomicString(),
                             AtomicString::createAtomicString(sf, "onblur"));
    m_focus = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "focus"));
    m_onfocus = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "onfocus"));
    m_focusin = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "focusin"));
    m_onfocusin =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onfocusin"));
    m_focusout =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "focusout"));
    m_onfocusout =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onfocusout"));
#ifdef STARFISH_ENABLE_MULTIMEDIA
    m_cuechange =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "cuechange"));
    m_sourceopen =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "sourceopen"));
    m_sourceended =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "sourceended"));
    m_sourceclose =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "sourceclose"));
    m_addsourcebuffer =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "addsourcebuffer"));
    m_removesourcebuffer = QualifiedName(
        AtomicString::emptyAtomicString(),
        AtomicString::createAtomicString(sf, "removesourcebuffer"));
    m_updatestart =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "updatestart"));
    m_update = QualifiedName(AtomicString::emptyAtomicString(),
                             AtomicString::createAtomicString(sf, "update"));
    m_updateend =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "updateend"));
    m_suspend = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "suspend"));
    m_emptied = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "emptied"));
    m_stalled = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "stalled"));
    m_loadedmetadata =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "loadedmetadata"));
    m_loadeddata =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "loadeddata"));
    m_canplay = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "canplay"));
    m_canplaythrough =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "canplaythrough"));
    m_playing = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "playing"));
    m_waiting = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "waiting"));
    m_seeking = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "seeking"));
    m_seeked = QualifiedName(AtomicString::emptyAtomicString(),
                             AtomicString::createAtomicString(sf, "seeked"));
    m_ended = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "ended"));
    m_open = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "open"));
    m_closed = QualifiedName(AtomicString::emptyAtomicString(),
                             AtomicString::createAtomicString(sf, "closed"));
    m_durationchange =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "durationchange"));
    m_timeupdate =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "timeupdate"));
    m_play = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "play"));
    m_pause = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "pause"));
    m_ratechange =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "ratechange"));
    m_volumechange =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "volumechange"));
#endif
    m_keydown = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "keydown"));
    m_keyup = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "keyup"));
}
}
