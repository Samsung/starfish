/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "StarFish.h"
#include "core/dom/DOMException.h"
#include "core/dom/HTMLAnchorElement.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/dom/HTMLBRElement.h"
#include "core/dom/HTMLDivElement.h"
#include "core/dom/HTMLDocument.h"
#include "core/dom/HTMLFormElement.h"
#include "core/dom/HTMLHeadElement.h"
#include "core/dom/HTMLHeadingElement.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/HTMLInputElement.h"
#include "core/dom/HTMLImageElement.h"
#include "core/dom/HTMLLabelElement.h"
#include "core/dom/HTMLLinkElement.h"
#include "core/dom/HTMLLIElement.h"
#include "core/dom/HTMLMetaElement.h"
#include "core/dom/HTMLObjectElement.h"
#include "core/dom/HTMLParagraphElement.h"
#include "core/dom/HTMLPreElement.h"
#include "core/dom/HTMLScriptElement.h"
#include "core/dom/HTMLSpanElement.h"
#include "core/dom/HTMLStrongElement.h"
#include "core/dom/HTMLStyleElement.h"
#include "core/dom/HTMLStyleElement.h"
#include "core/dom/HTMLTableCaptionElement.h"
#include "core/dom/HTMLTableColElement.h"
#include "core/dom/HTMLTableColGroupElement.h"
#include "core/dom/HTMLTableElement.h"
#include "core/dom/HTMLTableRowElement.h"
#include "core/dom/HTMLTBodyElement.h"
#include "core/dom/HTMLTDElement.h"
#include "core/dom/HTMLTFootElement.h"
#include "core/dom/HTMLTHeadElement.h"
#include "core/dom/HTMLTHElement.h"
#include "core/dom/HTMLTitleElement.h"
#include "core/dom/HTMLTrackElement.h"
#include "core/dom/HTMLUListElement.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/dom/HTMLUnknownElement.h"
#ifdef STARFISH_ENABLE_MULTIMEDIA
#include "core/dom/HTMLAudioElement.h"
#include "core/dom/HTMLSourceElement.h"
#include "core/dom/HTMLTrackElement.h"
#include "core/dom/HTMLVideoElement.h"
#endif

namespace StarFish {

Element* HTMLDocument::createHTMLElement(Document* document, AtomicString name)
{
    StaticStrings* str = document->starFish()->staticStrings();
    if (name == str->m_htmlTagName.localNameAtomic()) {
        return new HTMLHtmlElement(document);
    } else if (name == str->m_headTagName.localNameAtomic()) {
        return new HTMLHeadElement(document);
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
    } else if (name == str->m_strongTagName.localNameAtomic()) {
        return new HTMLStrongElement(document);
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
    }
#ifdef STARFISH_ENABLE_MULTIMEDIA
    else if (name == str->m_videoTagName.localNameAtomic()) {
        return new HTMLVideoElement(document);
    } else if (name == str->m_audioTagName.localNameAtomic()) {
        return new HTMLAudioElement(document);
    } else if (name == str->m_trackTagName.localNameAtomic()) {
        return new HTMLTrackElement(document);
    } else if (name == str->m_sourceTagName.localNameAtomic()) {
        return new HTMLSourceElement(document);
    }
#endif

    STARFISH_LOG_INFO("got unknown html element - %s\n",
                      name.string()->utf8Data());
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
