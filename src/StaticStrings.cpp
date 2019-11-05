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

#include "StarfishConfig.h"
#include "StaticStrings.h"

namespace Starfish {

StaticStrings::StaticStrings(Starfish* starfish)
    : m_starfish(starfish)
    // https://infra.spec.whatwg.org/#namespaces
    , m_xhtmlNamespaceURI(AtomicString::createAtomicString(
          starfish, "http://www.w3.org/1999/xhtml"))
    , m_xlinkNamespaceURI(AtomicString::createAtomicString(
          starfish, "http://www.w3.org/1999/xlink"))
    , m_xmlNamespaceURI(AtomicString::createAtomicString(
          starfish, "http://www.w3.org/XML/1998/namespace"))
    , m_xmlnsNamespaceURI(AtomicString::createAtomicString(
          starfish, "http://www.w3.org/2000/xmlns/"))
    , m_svgNamespaceURI(AtomicString::createAtomicString(
          starfish, "http://www.w3.org/2000/svg"))
    , m_mathmlNamespaceURI(AtomicString::createAtomicString(
          starfish, "http://www.w3.org/1998/Math/MathML"))
    , m_xlink(AtomicString::createAtomicString(starfish, "xlink"))
    , m_xml(AtomicString::createAtomicString(starfish, "xml"))
    , m_xmlns(AtomicString::createAtomicString(starfish, "xmlns"))
    , m_documentLocalName(
          AtomicString::createAtomicString(starfish, "#document"))
    , m_documentFragmentLocalName(
          AtomicString::createAtomicString(starfish, "#document-fragment"))
    , m_textLocalName(AtomicString::createAtomicString(starfish, "#text"))
    , m_cdataSectionLocalName(
          AtomicString::createAtomicString(starfish, "#cdata-section"))
    , m_commentLocalName(AtomicString::createAtomicString(starfish, "#comment"))
    , m_true(AtomicString::createAtomicString(starfish, "true"))
    , m_false(AtomicString::createAtomicString(starfish, "false"))
{
#define DEFINE_HTML_LOCAL_NAMES(name)      \
    m_##name##TagName =                    \
        QualifiedName(m_xhtmlNamespaceURI, \
                      AtomicString::createAtomicString(starfish, #name));
    STARFISH_ENUM_HTML_TAG_NAMES(DEFINE_HTML_LOCAL_NAMES)
#undef DEFINE_HTML_LOCAL_NAMES

#define DEFINE_SVG_LOCAL_NAMES(name)      \
    m_svg##name##TagName = QualifiedName( \
        m_svgNamespaceURI, AtomicString::createAtomicString(starfish, #name));
    STARFISH_ENUM_SVG_TAG_NAMES(DEFINE_SVG_LOCAL_NAMES)
#undef DEFINE_SVG_LOCAL_NAMES

#define DEFINE_MATHML_LOCAL_NAMES(name)     \
    m_mathml##name##TagName =               \
        QualifiedName(m_mathmlNamespaceURI, \
                      AtomicString::createAtomicString(starfish, #name));
    STARFISH_ENUM_MATHML_TAG_NAMES(DEFINE_MATHML_LOCAL_NAMES)
#undef DEFINE_MATHML_LOCAL_NAMES

#define DEFINE_PSEUDO_SELECTOR_LOCAL_NAMES(name, nameLower, selectorName) \
    m_##nameLower##Selector =                                             \
        AtomicString::createAtomicString(starfish, selectorName);
    STARFISH_ENUM_PSEUDO_SELECTORS(DEFINE_PSEUDO_SELECTOR_LOCAL_NAMES)
#undef DEFINE_PSEUDO_SELECTOR_LOCAL_NAMES

    m_id = QualifiedName(AtomicString::createAtomicString(starfish, "id"));
    m_name = QualifiedName(AtomicString::createAtomicString(starfish, "name"));
    m_class =
        QualifiedName(AtomicString::createAtomicString(starfish, "class"));
    m_localName =
        QualifiedName(AtomicString::createAtomicString(starfish, "localName"));
    m_style =
        QualifiedName(AtomicString::createAtomicString(starfish, "style"));
    m_src = QualifiedName(AtomicString::createAtomicString(starfish, "src"));
    m_srcdoc =
        QualifiedName(AtomicString::createAtomicString(starfish, "srcdoc"));
    m_width =
        QualifiedName(AtomicString::createAtomicString(starfish, "width"));
    m_height =
        QualifiedName(AtomicString::createAtomicString(starfish, "height"));
    m_rel = QualifiedName(AtomicString::createAtomicString(starfish, "rel"));
    m_rev = QualifiedName(AtomicString::createAtomicString(starfish, "rev"));
    m_href = QualifiedName(AtomicString::createAtomicString(starfish, "href"));
    m_hreflang =
        QualifiedName(AtomicString::createAtomicString(starfish, "hreflang"));
    m_type = QualifiedName(AtomicString::createAtomicString(starfish, "type"));
    m_dir = QualifiedName(AtomicString::createAtomicString(starfish, "dir"));
    m_title =
        QualifiedName(AtomicString::createAtomicString(starfish, "title"));
    m_align =
        QualifiedName(AtomicString::createAtomicString(starfish, "align"));
    m_disabled =
        QualifiedName(AtomicString::createAtomicString(starfish, "disabled"));
    m_color =
        QualifiedName(AtomicString::createAtomicString(starfish, "color"));
    m_face = QualifiedName(AtomicString::createAtomicString(starfish, "face"));
    m_size = QualifiedName(AtomicString::createAtomicString(starfish, "size"));
    m_charset =
        QualifiedName(AtomicString::createAtomicString(starfish, "charset"));
    m_content =
        QualifiedName(AtomicString::createAtomicString(starfish, "content"));
    m_for = QualifiedName(AtomicString::createAtomicString(starfish, "for"));
    m_httpEquiv =
        QualifiedName(AtomicString::createAtomicString(starfish, "http-equiv"));
    m_contentLanguage = QualifiedName(
        AtomicString::createAtomicString(starfish, "content-language"));
    m_contentEditable = QualifiedName(
        AtomicString::createAtomicString(starfish, "contentEditable"));
    m_designMode =
        QualifiedName(AtomicString::createAtomicString(starfish, "designMode"));
    m_lang = QualifiedName(AtomicString::createAtomicString(starfish, "lang"));
    m_cols = QualifiedName(AtomicString::createAtomicString(starfish, "cols"));
    m_colspan =
        QualifiedName(AtomicString::createAtomicString(starfish, "colspan"));
    m_coords =
        QualifiedName(AtomicString::createAtomicString(starfish, "coords"));
    m_cellspacing = QualifiedName(
        AtomicString::createAtomicString(starfish, "cellspacing"));
    m_cellpadding = QualifiedName(
        AtomicString::createAtomicString(starfish, "cellpadding"));
    m_char = QualifiedName(AtomicString::createAtomicString(starfish, "char"));
    m_rows = QualifiedName(AtomicString::createAtomicString(starfish, "rows"));
    m_rowspan =
        QualifiedName(AtomicString::createAtomicString(starfish, "rowspan"));
    m_bgcolor =
        QualifiedName(AtomicString::createAtomicString(starfish, "bgcolor"));
    m_shape =
        QualifiedName(AtomicString::createAtomicString(starfish, "shape"));
    m_span = QualifiedName(AtomicString::createAtomicString(starfish, "span"));
    m_scope =
        QualifiedName(AtomicString::createAtomicString(starfish, "scope"));
    m_media =
        QualifiedName(AtomicString::createAtomicString(starfish, "media"));
    m_value =
        QualifiedName(AtomicString::createAtomicString(starfish, "value"));
    m_defaultValue = QualifiedName(
        AtomicString::createAtomicString(starfish, "defaultValue"));
    m_max = QualifiedName(AtomicString::createAtomicString(starfish, "max"));
    m_maxlength =
        QualifiedName(AtomicString::createAtomicString(starfish, "maxlength"));
    m_min = QualifiedName(AtomicString::createAtomicString(starfish, "min"));
    m_minlength =
        QualifiedName(AtomicString::createAtomicString(starfish, "minlength"));
    m_step = QualifiedName(AtomicString::createAtomicString(starfish, "step"));
    m_tabindex =
        QualifiedName(AtomicString::createAtomicString(starfish, "tabindex"));
    m_formAction =
        QualifiedName(AtomicString::createAtomicString(starfish, "formAction"));
    m_action =
        QualifiedName(AtomicString::createAtomicString(starfish, "action"));
    m_enctype =
        QualifiedName(AtomicString::createAtomicString(starfish, "enctype"));
    m_method =
        QualifiedName(AtomicString::createAtomicString(starfish, "method"));
    m_target =
        QualifiedName(AtomicString::createAtomicString(starfish, "target"));
    m_formEnctype = QualifiedName(
        AtomicString::createAtomicString(starfish, "formyEnctype"));
    m_formMethod =
        QualifiedName(AtomicString::createAtomicString(starfish, "formMethod"));
    m_formTarget =
        QualifiedName(AtomicString::createAtomicString(starfish, "formTarget"));
    m_checked =
        QualifiedName(AtomicString::createAtomicString(starfish, "checked"));
    m_checkbox =
        QualifiedName(AtomicString::createAtomicString(starfish, "checkbox"));
    m_radio =
        QualifiedName(AtomicString::createAtomicString(starfish, "radio"));
    m_text = QualifiedName(AtomicString::createAtomicString(starfish, "text"));
    m_textlength =
        QualifiedName(AtomicString::createAtomicString(starfish, "textlength"));
    m_placeholder = QualifiedName(
        AtomicString::createAtomicString(starfish, "placeholder"));
    m_selected =
        QualifiedName(AtomicString::createAtomicString(starfish, "selected"));
    m_multiple =
        QualifiedName(AtomicString::createAtomicString(starfish, "multiple"));
    m_required =
        QualifiedName(AtomicString::createAtomicString(starfish, "required"));
    m_reversed =
        QualifiedName(AtomicString::createAtomicString(starfish, "reversed"));
    m_alt = QualifiedName(AtomicString::createAtomicString(starfish, "alt"));
    m_scrolling =
        QualifiedName(AtomicString::createAtomicString(starfish, "scrolling"));
    m_frameborder = QualifiedName(
        AtomicString::createAtomicString(starfish, "frameborder"));
    m_autofocus =
        QualifiedName(AtomicString::createAtomicString(starfish, "autofocus"));
    m_async =
        QualifiedName(AtomicString::createAtomicString(starfish, "async"));
    m_defer =
        QualifiedName(AtomicString::createAtomicString(starfish, "defer"));
    m_compact =
        QualifiedName(AtomicString::createAtomicString(starfish, "compact"));
    m_cite = QualifiedName(AtomicString::createAtomicString(starfish, "cite"));
    m_datetime =
        QualifiedName(AtomicString::createAtomicString(starfish, "datetime"));
    m_valuetype =
        QualifiedName(AtomicString::createAtomicString(starfish, "valuetype"));
    m_nohref =
        QualifiedName(AtomicString::createAtomicString(starfish, "nohref"));
    m_download =
        QualifiedName(AtomicString::createAtomicString(starfish, "download"));
    m_ping = QualifiedName(AtomicString::createAtomicString(starfish, "ping"));
    m_usemap =
        QualifiedName(AtomicString::createAtomicString(starfish, "useMap"));
    m_charoff =
        QualifiedName(AtomicString::createAtomicString(starfish, "charoff"));
    m_valign =
        QualifiedName(AtomicString::createAtomicString(starfish, "valign"));
    m_link = QualifiedName(AtomicString::createAtomicString(starfish, "link"));
    m_vlink =
        QualifiedName(AtomicString::createAtomicString(starfish, "vlink"));
    m_alink =
        QualifiedName(AtomicString::createAtomicString(starfish, "alink"));
    m_background =
        QualifiedName(AtomicString::createAtomicString(starfish, "background"));
    m_headers =
        QualifiedName(AtomicString::createAtomicString(starfish, "headers"));
    m_abbr = QualifiedName(AtomicString::createAtomicString(starfish, "abbr"));
    m_nowrap =
        QualifiedName(AtomicString::createAtomicString(starfish, "nowrap"));
    m_axis = QualifiedName(AtomicString::createAtomicString(starfish, "axis"));
    m_tbodies =
        QualifiedName(AtomicString::createAtomicString(starfish, "bodies"));
    m_border =
        QualifiedName(AtomicString::createAtomicString(starfish, "border"));
    m_bordercolor = QualifiedName(
        AtomicString::createAtomicString(starfish, "bordercolor"));
    m_frame =
        QualifiedName(AtomicString::createAtomicString(starfish, "frame"));
    m_rules =
        QualifiedName(AtomicString::createAtomicString(starfish, "rules"));
    m_summary =
        QualifiedName(AtomicString::createAtomicString(starfish, "summary"));
    m_referrerpolicy = QualifiedName(
        AtomicString::createAtomicString(starfish, "referrerpolicy"));
    m_event =
        QualifiedName(AtomicString::createAtomicString(starfish, "event"));
    m_nomodule =
        QualifiedName(AtomicString::createAtomicString(starfish, "nomodule"));
    m_noshade =
        QualifiedName(AtomicString::createAtomicString(starfish, "noshade"));

    m_longdesc =
        QualifiedName(AtomicString::createAtomicString(starfish, "longdesc"));

    m_hspace =
        QualifiedName(AtomicString::createAtomicString(starfish, "hspace"));

    m_vspace =
        QualifiedName(AtomicString::createAtomicString(starfish, "vspace"));

    m_version =
        QualifiedName(AtomicString::createAtomicString(starfish, "version"));

    m_clear =
        QualifiedName(AtomicString::createAtomicString(starfish, "clear"));

    m_archive =
        QualifiedName(AtomicString::createAtomicString(starfish, "archive"));

    m_code = QualifiedName(AtomicString::createAtomicString(starfish, "code"));

    m_codebase =
        QualifiedName(AtomicString::createAtomicString(starfish, "codebase"));

    m_codetype =
        QualifiedName(AtomicString::createAtomicString(starfish, "codetype"));

    m_standby =
        QualifiedName(AtomicString::createAtomicString(starfish, "standby"));

    m_declare =
        QualifiedName(AtomicString::createAtomicString(starfish, "declare"));

    m_securitypolicyviolation = QualifiedName(
        AtomicString::createAtomicString(starfish, "securitypolicyviolation"));

    m_nonce =
        QualifiedName(AtomicString::createAtomicString(starfish, "nonce"));

    m_crossorigin = QualifiedName(
        AtomicString::createAtomicString(starfish, "crossorigin"));

    m_leftmargin =
        QualifiedName(AtomicString::createAtomicString(starfish, "leftmargin"));
    m_rightmargin = QualifiedName(
        AtomicString::createAtomicString(starfish, "rightmargin"));
    m_topmargin =
        QualifiedName(AtomicString::createAtomicString(starfish, "topmargin"));
    m_bottommargin = QualifiedName(
        AtomicString::createAtomicString(starfish, "bottommargin"));
    m_marginwidth = QualifiedName(
        AtomicString::createAtomicString(starfish, "marginwidth"));
    m_marginheight = QualifiedName(
        AtomicString::createAtomicString(starfish, "marginheight"));

#ifdef STARFISH_ENABLE_MULTIMEDIA
    m_default =
        QualifiedName(AtomicString::createAtomicString(starfish, "default"));
    m_loop = QualifiedName(AtomicString::createAtomicString(starfish, "loop"));
    m_autoplay =
        QualifiedName(AtomicString::createAtomicString(starfish, "autoplay"));
    m_preload =
        QualifiedName(AtomicString::createAtomicString(starfish, "preload"));
    m_controls =
        QualifiedName(AtomicString::createAtomicString(starfish, "controls"));
    m_controlsList = QualifiedName(
        AtomicString::createAtomicString(starfish, "controlsList"));
    m_kind = QualifiedName(AtomicString::createAtomicString(starfish, "kind"));
    m_label =
        QualifiedName(AtomicString::createAtomicString(starfish, "label"));
    m_srclang =
        QualifiedName(AtomicString::createAtomicString(starfish, "srclang"));
#endif
    m_fill = QualifiedName(AtomicString::createAtomicString(starfish, "fill"));
    m_fillRule =
        QualifiedName(AtomicString::createAtomicString(starfish, "fill-rule"));
    m_opacity =
        QualifiedName(AtomicString::createAtomicString(starfish, "opacity"));
    m_strokeOpacity = QualifiedName(
        AtomicString::createAtomicString(starfish, "stroke-opacity"));
    m_fillOpacity = QualifiedName(
        AtomicString::createAtomicString(starfish, "fill-opacity"));
    m_stroke =
        QualifiedName(AtomicString::createAtomicString(starfish, "stroke"));
    m_strokeWidth = QualifiedName(
        AtomicString::createAtomicString(starfish, "stroke-width"));
    m_x = QualifiedName(AtomicString::createAtomicString(starfish, "x"));
    m_y = QualifiedName(AtomicString::createAtomicString(starfish, "y"));
    m_x1 = QualifiedName(AtomicString::createAtomicString(starfish, "x1"));
    m_y1 = QualifiedName(AtomicString::createAtomicString(starfish, "y1"));
    m_x2 = QualifiedName(AtomicString::createAtomicString(starfish, "x2"));
    m_y2 = QualifiedName(AtomicString::createAtomicString(starfish, "y2"));
    m_r = QualifiedName(AtomicString::createAtomicString(starfish, "r"));
    m_rx = QualifiedName(AtomicString::createAtomicString(starfish, "rx"));
    m_ry = QualifiedName(AtomicString::createAtomicString(starfish, "ry"));
    m_transform =
        QualifiedName(AtomicString::createAtomicString(starfish, "transform"));
    m_viewBox =
        QualifiedName(AtomicString::createAtomicString(starfish, "viewBox"));
    m_preserveAspectRatio = QualifiedName(
        AtomicString::emptyAtomicString(),
        AtomicString::createAtomicString(starfish, "preserveAspectRatio"));
    m_d = QualifiedName(AtomicString::createAtomicString(starfish, "d"));
    m_points =
        QualifiedName(AtomicString::createAtomicString(starfish, "points"));
    m_cx = QualifiedName(AtomicString::createAtomicString(starfish, "cx"));
    m_cy = QualifiedName(AtomicString::createAtomicString(starfish, "cy"));
    m_fontDashFamily = QualifiedName(
        AtomicString::createAtomicString(starfish, "font-family"));
    m_fontDashSize =
        QualifiedName(AtomicString::createAtomicString(starfish, "font-size"));
    m_start =
        QualifiedName(AtomicString::createAtomicString(starfish, "start"));
    m_end = QualifiedName(AtomicString::createAtomicString(starfish, "end"));
    m_pause =
        QualifiedName(AtomicString::createAtomicString(starfish, "pause"));
    m_resume =
        QualifiedName(AtomicString::createAtomicString(starfish, "resume"));
    m_mark = QualifiedName(AtomicString::createAtomicString(starfish, "mark"));
    m_boundary =
        QualifiedName(AtomicString::createAtomicString(starfish, "boundary"));
    m_voiceschanged = QualifiedName(
        AtomicString::createAtomicString(starfish, "voiceschanged"));
    m_dirname =
        QualifiedName(AtomicString::createAtomicString(starfish, "dirname"));
    m_readonly =
        QualifiedName(AtomicString::createAtomicString(starfish, "readonly"));

    m_xmlBase =
        QualifiedName(AtomicString::createAtomicString(starfish, "xml"),
                      AtomicString::emptyAtomicString(),
                      AtomicString::createAtomicString(starfish, "base"));
    m_ariaHidden = QualifiedName(
        AtomicString::createAtomicString(starfish, "aria-hidden"));
    m_ariaLabel =
        QualifiedName(AtomicString::createAtomicString(starfish, "aria-label"));
    m_ariaLabelledby = QualifiedName(
        AtomicString::createAtomicString(starfish, "aria-labelledby"));
    m_ariaDescribedby = QualifiedName(
        AtomicString::createAtomicString(starfish, "aria-describedby"));

    m_click =
        QualifiedName(AtomicString::createAtomicString(starfish, "click"));
    m_onclick =
        QualifiedName(AtomicString::createAtomicString(starfish, "onclick"));
    m_change =
        QualifiedName(AtomicString::createAtomicString(starfish, "change"));
    m_onchange =
        QualifiedName(AtomicString::createAtomicString(starfish, "onchange"));
    m_mousedown =
        QualifiedName(AtomicString::createAtomicString(starfish, "mousedown"));
    m_onmousedown = QualifiedName(
        AtomicString::createAtomicString(starfish, "onmousedown"));
    m_mousemove =
        QualifiedName(AtomicString::createAtomicString(starfish, "mousemove"));
    m_onmousemove = QualifiedName(
        AtomicString::createAtomicString(starfish, "onmousemove"));
    m_mouseout =
        QualifiedName(AtomicString::createAtomicString(starfish, "mouseout"));
    m_onmouseout =
        QualifiedName(AtomicString::createAtomicString(starfish, "onmouseout"));
    m_mouseover =
        QualifiedName(AtomicString::createAtomicString(starfish, "mouseover"));
    m_onmouseover = QualifiedName(
        AtomicString::createAtomicString(starfish, "onmouseover"));
    m_mouseenter =
        QualifiedName(AtomicString::createAtomicString(starfish, "mouseenter"));
    m_onmouseenter = QualifiedName(
        AtomicString::createAtomicString(starfish, "onmouseenter"));
    m_mouseleave =
        QualifiedName(AtomicString::createAtomicString(starfish, "mouseleave"));
    m_onmouseleave = QualifiedName(
        AtomicString::createAtomicString(starfish, "onmouseleave"));
    m_mouseup =
        QualifiedName(AtomicString::createAtomicString(starfish, "mouseup"));
    m_onmouseup =
        QualifiedName(AtomicString::createAtomicString(starfish, "onmouseup"));
    m_touchstart =
        QualifiedName(AtomicString::createAtomicString(starfish, "touchstart"));
    m_ontouchstart = QualifiedName(
        AtomicString::createAtomicString(starfish, "ontouchstart"));
    m_touchmove =
        QualifiedName(AtomicString::createAtomicString(starfish, "touchmove"));
    m_ontouchmove = QualifiedName(
        AtomicString::createAtomicString(starfish, "ontouchmove"));
    m_touchend =
        QualifiedName(AtomicString::createAtomicString(starfish, "touchend"));
    m_ontouchend =
        QualifiedName(AtomicString::createAtomicString(starfish, "ontouchend"));
    m_load = QualifiedName(AtomicString::createAtomicString(starfish, "load"));
    m_onload =
        QualifiedName(AtomicString::createAtomicString(starfish, "onload"));
    m_submit =
        QualifiedName(AtomicString::createAtomicString(starfish, "submit"));
    m_onsubmit =
        QualifiedName(AtomicString::createAtomicString(starfish, "onsubmit"));
    m_input =
        QualifiedName(AtomicString::createAtomicString(starfish, "input"));
    m_oninput =
        QualifiedName(AtomicString::createAtomicString(starfish, "oninput"));
    m_invalid =
        QualifiedName(AtomicString::createAtomicString(starfish, "invalid"));
    m_oninvalid =
        QualifiedName(AtomicString::createAtomicString(starfish, "oninvalid"));
    m_error =
        QualifiedName(AtomicString::createAtomicString(starfish, "error"));
    m_onerror =
        QualifiedName(AtomicString::createAtomicString(starfish, "onerror"));
    m_unload =
        QualifiedName(AtomicString::createAtomicString(starfish, "unload"));
    m_onunload =
        QualifiedName(AtomicString::createAtomicString(starfish, "onunload"));
    m_visibilitychange = QualifiedName(
        AtomicString::createAtomicString(starfish, "visibilitychange"));
    m_DOMContentLoaded = QualifiedName(
        AtomicString::createAtomicString(starfish, "DOMContentLoaded"));
    m_readystatechange = QualifiedName(
        AtomicString::createAtomicString(starfish, "readystatechange"));
    m_progress =
        QualifiedName(AtomicString::createAtomicString(starfish, "progress"));
    m_abort =
        QualifiedName(AtomicString::createAtomicString(starfish, "abort"));
    m_timeout =
        QualifiedName(AtomicString::createAtomicString(starfish, "timeout"));
    m_loadend =
        QualifiedName(AtomicString::createAtomicString(starfish, "loadend"));
    m_loadstart =
        QualifiedName(AtomicString::createAtomicString(starfish, "loadstart"));
    m_enter =
        QualifiedName(AtomicString::createAtomicString(starfish, "enter"));
    m_exit = QualifiedName(AtomicString::createAtomicString(starfish, "exit"));
    m_blur = QualifiedName(AtomicString::createAtomicString(starfish, "blur"));
    m_onblur =
        QualifiedName(AtomicString::createAtomicString(starfish, "onblur"));
    m_focus =
        QualifiedName(AtomicString::createAtomicString(starfish, "focus"));
    m_onfocus =
        QualifiedName(AtomicString::createAtomicString(starfish, "onfocus"));
    m_focusin =
        QualifiedName(AtomicString::createAtomicString(starfish, "focusin"));
    m_onfocusin =
        QualifiedName(AtomicString::createAtomicString(starfish, "onfocusin"));
    m_focusout =
        QualifiedName(AtomicString::createAtomicString(starfish, "focusout"));
    m_onfocusout =
        QualifiedName(AtomicString::createAtomicString(starfish, "onfocusout"));
    m_scroll =
        QualifiedName(AtomicString::createAtomicString(starfish, "scroll"));
    m_onscroll =
        QualifiedName(AtomicString::createAtomicString(starfish, "onscroll"));
    m_clipPath =
        QualifiedName(AtomicString::createAtomicString(starfish, "clip-path"));
    m_reset =
        QualifiedName(AtomicString::createAtomicString(starfish, "reset"));
    m_resize =
        QualifiedName(AtomicString::createAtomicString(starfish, "resize"));
    m_onresize =
        QualifiedName(AtomicString::createAtomicString(starfish, "onresize"));
    m_message =
        QualifiedName(AtomicString::createAtomicString(starfish, "message"));
    m_onmessage =
        QualifiedName(AtomicString::createAtomicString(starfish, "onmessage"));
    m_messageerror = QualifiedName(
        AtomicString::createAtomicString(starfish, "messageerror"));
    m_onmessageerror = QualifiedName(
        AtomicString::createAtomicString(starfish, "onmessageerror"));
    m_animationstart = QualifiedName(
        AtomicString::createAtomicString(starfish, "animationstart"));
    m_animationend = QualifiedName(
        AtomicString::createAtomicString(starfish, "animationend"));
    m_animationcancel = QualifiedName(
        AtomicString::createAtomicString(starfish, "animationcancel"));
    m_transitionstart = QualifiedName(
        AtomicString::createAtomicString(starfish, "transitionstart"));
    m_transitionend = QualifiedName(
        AtomicString::createAtomicString(starfish, "transitionend"));
    m_transitioncancel = QualifiedName(
        AtomicString::createAtomicString(starfish, "transitioncancel"));

    m_open = QualifiedName(AtomicString::createAtomicString(starfish, "open"));
#ifdef STARFISH_ENABLE_SERVICE_WORKER
    m_statechange = QualifiedName(
        AtomicString::createAtomicString(starfish, "statechange"));
#endif

    m_ttsstart =
        QualifiedName(AtomicString::createAtomicString(starfish, "ttsstart"));
    m_onttsstart =
        QualifiedName(AtomicString::createAtomicString(starfish, "onttsstart"));
    m_ttsend =
        QualifiedName(AtomicString::createAtomicString(starfish, "ttsend"));
    m_onttsend =
        QualifiedName(AtomicString::createAtomicString(starfish, "onttsend"));

#ifdef STARFISH_ENABLE_MULTIMEDIA
    m_cuechange =
        QualifiedName(AtomicString::createAtomicString(starfish, "cuechange"));
    m_sourceopen =
        QualifiedName(AtomicString::createAtomicString(starfish, "sourceopen"));
    m_sourceended = QualifiedName(
        AtomicString::createAtomicString(starfish, "sourceended"));
    m_sourceclose = QualifiedName(
        AtomicString::createAtomicString(starfish, "sourceclose"));
    m_addsourcebuffer = QualifiedName(
        AtomicString::createAtomicString(starfish, "addsourcebuffer"));
    m_removesourcebuffer = QualifiedName(
        AtomicString::emptyAtomicString(),
        AtomicString::createAtomicString(starfish, "removesourcebuffer"));
    m_updatestart = QualifiedName(
        AtomicString::createAtomicString(starfish, "updatestart"));
    m_update =
        QualifiedName(AtomicString::createAtomicString(starfish, "update"));
    m_updateend =
        QualifiedName(AtomicString::createAtomicString(starfish, "updateend"));
    m_suspend =
        QualifiedName(AtomicString::createAtomicString(starfish, "suspend"));
    m_emptied =
        QualifiedName(AtomicString::createAtomicString(starfish, "emptied"));
    m_stalled =
        QualifiedName(AtomicString::createAtomicString(starfish, "stalled"));
    m_loadedmetadata = QualifiedName(
        AtomicString::createAtomicString(starfish, "loadedmetadata"));
    m_loadeddata =
        QualifiedName(AtomicString::createAtomicString(starfish, "loadeddata"));
    m_onloadeddata = QualifiedName(
        AtomicString::createAtomicString(starfish, "onloadeddata"));
    m_canplay =
        QualifiedName(AtomicString::createAtomicString(starfish, "canplay"));
    m_canplaythrough = QualifiedName(
        AtomicString::createAtomicString(starfish, "canplaythrough"));
    m_playing =
        QualifiedName(AtomicString::createAtomicString(starfish, "playing"));
    m_waiting =
        QualifiedName(AtomicString::createAtomicString(starfish, "waiting"));
    m_seeking =
        QualifiedName(AtomicString::createAtomicString(starfish, "seeking"));
    m_seeked =
        QualifiedName(AtomicString::createAtomicString(starfish, "seeked"));
    m_ended =
        QualifiedName(AtomicString::createAtomicString(starfish, "ended"));
    m_closed =
        QualifiedName(AtomicString::createAtomicString(starfish, "closed"));
    m_durationchange = QualifiedName(
        AtomicString::createAtomicString(starfish, "durationchange"));
    m_timeupdate =
        QualifiedName(AtomicString::createAtomicString(starfish, "timeupdate"));
    m_play = QualifiedName(AtomicString::createAtomicString(starfish, "play"));
    m_ratechange =
        QualifiedName(AtomicString::createAtomicString(starfish, "ratechange"));
    m_volumechange = QualifiedName(
        AtomicString::createAtomicString(starfish, "volumechange"));
#endif
    m_keydown =
        QualifiedName(AtomicString::createAtomicString(starfish, "keydown"));
    m_keypress =
        QualifiedName(AtomicString::createAtomicString(starfish, "keypress"));
    m_keyup =
        QualifiedName(AtomicString::createAtomicString(starfish, "keyup"));
    m_onkeydown =
        QualifiedName(AtomicString::createAtomicString(starfish, "onkeydown"));
    m_onkeypress =
        QualifiedName(AtomicString::createAtomicString(starfish, "onkeypress"));
    m_compositionstart = QualifiedName(
        AtomicString::createAtomicString(starfish, "compositionstart"));
    m_compositionupdate = QualifiedName(
        AtomicString::emptyAtomicString(),
        AtomicString::createAtomicString(starfish, "compositionupdate"));
    m_compositionend = QualifiedName(
        AtomicString::createAtomicString(starfish, "compositionend"));

    m_xlinkType =
        QualifiedName(m_xlink, m_xlinkNamespaceURI,
                      AtomicString::createAtomicString(starfish, "type"));
    m_xlinkHref =
        QualifiedName(m_xlink, m_xlinkNamespaceURI,
                      AtomicString::createAtomicString(starfish, "href"));
    m_xlinkRole =
        QualifiedName(m_xlink, m_xlinkNamespaceURI,
                      AtomicString::createAtomicString(starfish, "role"));
    m_xlinkArcrole =
        QualifiedName(m_xlink, m_xlinkNamespaceURI,
                      AtomicString::createAtomicString(starfish, "arcrole"));
    m_xlinkTitle =
        QualifiedName(m_xlink, m_xlinkNamespaceURI,
                      AtomicString::createAtomicString(starfish, "title"));
    m_xlinkShow =
        QualifiedName(m_xlink, m_xlinkNamespaceURI,
                      AtomicString::createAtomicString(starfish, "show"));
    m_xlinkActuate =
        QualifiedName(m_xlink, m_xlinkNamespaceURI,
                      AtomicString::createAtomicString(starfish, "actuate"));
    m_xlinkLabel =
        QualifiedName(m_xlink, m_xlinkNamespaceURI,
                      AtomicString::createAtomicString(starfish, "label"));
    m_xlinkFrom =
        QualifiedName(m_xlink, m_xlinkNamespaceURI,
                      AtomicString::createAtomicString(starfish, "from"));
    m_xlinkTo = QualifiedName(m_xlink, m_xlinkNamespaceURI,
                              AtomicString::createAtomicString(starfish, "to"));
#ifdef STARFISH_ENABLE_WEBSOCKET
    m_close =
        QualifiedName(m_xlink, m_xlinkNamespaceURI,
                      AtomicString::createAtomicString(starfish, "close"));
#endif
#ifdef STARFISH_ENABLE_WEBRTC
    m_srcObject =
        QualifiedName(AtomicString::createAtomicString(starfish, "srcObject"));
    m_track =
        QualifiedName(AtomicString::createAtomicString(starfish, "track"));
    m_negotiationneeded = QualifiedName(
        AtomicString::createAtomicString(starfish, "negotiationneeded"));
    m_icecandidate = QualifiedName(
        AtomicString::createAtomicString(starfish, "icecandidate"));
    m_icecandidateerror = QualifiedName(
        AtomicString::createAtomicString(starfish, "icecandidateerror"));
    m_signalingstatechange = QualifiedName(
        AtomicString::createAtomicString(starfish, "signalingstatechange"));
    m_iceconnectionstatechange = QualifiedName(
        AtomicString::createAtomicString(starfish, "iceconnectionstatechange"));
    m_icegatheringstatechange = QualifiedName(
        AtomicString::createAtomicString(starfish, "icegatheringstatechange"));
    m_connectionstatechange = QualifiedName(
        AtomicString::createAtomicString(starfish, "connectionstatechange"));
    m_datachannel = QualifiedName(
        AtomicString::createAtomicString(starfish, "datachannel"));
#endif
}
}
