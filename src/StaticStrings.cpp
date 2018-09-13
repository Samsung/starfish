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

    m_id = QualifiedName(AtomicString::createAtomicString(sf, "id"));
    m_name = QualifiedName(AtomicString::createAtomicString(sf, "name"));
    m_class = QualifiedName(AtomicString::createAtomicString(sf, "class"));
    m_localName =
        QualifiedName(AtomicString::createAtomicString(sf, "localName"));
    m_style = QualifiedName(AtomicString::createAtomicString(sf, "style"));
    m_src = QualifiedName(AtomicString::createAtomicString(sf, "src"));
    m_width = QualifiedName(AtomicString::createAtomicString(sf, "width"));
    m_height = QualifiedName(AtomicString::createAtomicString(sf, "height"));
    m_rel = QualifiedName(AtomicString::createAtomicString(sf, "rel"));
    m_rev = QualifiedName(AtomicString::createAtomicString(sf, "rev"));
    m_href = QualifiedName(AtomicString::createAtomicString(sf, "href"));
    m_hreflang =
        QualifiedName(AtomicString::createAtomicString(sf, "hreflang"));
    m_type = QualifiedName(AtomicString::createAtomicString(sf, "type"));
    m_dir = QualifiedName(AtomicString::createAtomicString(sf, "dir"));
    m_title = QualifiedName(AtomicString::createAtomicString(sf, "title"));
    m_align = QualifiedName(AtomicString::createAtomicString(sf, "align"));
    m_disabled =
        QualifiedName(AtomicString::createAtomicString(sf, "disabled"));
    m_color = QualifiedName(AtomicString::createAtomicString(sf, "color"));
    m_face = QualifiedName(AtomicString::createAtomicString(sf, "face"));
    m_size = QualifiedName(AtomicString::createAtomicString(sf, "size"));
    m_charset = QualifiedName(AtomicString::createAtomicString(sf, "charset"));
    m_content = QualifiedName(AtomicString::createAtomicString(sf, "content"));
    m_for = QualifiedName(AtomicString::createAtomicString(sf, "for"));
    m_httpEquiv =
        QualifiedName(AtomicString::createAtomicString(sf, "http-equiv"));
    m_contentLanguage =
        QualifiedName(AtomicString::createAtomicString(sf, "content-language"));
    m_contentEditable =
        QualifiedName(AtomicString::createAtomicString(sf, "contentEditable"));
    m_designMode =
        QualifiedName(AtomicString::createAtomicString(sf, "designMode"));
    m_lang = QualifiedName(AtomicString::createAtomicString(sf, "lang"));
    m_cols = QualifiedName(AtomicString::createAtomicString(sf, "cols"));
    m_colspan = QualifiedName(AtomicString::createAtomicString(sf, "colspan"));
    m_coords = QualifiedName(AtomicString::createAtomicString(sf, "coords"));
    m_cellspacing =
        QualifiedName(AtomicString::createAtomicString(sf, "cellspacing"));
    m_cellpadding =
        QualifiedName(AtomicString::createAtomicString(sf, "cellpadding"));
    m_char = QualifiedName(AtomicString::createAtomicString(sf, "char"));
    m_rows = QualifiedName(AtomicString::createAtomicString(sf, "rows"));
    m_rowspan = QualifiedName(AtomicString::createAtomicString(sf, "rowspan"));
    m_bgcolor = QualifiedName(AtomicString::createAtomicString(sf, "bgcolor"));
    m_shape = QualifiedName(AtomicString::createAtomicString(sf, "shape"));
    m_span = QualifiedName(AtomicString::createAtomicString(sf, "span"));
    m_scope = QualifiedName(AtomicString::createAtomicString(sf, "scope"));
    m_media = QualifiedName(AtomicString::createAtomicString(sf, "media"));
    m_value = QualifiedName(AtomicString::createAtomicString(sf, "value"));
    m_defaultValue =
        QualifiedName(AtomicString::createAtomicString(sf, "defaultValue"));
    m_max = QualifiedName(AtomicString::createAtomicString(sf, "max"));
    m_maxlength =
        QualifiedName(AtomicString::createAtomicString(sf, "maxlength"));
    m_min = QualifiedName(AtomicString::createAtomicString(sf, "min"));
    m_minlength =
        QualifiedName(AtomicString::createAtomicString(sf, "minlength"));
    m_step = QualifiedName(AtomicString::createAtomicString(sf, "step"));
    m_tabindex =
        QualifiedName(AtomicString::createAtomicString(sf, "tabindex"));
    m_formAction =
        QualifiedName(AtomicString::createAtomicString(sf, "formAction"));
    m_action = QualifiedName(AtomicString::createAtomicString(sf, "action"));
    m_enctype = QualifiedName(AtomicString::createAtomicString(sf, "enctype"));
    m_method = QualifiedName(AtomicString::createAtomicString(sf, "method"));
    m_target = QualifiedName(AtomicString::createAtomicString(sf, "target"));
    m_formEnctype =
        QualifiedName(AtomicString::createAtomicString(sf, "formyEnctype"));
    m_formMethod =
        QualifiedName(AtomicString::createAtomicString(sf, "formMethod"));
    m_formTarget =
        QualifiedName(AtomicString::createAtomicString(sf, "formTarget"));
    m_checked = QualifiedName(AtomicString::createAtomicString(sf, "checked"));
    m_checkbox =
        QualifiedName(AtomicString::createAtomicString(sf, "checkbox"));
    m_radio = QualifiedName(AtomicString::createAtomicString(sf, "radio"));
    m_text = QualifiedName(AtomicString::createAtomicString(sf, "text"));
    m_textlength =
        QualifiedName(AtomicString::createAtomicString(sf, "textlength"));
    m_placeholder =
        QualifiedName(AtomicString::createAtomicString(sf, "placeholder"));
    m_selected =
        QualifiedName(AtomicString::createAtomicString(sf, "selected"));
    m_multiple =
        QualifiedName(AtomicString::createAtomicString(sf, "multiple"));
    m_required =
        QualifiedName(AtomicString::createAtomicString(sf, "required"));
    m_reversed =
        QualifiedName(AtomicString::createAtomicString(sf, "reversed"));
    m_alt = QualifiedName(AtomicString::createAtomicString(sf, "alt"));
    m_scrolling =
        QualifiedName(AtomicString::createAtomicString(sf, "scrolling"));
    m_frameborder =
        QualifiedName(AtomicString::createAtomicString(sf, "frameborder"));
    m_autofocus =
        QualifiedName(AtomicString::createAtomicString(sf, "autofocus"));
    m_async = QualifiedName(AtomicString::createAtomicString(sf, "async"));
    m_defer = QualifiedName(AtomicString::createAtomicString(sf, "defer"));
    m_compact = QualifiedName(AtomicString::createAtomicString(sf, "compact"));
    m_cite = QualifiedName(AtomicString::createAtomicString(sf, "cite"));
    m_datetime =
        QualifiedName(AtomicString::createAtomicString(sf, "datetime"));
    m_valuetype =
        QualifiedName(AtomicString::createAtomicString(sf, "valuetype"));
    m_nohref = QualifiedName(AtomicString::createAtomicString(sf, "nohref"));
    m_download =
        QualifiedName(AtomicString::createAtomicString(sf, "download"));
    m_ping = QualifiedName(AtomicString::createAtomicString(sf, "ping"));
    m_usemap = QualifiedName(AtomicString::createAtomicString(sf, "useMap"));
    m_charoff = QualifiedName(AtomicString::createAtomicString(sf, "charoff"));
    m_valign = QualifiedName(AtomicString::createAtomicString(sf, "valign"));
    m_link = QualifiedName(AtomicString::createAtomicString(sf, "link"));
    m_vlink = QualifiedName(AtomicString::createAtomicString(sf, "vlink"));
    m_alink = QualifiedName(AtomicString::createAtomicString(sf, "alink"));
    m_background =
        QualifiedName(AtomicString::createAtomicString(sf, "background"));
    m_headers = QualifiedName(AtomicString::createAtomicString(sf, "headers"));
    m_abbr = QualifiedName(AtomicString::createAtomicString(sf, "abbr"));
    m_nowrap = QualifiedName(AtomicString::createAtomicString(sf, "nowrap"));
    m_axis = QualifiedName(AtomicString::createAtomicString(sf, "axis"));
    m_tbodies = QualifiedName(AtomicString::createAtomicString(sf, "bodies"));
    m_border = QualifiedName(AtomicString::createAtomicString(sf, "border"));
    m_bordercolor =
        QualifiedName(AtomicString::createAtomicString(sf, "bordercolor"));
    m_frame = QualifiedName(AtomicString::createAtomicString(sf, "frame"));
    m_rules = QualifiedName(AtomicString::createAtomicString(sf, "rules"));
    m_summary = QualifiedName(AtomicString::createAtomicString(sf, "summary"));
    m_referrerpolicy =
        QualifiedName(AtomicString::createAtomicString(sf, "referrerpolicy"));
    m_event = QualifiedName(AtomicString::createAtomicString(sf, "event"));
    m_nomodule =
        QualifiedName(AtomicString::createAtomicString(sf, "nomodule"));
    m_noshade = QualifiedName(AtomicString::createAtomicString(sf, "noshade"));

    m_longdesc =
        QualifiedName(AtomicString::createAtomicString(sf, "longdesc"));

    m_hspace = QualifiedName(AtomicString::createAtomicString(sf, "hspace"));

    m_vspace = QualifiedName(AtomicString::createAtomicString(sf, "vspace"));

    m_version = QualifiedName(AtomicString::createAtomicString(sf, "version"));

    m_clear = QualifiedName(AtomicString::createAtomicString(sf, "clear"));

    m_archive = QualifiedName(AtomicString::createAtomicString(sf, "archive"));

    m_code = QualifiedName(AtomicString::createAtomicString(sf, "code"));

    m_codebase =
        QualifiedName(AtomicString::createAtomicString(sf, "codebase"));

    m_codetype =
        QualifiedName(AtomicString::createAtomicString(sf, "codetype"));

    m_standby = QualifiedName(AtomicString::createAtomicString(sf, "standby"));

    m_declare = QualifiedName(AtomicString::createAtomicString(sf, "declare"));
#ifdef STARFISH_ENABLE_MULTIMEDIA
    m_default = QualifiedName(AtomicString::createAtomicString(sf, "default"));
    m_loop = QualifiedName(AtomicString::createAtomicString(sf, "loop"));
    m_autoplay =
        QualifiedName(AtomicString::createAtomicString(sf, "autoplay"));
    m_preload = QualifiedName(AtomicString::createAtomicString(sf, "preload"));
    m_controls =
        QualifiedName(AtomicString::createAtomicString(sf, "controls"));
    m_controlsList =
        QualifiedName(AtomicString::createAtomicString(sf, "controlsList"));
    m_kind = QualifiedName(AtomicString::createAtomicString(sf, "kind"));
    m_label = QualifiedName(AtomicString::createAtomicString(sf, "label"));
    m_srclang = QualifiedName(AtomicString::createAtomicString(sf, "srclang"));
#endif
    m_fill = QualifiedName(AtomicString::createAtomicString(sf, "fill"));
    m_fillRule =
        QualifiedName(AtomicString::createAtomicString(sf, "fill-rule"));
    m_fillOpacity =
        QualifiedName(AtomicString::createAtomicString(sf, "fill-opacity"));
    m_stroke = QualifiedName(AtomicString::createAtomicString(sf, "stroke"));
    m_strokeWidth =
        QualifiedName(AtomicString::createAtomicString(sf, "stroke-width"));
    m_x = QualifiedName(AtomicString::createAtomicString(sf, "x"));
    m_y = QualifiedName(AtomicString::createAtomicString(sf, "y"));
    m_r = QualifiedName(AtomicString::createAtomicString(sf, "r"));
    m_rx = QualifiedName(AtomicString::createAtomicString(sf, "rx"));
    m_ry = QualifiedName(AtomicString::createAtomicString(sf, "ry"));
    m_transform =
        QualifiedName(AtomicString::createAtomicString(sf, "transform"));
    m_viewBox = QualifiedName(AtomicString::createAtomicString(sf, "viewBox"));
    m_preserveAspectRatio = QualifiedName(
        AtomicString::emptyAtomicString(),
        AtomicString::createAtomicString(sf, "preserveAspectRatio"));
    m_d = QualifiedName(AtomicString::createAtomicString(sf, "d"));
    m_points = QualifiedName(AtomicString::createAtomicString(sf, "points"));
    m_cx = QualifiedName(AtomicString::createAtomicString(sf, "cx"));
    m_cy = QualifiedName(AtomicString::createAtomicString(sf, "cy"));
    m_fontDashFamily =
        QualifiedName(AtomicString::createAtomicString(sf, "font-family"));
    m_fontDashSize =
        QualifiedName(AtomicString::createAtomicString(sf, "font-size"));
    m_start = QualifiedName(AtomicString::createAtomicString(sf, "start"));
    m_dirname = QualifiedName(AtomicString::createAtomicString(sf, "dirname"));
    m_readonly =
        QualifiedName(AtomicString::createAtomicString(sf, "readonly"));

    m_xmlBase = QualifiedName(AtomicString::createAtomicString(sf, "xml"),
                              AtomicString::emptyAtomicString(),
                              AtomicString::createAtomicString(sf, "base"));
    m_ariaHidden =
        QualifiedName(AtomicString::createAtomicString(sf, "aria-hidden"));
    m_ariaLabel =
        QualifiedName(AtomicString::createAtomicString(sf, "aria-label"));
    m_ariaLabelledby =
        QualifiedName(AtomicString::createAtomicString(sf, "aria-labelledby"));
    m_ariaDescribedby =
        QualifiedName(AtomicString::createAtomicString(sf, "aria-describedby"));

    m_click = QualifiedName(AtomicString::createAtomicString(sf, "click"));
    m_onclick = QualifiedName(AtomicString::createAtomicString(sf, "onclick"));
    m_change = QualifiedName(AtomicString::createAtomicString(sf, "change"));
    m_onchange =
        QualifiedName(AtomicString::createAtomicString(sf, "onchange"));
    m_mousedown =
        QualifiedName(AtomicString::createAtomicString(sf, "mousedown"));
    m_onmousedown =
        QualifiedName(AtomicString::createAtomicString(sf, "onmousedown"));
    m_mousemove =
        QualifiedName(AtomicString::createAtomicString(sf, "mousemove"));
    m_onmousemove =
        QualifiedName(AtomicString::createAtomicString(sf, "onmousemove"));
    m_mouseout =
        QualifiedName(AtomicString::createAtomicString(sf, "mouseout"));
    m_onmouseout =
        QualifiedName(AtomicString::createAtomicString(sf, "onmouseout"));
    m_mouseover =
        QualifiedName(AtomicString::createAtomicString(sf, "mouseover"));
    m_onmouseover =
        QualifiedName(AtomicString::createAtomicString(sf, "onmouseover"));
    m_mouseenter =
        QualifiedName(AtomicString::createAtomicString(sf, "mouseenter"));
    m_onmouseenter =
        QualifiedName(AtomicString::createAtomicString(sf, "onmouseenter"));
    m_mouseleave =
        QualifiedName(AtomicString::createAtomicString(sf, "mouseleave"));
    m_onmouseleave =
        QualifiedName(AtomicString::createAtomicString(sf, "onmouseleave"));
    m_mouseup = QualifiedName(AtomicString::createAtomicString(sf, "mouseup"));
    m_onmouseup =
        QualifiedName(AtomicString::createAtomicString(sf, "onmouseup"));
    m_touchstart =
        QualifiedName(AtomicString::createAtomicString(sf, "touchstart"));
    m_ontouchstart =
        QualifiedName(AtomicString::createAtomicString(sf, "ontouchstart"));
    m_touchmove =
        QualifiedName(AtomicString::createAtomicString(sf, "touchmove"));
    m_ontouchmove =
        QualifiedName(AtomicString::createAtomicString(sf, "ontouchmove"));
    m_touchend =
        QualifiedName(AtomicString::createAtomicString(sf, "touchend"));
    m_ontouchend =
        QualifiedName(AtomicString::createAtomicString(sf, "ontouchend"));
    m_load = QualifiedName(AtomicString::createAtomicString(sf, "load"));
    m_onload = QualifiedName(AtomicString::createAtomicString(sf, "onload"));
    m_submit = QualifiedName(AtomicString::createAtomicString(sf, "submit"));
    m_onsubmit =
        QualifiedName(AtomicString::createAtomicString(sf, "onsubmit"));
    m_input = QualifiedName(AtomicString::createAtomicString(sf, "input"));
    m_oninput = QualifiedName(AtomicString::createAtomicString(sf, "oninput"));
    m_invalid = QualifiedName(AtomicString::createAtomicString(sf, "invalid"));
    m_oninvalid =
        QualifiedName(AtomicString::createAtomicString(sf, "oninvalid"));
    m_error = QualifiedName(AtomicString::createAtomicString(sf, "error"));
    m_onerror = QualifiedName(AtomicString::createAtomicString(sf, "onerror"));
    m_unload = QualifiedName(AtomicString::createAtomicString(sf, "unload"));
    m_onunload =
        QualifiedName(AtomicString::createAtomicString(sf, "onunload"));
    m_visibilitychange =
        QualifiedName(AtomicString::createAtomicString(sf, "visibilitychange"));
    m_DOMContentLoaded =
        QualifiedName(AtomicString::createAtomicString(sf, "DOMContentLoaded"));
    m_readystatechange =
        QualifiedName(AtomicString::createAtomicString(sf, "readystatechange"));
    m_progress =
        QualifiedName(AtomicString::createAtomicString(sf, "progress"));
    m_abort = QualifiedName(AtomicString::createAtomicString(sf, "abort"));
    m_timeout = QualifiedName(AtomicString::createAtomicString(sf, "timeout"));
    m_loadend = QualifiedName(AtomicString::createAtomicString(sf, "loadend"));
    m_loadstart =
        QualifiedName(AtomicString::createAtomicString(sf, "loadstart"));
    m_enter = QualifiedName(AtomicString::createAtomicString(sf, "enter"));
    m_exit = QualifiedName(AtomicString::createAtomicString(sf, "exit"));
    m_blur = QualifiedName(AtomicString::createAtomicString(sf, "blur"));
    m_onblur = QualifiedName(AtomicString::createAtomicString(sf, "onblur"));
    m_focus = QualifiedName(AtomicString::createAtomicString(sf, "focus"));
    m_onfocus = QualifiedName(AtomicString::createAtomicString(sf, "onfocus"));
    m_focusin = QualifiedName(AtomicString::createAtomicString(sf, "focusin"));
    m_onfocusin =
        QualifiedName(AtomicString::createAtomicString(sf, "onfocusin"));
    m_focusout =
        QualifiedName(AtomicString::createAtomicString(sf, "focusout"));
    m_onfocusout =
        QualifiedName(AtomicString::createAtomicString(sf, "onfocusout"));

    m_reset = QualifiedName(AtomicString::createAtomicString(sf, "reset"));

    m_resize = QualifiedName(AtomicString::createAtomicString(sf, "resize"));
    m_onresize =
        QualifiedName(AtomicString::createAtomicString(sf, "onresize"));
    m_message = QualifiedName(AtomicString::createAtomicString(sf, "message"));
    m_onmessage =
        QualifiedName(AtomicString::createAtomicString(sf, "onmessage"));
    m_messageerror =
        QualifiedName(AtomicString::createAtomicString(sf, "messageerror"));
    m_onmessageerror =
        QualifiedName(AtomicString::createAtomicString(sf, "onmessageerror"));
    m_transitionstart =
        QualifiedName(AtomicString::createAtomicString(sf, "transitionstart"));
    m_transitionend =
        QualifiedName(AtomicString::createAtomicString(sf, "transitionend"));
    m_transitioncancel =
        QualifiedName(AtomicString::createAtomicString(sf, "transitioncancel"));

    m_open = QualifiedName(AtomicString::createAtomicString(sf, "open"));

#ifdef STARFISH_ENABLE_MULTIMEDIA
    m_cuechange =
        QualifiedName(AtomicString::createAtomicString(sf, "cuechange"));
    m_sourceopen =
        QualifiedName(AtomicString::createAtomicString(sf, "sourceopen"));
    m_sourceended =
        QualifiedName(AtomicString::createAtomicString(sf, "sourceended"));
    m_sourceclose =
        QualifiedName(AtomicString::createAtomicString(sf, "sourceclose"));
    m_addsourcebuffer =
        QualifiedName(AtomicString::createAtomicString(sf, "addsourcebuffer"));
    m_removesourcebuffer = QualifiedName(
        AtomicString::emptyAtomicString(),
        AtomicString::createAtomicString(sf, "removesourcebuffer"));
    m_updatestart =
        QualifiedName(AtomicString::createAtomicString(sf, "updatestart"));
    m_update = QualifiedName(AtomicString::createAtomicString(sf, "update"));
    m_updateend =
        QualifiedName(AtomicString::createAtomicString(sf, "updateend"));
    m_suspend = QualifiedName(AtomicString::createAtomicString(sf, "suspend"));
    m_emptied = QualifiedName(AtomicString::createAtomicString(sf, "emptied"));
    m_stalled = QualifiedName(AtomicString::createAtomicString(sf, "stalled"));
    m_loadedmetadata =
        QualifiedName(AtomicString::createAtomicString(sf, "loadedmetadata"));
    m_loadeddata =
        QualifiedName(AtomicString::createAtomicString(sf, "loadeddata"));
    m_canplay = QualifiedName(AtomicString::createAtomicString(sf, "canplay"));
    m_canplaythrough =
        QualifiedName(AtomicString::createAtomicString(sf, "canplaythrough"));
    m_playing = QualifiedName(AtomicString::createAtomicString(sf, "playing"));
    m_waiting = QualifiedName(AtomicString::createAtomicString(sf, "waiting"));
    m_seeking = QualifiedName(AtomicString::createAtomicString(sf, "seeking"));
    m_seeked = QualifiedName(AtomicString::createAtomicString(sf, "seeked"));
    m_ended = QualifiedName(AtomicString::createAtomicString(sf, "ended"));
    m_closed = QualifiedName(AtomicString::createAtomicString(sf, "closed"));
    m_durationchange =
        QualifiedName(AtomicString::createAtomicString(sf, "durationchange"));
    m_timeupdate =
        QualifiedName(AtomicString::createAtomicString(sf, "timeupdate"));
    m_play = QualifiedName(AtomicString::createAtomicString(sf, "play"));
    m_pause = QualifiedName(AtomicString::createAtomicString(sf, "pause"));
    m_ratechange =
        QualifiedName(AtomicString::createAtomicString(sf, "ratechange"));
    m_volumechange =
        QualifiedName(AtomicString::createAtomicString(sf, "volumechange"));
#endif
    m_keydown = QualifiedName(AtomicString::createAtomicString(sf, "keydown"));
    m_keypress =
        QualifiedName(AtomicString::createAtomicString(sf, "keypress"));
    m_keyup = QualifiedName(AtomicString::createAtomicString(sf, "keyup"));
    m_onkeydown =
        QualifiedName(AtomicString::createAtomicString(sf, "onkeydown"));
    m_onkeypress =
        QualifiedName(AtomicString::createAtomicString(sf, "onkeypress"));
    m_compositionstart =
        QualifiedName(AtomicString::createAtomicString(sf, "compositionstart"));
    m_compositionupdate = QualifiedName(
        AtomicString::emptyAtomicString(),
        AtomicString::createAtomicString(sf, "compositionupdate"));
    m_compositionend =
        QualifiedName(AtomicString::createAtomicString(sf, "compositionend"));

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
