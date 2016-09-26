/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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
#include "HTMLDocument.h"

#include "DOM.h"

namespace StarFish {

Element* HTMLDocument::createHTMLElement(Document* document, AtomicString name)
{
    StaticStrings* str = document->window()->starFish()->staticStrings();
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
    } else if (name == str->m_spanTagName.localNameAtomic()) {
        return new HTMLSpanElement(document);
    } else if (name == str->m_brTagName.localNameAtomic()) {
        return new HTMLBRElement(document);
    } else if (name == str->m_imgTagName.localNameAtomic()) {
        return new HTMLImageElement(document);
    }
#ifdef STARFISH_ENABLE_MULTIMEDIA
    else if (name == str->m_videoTagName.localNameAtomic()) {
        return new HTMLVideoElement(document);
    } else if (name == str->m_audioTagName.localNameAtomic()) {
        return new HTMLAudioElement(document);
    } else if (name == str->m_trackTagName.localNameAtomic()) {
        return new HTMLTrackElement(document);
    }
#endif
#ifdef STARFISH_ENABLE_MULTI_PAGE
    else if (name == str->m_aTagName.localNameAtomic()) {
        return new HTMLAnchorElement(document);
    }
#endif
    STARFISH_LOG_INFO("got unknown html element - %s\n", name.string()->utf8Data());
    return new HTMLUnknownElement(document, name);
}

Element* HTMLDocument::createElement(AtomicString localName, bool shouldCheckName)
{
    if (shouldCheckName && !QualifiedName::checkNameProductionRule(localName.string(), localName.string()->length()))
        throw new DOMException(document()->scriptBindingInstance(), DOMException::Code::INVALID_CHARACTER_ERR, nullptr);

#ifdef STARFISH_TC_COVERAGE
    if (localName.localName()->equals("style")) {
        STARFISH_LOG_INFO("+++tag:Element&&&style\n");
    } else {
        STARFISH_LOG_INFO("+++tag:%s\n", localName.localName()->utf8Data());
    }
#endif

    return HTMLDocument::createHTMLElement(this, localName);
}

}
