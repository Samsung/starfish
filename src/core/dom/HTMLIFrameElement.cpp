/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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
#include "Starfish.h"
#include "core/dom/Document.h"
#include "browser/history/HistoryManager.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/dom/Event.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/csp/ContentSecurityPolicy.h"

namespace Starfish {

void* HTMLIFrameElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLIFrameElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLIFrameElement)] = { 0 };
        HTMLIFrameElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLIFrameElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

HTMLIFrameElement::HTMLIFrameElement(Document* document,
                                     const QualifiedName& qname)
    : HTMLElement(document, qname)
    , m_browsingContext(nullptr)
    , m_contentWindowProxy(nullptr)
    , m_historyManager(nullptr)
    , m_isContentDocumentDisabled(false)
{
    m_tabIndexWasSetExplicitly = true;
}

void HTMLIFrameElement::setSrc(String* src)
{
    setAttribute(starfish()->staticStrings()->m_src, src);
}

String* HTMLIFrameElement::src()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_src);
}

void HTMLIFrameElement::setSrcdoc(String* srcDoc)
{
    setAttribute(starfish()->staticStrings()->m_srcdoc, srcDoc);
}

String* HTMLIFrameElement::srcdoc()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_srcdoc);
}

uint32_t HTMLIFrameElement::frameWidth()
{
    String* v = width();
    if (v->length()) {
        return String::parseInt(v);
    }
    return STARFISH_DEFAULT_IFRAME_WIDTH;
}

uint32_t HTMLIFrameElement::frameHeight()
{
    String* v = height();
    if (v->length()) {
        return String::parseInt(v);
    }
    return STARFISH_DEFAULT_IFRAME_HEIGHT;
}

String* HTMLIFrameElement::width()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_width);
}

void HTMLIFrameElement::setWidth(String* width)
{
    setAttribute(starfish()->staticStrings()->m_width, width);
}

String* HTMLIFrameElement::height()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_height);
}

void HTMLIFrameElement::setHeight(String* height)
{
    setAttribute(starfish()->staticStrings()->m_height, height);
}

String* HTMLIFrameElement::scrolling()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_scrolling);
}

void HTMLIFrameElement::setScrolling(String* scrolling)
{
    setAttribute(starfish()->staticStrings()->m_scrolling, scrolling);
}

void HTMLIFrameElement::didAttributeChanged(QualifiedName name,
                                            Optional<String*> old,
                                            String* value,
                                            bool attributeCreated,
                                            bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    if (name == starfish()->staticStrings()->m_src) {
        if (!inHTMLConstructionSite()) {
            unloadSrc();
            if (value->length() && document()->doesParticipateInRendering()) {
                loadSrc();
            }
        }
    } else if (name == starfish()->staticStrings()->m_width ||
               name == starfish()->staticStrings()->m_height) {
        if (frame()) {
            setNeedsLayout();
        }
    } else if (name == starfish()->staticStrings()->m_tabindex) {
        m_tabIndexWasSetExplicitly = true;
    } else if (name == starfish()->staticStrings()->m_frameborder) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
    } else if (name == starfish()->staticStrings()->m_name) {
        if (m_browsingContext) {
            m_browsingContext->setName(value);
        }
    } else if (name == starfish()->staticStrings()->m_marginwidth) {
        // https://html.spec.whatwg.org/multipage/rendering.html#the-page
        if (m_browsingContext && m_browsingContext->document()->body()) {
            m_browsingContext->document()->body()->setNeedsStyleRecalc(
                StyleChangeReason::JustNeedsRecalcSelf);
        }
    } else if (name == starfish()->staticStrings()->m_marginheight) {
        // https://html.spec.whatwg.org/multipage/rendering.html#the-page
        if (m_browsingContext && m_browsingContext->document()->body()) {
            m_browsingContext->document()->body()->setNeedsStyleRecalc(
                StyleChangeReason::JustNeedsRecalcSelf);
        }
    } else if (name == starfish()->staticStrings()->m_srcdoc) {
        if (!inHTMLConstructionSite()) {
            unloadSrc();
            if (value->length() && document()->doesParticipateInRendering()) {
                loadSrcDoc();
            }
            if (m_browsingContext && m_browsingContext->document()) {
                m_browsingContext->document()->setCompatibilityMode(
                    Document::CompatibilityMode::NoQuirksModeForce);
            }
        }
    }
}

void HTMLIFrameElement::didNodeInsertedToDocumentTree()
{
    HTMLElement::didNodeInsertedToDocumentTree();
    if (document()->doesParticipateInRendering()) {
        if (srcdoc()->length() == 0) {
            loadSrc();
        } else {
            loadSrcDoc();
        }
    } else {
        unloadSrc();
    }
}

void HTMLIFrameElement::didNodeRemovedFromDocumentTree()
{
    HTMLElement::didNodeRemovedFromDocumentTree();
    unloadSrc();
}

void HTMLIFrameElement::loadSrc()
{
    String* s = src();
    GET_EFFECTIVE_REFERRERPOLICY();
    if (s->length()) {
        navigate(new ResourceURL(s, document()->baseURL()->baseURI()),
                 HistoryManagerAction::Intact,
                 new ReferrerURL(document()->documentURI(), policy));
    } else {
        navigate(new ResourceURL(String::createASCIIString("about:blank")),
                 HistoryManagerAction::Intact,
                 new ReferrerURL(document()->documentURI(), policy));
    }
}

void HTMLIFrameElement::unloadSrc()
{
    if (m_browsingContext) {
        if (document()->doesParticipateInRendering() &&
            m_browsingContext->document()->doesParticipateInRendering()) {
            // we need to rebuild StackingContext tree for removing rendered
            // result
            webView()->setNeedsEstablishesStackingContext();
        }
        m_browsingContext->dispose();
        m_browsingContext = nullptr;
    }
}

void HTMLIFrameElement::loadSrcDoc()
{
    std::string srcDoc = srcdoc()->toUTF8NonGCString();
    GET_EFFECTIVE_REFERRERPOLICY();
    if (srcDoc.length()) {
        auto dataURI = Base64Utils::encodeBase64HTMLDataURI(srcDoc);
        navigate(new ResourceURL(String::createASCIIString(dataURI.c_str(),
                                                           dataURI.length()),
                                 document()->baseURL()->baseURI()),
                 HistoryManagerAction::Intact,
                 new ReferrerURL(document()->documentURI(), policy));
    } else {
        navigate(new ResourceURL(String::createASCIIString("about:blank")),
                 HistoryManagerAction::Intact,
                 new ReferrerURL(document()->documentURI(), policy));
    }
}

int HTMLIFrameElement::tabIndex()
{
    Optional<String*> result =
        getAttribute(starfish()->staticStrings()->m_tabindex);
    if (result.hasValue()) {
        return String::parseInt(result.getValue());
    }
    return 0;
}

Document* HTMLIFrameElement::contentDocument()
{
    if (m_browsingContext && !m_isContentDocumentDisabled) {
        return m_browsingContext->document();
    }
    return nullptr;
}

WindowProxy* HTMLIFrameElement::contentWindow()
{
    if (m_browsingContext) {
        return m_contentWindowProxy;
    }
    return nullptr;
}

String* HTMLIFrameElement::referrerPolicy()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_referrerpolicy);
}

void HTMLIFrameElement::setReferrerPolicy(String* policy)
{
    if (ReferrerURL::isValidPolicy(policy)) {
        setAttribute(starfish()->staticStrings()->m_referrerpolicy, policy);
    }
}

void HTMLIFrameElement::navigate(ResourceURL* url, HistoryManagerAction type,
                                 ReferrerURL* referrerURL,
                                 CustomHTMLIFrameElementType elementType)
{
    if (ResourceURL::isValidURL(url->urlString())) {
        String* name = nameAttr();
        if (m_browsingContext) {
            name = m_browsingContext->name();
        }
        unloadSrc();
        unmarkContentDocumentDisabled();

        CSPDirectives directiveType;
        bool allowScripting =
            elementType ==
            CustomHTMLIFrameElementType::NotCustomHTMLIFrameElement;
        if (elementType == CustomHTMLIFrameElementType::SVG) {
            directiveType = CSPDirectives::ImgSrc;
        } else {
            directiveType = CSPDirectives::ChildSrc;
        }

        if (!document()->contentSecurityPolicy()->allowSource(directiveType,
                                                              url)) {
            /*
            NOTE: IFrames blocked by CSP should generate a 'load', not 'error'
            event, regardless of blocked state. This means they appear to be
            normal cross-origin loads, thereby not leaking URL information
            directly to JS. We solve that through replacing the requested url
            with blankurl.
            */
            url = ResourceURL::aboutBlankURL();
        }

        if (!m_historyManager) {
            m_historyManager = HistoryManager::create(this);
        }

        if (m_browsingContext) {
            m_browsingContext->dispose();
        }

        m_browsingContext = BrowsingContext::create(this, allowScripting);
        m_browsingContext->setName(name);
        m_browsingContext->open(url, type, referrerURL);

        if (!m_contentWindowProxy) {
            m_contentWindowProxy = m_browsingContext->window()->window();
        }
    }
}

void HTMLIFrameElement::childBrowsingContextLoaded()
{
    String* eventType = starfish()->staticStrings()->m_load.localName();
    Event* e =
        new Event(executionContext(), eventType, EventInit(false, false));
    dispatchEventByUA(this, e, true);
}

void HTMLIFrameElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    HTMLElement::styleForPresentationAttribute(cssValues);
    size_t idx = hasAttribute(starfish()->staticStrings()->m_frameborder);
    if (idx != SIZE_MAX) {
        String* val = getAssuredAttribute(idx);
        if (val->equals("0") || val->equalsIgnoreCase("none")) {
            CSSStyleValuePair pair;
            pair.setLengthValue(CSSLength(0));
            pair.setKeyKind(CSSStyleValuePair::BorderTopWidth);
            cssValues.push_back(pair);

            pair.setKeyKind(CSSStyleValuePair::BorderRightWidth);
            cssValues.push_back(pair);

            pair.setKeyKind(CSSStyleValuePair::BorderBottomWidth);
            cssValues.push_back(pair);

            pair.setKeyKind(CSSStyleValuePair::BorderLeftWidth);
            cssValues.push_back(pair);
        }
    }
}

String* HTMLIFrameElement::nameAttr()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_name);
}

void HTMLIFrameElement::setNameAttr(String* name)
{
    setAttribute(starfish()->staticStrings()->m_name, name);
}
} // namespace Starfish
