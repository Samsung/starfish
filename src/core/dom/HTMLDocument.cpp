/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "StarFish.h"
#include "core/dom/DOMException.h"
#include "core/dom/HTMLAnchorElement.h"
#include "core/dom/HTMLBaseElement.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/dom/HTMLBRElement.h"
#include "core/dom/HTMLButtonElement.h"
#include "core/dom/canvas/HTMLCanvasElement.h"
#include "core/dom/HTMLDivElement.h"
#include "core/dom/HTMLDocument.h"
#include "core/dom/HTMLFieldSetElement.h"
#include "core/dom/HTMLFontElement.h"
#include "core/dom/HTMLFormElement.h"
#include "core/dom/HTMLHeadElement.h"
#include "core/dom/HTMLHeadingElement.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/HTMLInputElement.h"
#include "core/dom/HTMLImageElement.h"
#include "core/dom/HTMLLabelElement.h"
#include "core/dom/HTMLLegendElement.h"
#include "core/dom/HTMLLinkElement.h"
#include "core/dom/HTMLLIElement.h"
#include "core/dom/HTMLMetaElement.h"
#include "core/dom/HTMLObjectElement.h"
#include "core/dom/HTMLOptGroupElement.h"
#include "core/dom/HTMLOptionElement.h"
#include "core/dom/HTMLParagraphElement.h"
#include "core/dom/HTMLHRElement.h"
#include "core/dom/HTMLPreElement.h"
#include "core/dom/HTMLScriptElement.h"
#include "core/dom/HTMLSelectElement.h"
#include "core/dom/HTMLSpanElement.h"
#include "core/dom/HTMLStyleElement.h"
#include "core/dom/HTMLTableCaptionElement.h"
#include "core/dom/HTMLTableColElement.h"
#include "core/dom/HTMLTableColGroupElement.h"
#include "core/dom/HTMLTableElement.h"
#include "core/dom/HTMLTableRowElement.h"
#include "core/dom/HTMLTBodyElement.h"
#include "core/dom/HTMLTDElement.h"
#include "core/dom/HTMLTextAreaElement.h"
#include "core/dom/HTMLTFootElement.h"
#include "core/dom/HTMLTHeadElement.h"
#include "core/dom/HTMLTHElement.h"
#include "core/dom/HTMLTitleElement.h"
#include "core/dom/HTMLTrackElement.h"
#include "core/dom/HTMLUListElement.h"
#include "core/dom/HTMLOListElement.h"
#include "core/dom/HTMLDListElement.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/dom/HTMLQuoteElement.h"
#include "core/dom/HTMLModElement.h"
#include "core/dom/HTMLUnknownElement.h"
#ifdef STARFISH_ENABLE_MULTIMEDIA
#include "core/dom/HTMLAudioElement.h"
#include "core/dom/HTMLSourceElement.h"
#include "core/dom/HTMLTrackElement.h"
#include "core/dom/HTMLVideoElement.h"
#endif

namespace StarFish {

void* HTMLDocument::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLDocument)] = { 0 };
        Document::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLDocument));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

Element* HTMLDocument::createHTMLElement(Document* document, AtomicString name)
{
    StaticStrings* str = document->starFish()->staticStrings();
    if (name == str->m_htmlTagName.localNameAtomic()) {
        return new HTMLHtmlElement(document);
    } else if (name == str->m_headTagName.localNameAtomic()) {
        return new HTMLHeadElement(document);
    } else if (name == str->m_baseTagName.localNameAtomic()) {
        return new HTMLBaseElement(document);
    } else if (name == str->m_styleTagName.localNameAtomic()) {
        return new HTMLStyleElement(document);
    } else if (name == str->m_scriptTagName.localNameAtomic()) {
        return new HTMLScriptElement(document);
    } else if (name == str->m_linkTagName.localNameAtomic()) {
        return new HTMLLinkElement(document);
    } else if (name == str->m_metaTagName.localNameAtomic()) {
        return new HTMLMetaElement(document);
    } else if (name == str->m_bodyTagName.localNameAtomic()) {
        return new HTMLBodyElement(document);
    } else if (name == str->m_divTagName.localNameAtomic()) {
        return new HTMLDivElement(document);
    } else if (name == str->m_pTagName.localNameAtomic()) {
        return new HTMLParagraphElement(document);
    } else if (name == str->m_preTagName.localNameAtomic()) {
        return new HTMLPreElement(document);
    } else if (name == str->m_hrTagName.localNameAtomic()) {
        return new HTMLHRElement(document);
    } else if (name == str->m_spanTagName.localNameAtomic()) {
        return new HTMLSpanElement(document);
    } else if (name == str->m_brTagName.localNameAtomic()) {
        return new HTMLBRElement(document);
    } else if (name == str->m_objectTagName.localNameAtomic()) {
        return new HTMLObjectElement(document);
    } else if (name == str->m_imgTagName.localNameAtomic()) {
        return new HTMLImageElement(document);
    } else if (name == str->m_h1TagName.localNameAtomic() ||
               name == str->m_h2TagName.localNameAtomic() ||
               name == str->m_h3TagName.localNameAtomic() ||
               name == str->m_h4TagName.localNameAtomic() ||
               name == str->m_h5TagName.localNameAtomic() ||
               name == str->m_h6TagName.localNameAtomic()) {
        return new HTMLHeadingElement(document, name);
    } else if (name == str->m_liTagName.localNameAtomic()) {
        return new HTMLLIElement(document);
    } else if (name == str->m_ulTagName.localNameAtomic()) {
        return new HTMLUListElement(document);
    } else if (name == str->m_olTagName.localNameAtomic()) {
        return new HTMLOListElement(document);
    } else if (name == str->m_dlTagName.localNameAtomic()) {
        return new HTMLDListElement(document);
    } else if (name == str->m_tableTagName.localNameAtomic()) {
        return new HTMLTableElement(document);
    } else if (name == str->m_thTagName.localNameAtomic()) {
        return new HTMLTHElement(document);
    } else if (name == str->m_trTagName.localNameAtomic()) {
        return new HTMLTableRowElement(document);
    } else if (name == str->m_tdTagName.localNameAtomic()) {
        return new HTMLTDElement(document);
    } else if (name == str->m_captionTagName.localNameAtomic()) {
        return new HTMLTableCaptionElement(document);
    } else if (name == str->m_colgroupTagName.localNameAtomic()) {
        return new HTMLTableColGroupElement(document);
    } else if (name == str->m_colTagName.localNameAtomic()) {
        return new HTMLTableColElement(document);
    } else if (name == str->m_theadTagName.localNameAtomic()) {
        return new HTMLTHeadElement(document);
    } else if (name == str->m_tbodyTagName.localNameAtomic()) {
        return new HTMLTBodyElement(document);
    } else if (name == str->m_tfootTagName.localNameAtomic()) {
        return new HTMLTFootElement(document);
    } else if (name == str->m_iframeTagName.localNameAtomic()) {
        return new HTMLIFrameElement(document);
    } else if (name == str->m_aTagName.localNameAtomic()) {
        return new HTMLAnchorElement(document);
    } else if (name == str->m_formTagName.localNameAtomic()) {
        return new HTMLFormElement(document);
    } else if (name == str->m_inputTagName.localNameAtomic()) {
        return new HTMLInputElement(document);
    } else if (name == str->m_labelTagName.localNameAtomic()) {
        return new HTMLLabelElement(document);
    } else if (name == str->m_titleTagName.localNameAtomic()) {
        return new HTMLTitleElement(document);
    } else if (name == str->m_fontTagName.localNameAtomic()) {
        return new HTMLFontElement(document);
    } else if (name == str->m_fieldsetTagName.localNameAtomic()) {
        return new HTMLFieldSetElement(document);
    } else if (name == str->m_legendTagName.localNameAtomic()) {
        return new HTMLLegendElement(document);
    } else if (name == str->m_buttonTagName.localNameAtomic()) {
        return new HTMLButtonElement(document);
    } else if (name == str->m_selectTagName.localNameAtomic()) {
        return new HTMLSelectElement(document);
    } else if (name == str->m_textareaTagName.localNameAtomic()) {
        return new HTMLTextAreaElement(document);
    } else if (name == str->m_optionTagName.localNameAtomic()) {
        return new HTMLOptionElement(document);
    } else if (name == str->m_optgroupTagName.localNameAtomic()) {
        return new HTMLOptGroupElement(document);
    } else if (name == str->m_qTagName.localNameAtomic() ||
               name == str->m_blockquoteTagName.localNameAtomic()) {
        return new HTMLQuoteElement(document, name);
    } else if (name == str->m_delTagName.localNameAtomic() ||
               name == str->m_insTagName.localNameAtomic()) {
        return new HTMLModElement(document, name);
    }
#define DEFINE_KNOWN_ELEMENT(tagName)                               \
    else if (name == str->m_##tagName##TagName.localNameAtomic())   \
    {                                                               \
        return new HTMLKnownElement(                                \
            document, str->m_##tagName##TagName.localNameAtomic()); \
    }
    DEFINE_KNOWN_ELEMENT(center)
    DEFINE_KNOWN_ELEMENT(i)
    DEFINE_KNOWN_ELEMENT(b)
    DEFINE_KNOWN_ELEMENT(strong)
    DEFINE_KNOWN_ELEMENT(cite)
    DEFINE_KNOWN_ELEMENT(em)
    DEFINE_KNOWN_ELEMENT(var)
    DEFINE_KNOWN_ELEMENT(address)
    DEFINE_KNOWN_ELEMENT(article)
    DEFINE_KNOWN_ELEMENT(aside)
    DEFINE_KNOWN_ELEMENT(details)
    DEFINE_KNOWN_ELEMENT(footer)
    DEFINE_KNOWN_ELEMENT(header)
    DEFINE_KNOWN_ELEMENT(hgroup)
    DEFINE_KNOWN_ELEMENT(main)
    DEFINE_KNOWN_ELEMENT(nav)
    DEFINE_KNOWN_ELEMENT(section)
    DEFINE_KNOWN_ELEMENT(summary)
    DEFINE_KNOWN_ELEMENT(code)
    DEFINE_KNOWN_ELEMENT(dt)
    DEFINE_KNOWN_ELEMENT(dd)
#ifdef STARFISH_ENABLE_MULTIMEDIA
    else if (name == str->m_videoTagName.localNameAtomic())
    {
        return new HTMLVideoElement(document);
    }
    else if (name == str->m_audioTagName.localNameAtomic())
    {
        return new HTMLAudioElement(document);
    }
    else if (name == str->m_trackTagName.localNameAtomic())
    {
        return new HTMLTrackElement(document);
    }
    else if (name == str->m_sourceTagName.localNameAtomic())
    {
        return new HTMLSourceElement(document);
    }
#endif
#ifdef STARFISH_ENABLE_CANVAS
    else if (name == str->m_canvasTagName.localNameAtomic())
    {
        return new HTMLCanvasElement(document);
    }
#endif

    auto s = name.string()->toUTF8NonGCString();
    STARFISH_LOG_INFO("HTMLDocument: invalid (or unsupported) element: %s\n",
                      s.data());
    return new HTMLUnknownElement(document, name);
}

static void createHtmlCaseInsensitiveAttributesSet(
    Document* document, GCUnorderedMap<String*, size_t>& attrSet)
{
    // This is the list of attributes in HTML 4.01 with values marked as "[CI]"
    // or case-insensitive
    StaticStrings* str = document->starFish()->staticStrings();

    const QualifiedName* caseInsesitiveAttributes[] = {
        /* &accept_charsetAttr, &acceptAttr, &alignAttr, &alinkAttr, &axisAttr,
        &bgcolorAttr, */
        &str->m_charset,
        /* &checkedAttr, &clearAttr, &codetypeAttr, */ &str->m_color,
        /* &compactAttr,
        &declareAttr, &deferAttr, */ &str->m_dir, /* &directionAttr, */
        &str->m_disabled,
        /* &enctypeAttr, */
        &str->m_face, /* &frameAttr,
        &hreflangAttr, &http_equivAttr, */
        &str->m_lang, /* &languageAttr, &linkAttr,
        &mediaAttr, &methodAttr, &multipleAttr,
        &nohrefAttr, &noresizeAttr, &noshadeAttr, &nowrapAttr,
        &readonlyAttr, */ &str->m_rel,
        /* &revAttr, &rulesAttr,*/
        &str->m_scope,
        /*&scrollingAttr, &selectedAttr, &shapeAttr,
        &targetAttr, &textAttr, */ &str->m_type,
        /* &valignAttr, &valuetypeAttr, &vlinkAttr */
    };

    for (const QualifiedName* attr : caseInsesitiveAttributes) {
        attrSet.insert(std::make_pair(attr->localName(), 1));
    }
}

bool HTMLDocument::isCaseSensitiveAttribute(Document* document,
                                            const QualifiedName& attributeName)
{
    GCUnorderedMap<String*, size_t>& caseInsensitiveAttrSet =
        document->starFish()->m_caseInsensitiveAttrSet;

    if (caseInsensitiveAttrSet.size() == 0)
        createHtmlCaseInsensitiveAttributesSet(document,
                                               caseInsensitiveAttrSet);

    return caseInsensitiveAttrSet.find(attributeName.localName()) ==
           caseInsensitiveAttrSet.end();
}
}
