/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "StarFishConfig.h"
#include "StaticStrings.h"

namespace StarFish {

StaticStrings::StaticStrings(StarFish* sf)
    : m_starFish(sf)
    // https://infra.spec.whatwg.org/#namespaces
    , m_xhtmlNamespaceURI(
          AtomicString::createAtomicString(sf, "http://www.w3.org/1999/xhtml"))
    , m_xlinkNamespaceURI(
          AtomicString::createAtomicString(sf, "http://www.w3.org/1999/xlink"))
    , m_xmlNamespaceURI(AtomicString::createAtomicString(
          sf, "http://www.w3.org/XML/1998/namespace"))
    , m_xmlnsNamespaceURI(
          AtomicString::createAtomicString(sf, "http://www.w3.org/2000/xmlns/"))
    , m_svgNamespaceURI(
          AtomicString::createAtomicString(sf, "http://www.w3.org/2000/svg"))
    , m_mathmlNamespaceURI(AtomicString::createAtomicString(
          sf, "http://www.w3.org/1998/Math/MathML"))
    , m_xlink(AtomicString::createAtomicString(sf, "xlink"))
    , m_xml(AtomicString::createAtomicString(sf, "xml"))
    , m_xmlns(AtomicString::createAtomicString(sf, "xmlns"))
    , m_documentLocalName(AtomicString::createAtomicString(sf, "#document"))
    , m_documentFragmentLocalName(
          AtomicString::createAtomicString(sf, "#document-fragment"))
    , m_textLocalName(AtomicString::createAtomicString(sf, "#text"))
    , m_cdataSectionLocalName(
          AtomicString::createAtomicString(sf, "#cdata-section"))
    , m_commentLocalName(AtomicString::createAtomicString(sf, "#comment"))
    , m_true(AtomicString::createAtomicString(sf, "true"))
    , m_false(AtomicString::createAtomicString(sf, "false"))
{
#define DEFINE_HTML_LOCAL_NAMES(name)  \
    m_##name##TagName = QualifiedName( \
        m_xhtmlNamespaceURI, AtomicString::createAtomicString(sf, #name));
    STARFISH_ENUM_HTML_TAG_NAMES(DEFINE_HTML_LOCAL_NAMES)
#undef DEFINE_HTML_LOCAL_NAMES

#define DEFINE_SVG_LOCAL_NAMES(name)      \
    m_svg##name##TagName = QualifiedName( \
        m_svgNamespaceURI, AtomicString::createAtomicString(sf, #name));
    STARFISH_ENUM_SVG_TAG_NAMES(DEFINE_SVG_LOCAL_NAMES)
#undef DEFINE_SVG_LOCAL_NAMES

#define DEFINE_MATHML_LOCAL_NAMES(name)      \
    m_mathml##name##TagName = QualifiedName( \
        m_mathmlNamespaceURI, AtomicString::createAtomicString(sf, #name));
    STARFISH_ENUM_MATHML_TAG_NAMES(DEFINE_MATHML_LOCAL_NAMES)
#undef DEFINE_MATHML_LOCAL_NAMES

#define DEFINE_PSEUDO_SELECTOR_LOCAL_NAMES(name, nameLower, selectorName) \
    m_##nameLower##Selector =                                             \
        AtomicString::createAtomicString(sf, selectorName);
    STARFISH_ENUM_PSEUDO_SELECTORS(DEFINE_PSEUDO_SELECTOR_LOCAL_NAMES)
#undef DEFINE_PSEUDO_SELECTOR_LOCAL_NAMES

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
    m_align = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "align"));
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
    m_httpEquiv =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "http-equiv"));
    m_contentLanguage =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "content-language"));
    m_contentEditable =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "contentEditable"));
    m_designMode =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "designMode"));
    m_lang = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "lang"));
    m_cols = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "cols"));
    m_colspan = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "colspan"));
    m_cellspacing =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "cellspacing"));
    m_cellpadding =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "cellpadding"));
    m_char = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "char"));
    m_rows = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "rows"));
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
    m_defaultValue =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "defaultValue"));
    m_max = QualifiedName(AtomicString::emptyAtomicString(),
                          AtomicString::createAtomicString(sf, "max"));
    m_maxlength =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "maxlength"));
    m_min = QualifiedName(AtomicString::emptyAtomicString(),
                          AtomicString::createAtomicString(sf, "min"));
    m_step = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "step"));
    m_tabindex =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "tabindex"));
    m_formAction =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "formAction"));
    m_action = QualifiedName(AtomicString::emptyAtomicString(),
                             AtomicString::createAtomicString(sf, "action"));
    m_enctype = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "enctype"));
    m_method = QualifiedName(AtomicString::emptyAtomicString(),
                             AtomicString::createAtomicString(sf, "method"));
    m_target = QualifiedName(AtomicString::emptyAtomicString(),
                             AtomicString::createAtomicString(sf, "target"));
    m_formEnctype =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "formyEnctype"));
    m_formMethod =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "formMethod"));
    m_formTarget =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "formTarget"));
    m_checked = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "checked"));
    m_checkbox =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "checkbox"));
    m_radio = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "radio"));
    m_text = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "text"));
    m_placeholder =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "placeholder"));
    m_selected =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "selected"));
    m_multiple =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "multiple"));
    m_required =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "required"));
    m_alt = QualifiedName(AtomicString::emptyAtomicString(),
                          AtomicString::createAtomicString(sf, "alt"));
    m_scrolling =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "scrolling"));
    m_frameborder =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "frameborder"));
    m_autofocus =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "autofocus"));
    m_async = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "async"));
    m_defer = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "defer"));
    m_compact = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "compact"));
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
    m_fill = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "fill"));
    m_fillRule =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "fill-rule"));
    m_fillOpacity =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "fill-opacity"));
    m_stroke = QualifiedName(AtomicString::emptyAtomicString(),
                             AtomicString::createAtomicString(sf, "stroke"));
    m_strokeWidth =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "stroke-width"));
    m_x = QualifiedName(AtomicString::emptyAtomicString(),
                        AtomicString::createAtomicString(sf, "x"));
    m_y = QualifiedName(AtomicString::emptyAtomicString(),
                        AtomicString::createAtomicString(sf, "y"));
    m_r = QualifiedName(AtomicString::emptyAtomicString(),
                        AtomicString::createAtomicString(sf, "r"));
    m_rx = QualifiedName(AtomicString::emptyAtomicString(),
                         AtomicString::createAtomicString(sf, "rx"));
    m_ry = QualifiedName(AtomicString::emptyAtomicString(),
                         AtomicString::createAtomicString(sf, "ry"));
    m_transform =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "transform"));
    m_viewBox = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "viewBox"));
    m_preserveAspectRatio = QualifiedName(
        AtomicString::emptyAtomicString(),
        AtomicString::createAtomicString(sf, "preserveAspectRatio"));
    m_d = QualifiedName(AtomicString::emptyAtomicString(),
                        AtomicString::createAtomicString(sf, "d"));
    m_points = QualifiedName(AtomicString::emptyAtomicString(),
                             AtomicString::createAtomicString(sf, "points"));
    m_cx = QualifiedName(AtomicString::emptyAtomicString(),
                         AtomicString::createAtomicString(sf, "cx"));
    m_cy = QualifiedName(AtomicString::emptyAtomicString(),
                         AtomicString::createAtomicString(sf, "cy"));
    m_fontDashFamily =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "font-family"));
    m_fontDashSize =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "font-size"));
    m_start = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "start"));
    m_dirname = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "dirname"));
    m_readonly =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "readonly"));

    m_xmlBase = QualifiedName(AtomicString::createAtomicString(sf, "xml"),
                              AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "base"));
    m_ariaHidden =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "aria-hidden"));
    m_ariaLabel =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "aria-label"));
    m_ariaLabelledby =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "aria-labelledby"));
    m_ariaDescribedby =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "aria-describedby"));

    m_click = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "click"));
    m_onclick = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "onclick"));
    m_change = QualifiedName(AtomicString::emptyAtomicString(),
                             AtomicString::createAtomicString(sf, "change"));
    m_onchange =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onchange"));
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
    m_mouseenter =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "mouseenter"));
    m_onmouseenter =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onmouseenter"));
    m_mouseleave =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "mouseleave"));
    m_onmouseleave =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onmouseleave"));
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
    m_submit = QualifiedName(AtomicString::emptyAtomicString(),
                             AtomicString::createAtomicString(sf, "submit"));
    m_onsubmit =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onsubmit"));
    m_input = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "input"));
    m_oninput = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "oninput"));
    m_invalid = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "invalid"));
    m_oninvalid =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "oninvalid"));
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
    m_resize = QualifiedName(AtomicString::emptyAtomicString(),
                             AtomicString::createAtomicString(sf, "resize"));
    m_onresize =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onresize"));
    m_message = QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "message"));
    m_onmessage =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onmessage"));
    m_messageerror =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "messageerror"));
    m_onmessageerror =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onmessageerror"));
    m_transitionstart =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "transitionstart"));
    m_transitionend =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "transitionend"));
    m_transitioncancel =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "transitioncancel"));

    m_open = QualifiedName(AtomicString::emptyAtomicString(),
                           AtomicString::createAtomicString(sf, "open"));

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
    m_keypress =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "keypress"));
    m_keyup = QualifiedName(AtomicString::emptyAtomicString(),
                            AtomicString::createAtomicString(sf, "keyup"));
    m_onkeydown =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onkeydown"));
    m_onkeypress =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "onkeypress"));
    m_compositionstart =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "compositionstart"));
    m_compositionupdate = QualifiedName(
        AtomicString::emptyAtomicString(),
        AtomicString::createAtomicString(sf, "compositionupdate"));
    m_compositionend =
        QualifiedName(AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(sf, "compositionend"));

    m_xlinkType = QualifiedName(m_xlink, m_xlinkNamespaceURI,
                                AtomicString::createAtomicString(sf, "type"));
    m_xlinkHref = QualifiedName(m_xlink, m_xlinkNamespaceURI,
                                AtomicString::createAtomicString(sf, "href"));
    m_xlinkRole = QualifiedName(m_xlink, m_xlinkNamespaceURI,
                                AtomicString::createAtomicString(sf, "role"));
    m_xlinkArcrole =
        QualifiedName(m_xlink, m_xlinkNamespaceURI,
                      AtomicString::createAtomicString(sf, "arcrole"));
    m_xlinkTitle = QualifiedName(m_xlink, m_xlinkNamespaceURI,
                                 AtomicString::createAtomicString(sf, "title"));
    m_xlinkShow = QualifiedName(m_xlink, m_xlinkNamespaceURI,
                                AtomicString::createAtomicString(sf, "show"));
    m_xlinkActuate =
        QualifiedName(m_xlink, m_xlinkNamespaceURI,
                      AtomicString::createAtomicString(sf, "actuate"));
    m_xlinkLabel = QualifiedName(m_xlink, m_xlinkNamespaceURI,
                                 AtomicString::createAtomicString(sf, "label"));
    m_xlinkFrom = QualifiedName(m_xlink, m_xlinkNamespaceURI,
                                AtomicString::createAtomicString(sf, "from"));
    m_xlinkTo = QualifiedName(m_xlink, m_xlinkNamespaceURI,
                              AtomicString::createAtomicString(sf, "to"));
}
}
