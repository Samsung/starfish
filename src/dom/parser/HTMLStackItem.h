/*
 * Copyright (C) 2012 Company 100, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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

#ifndef __StarFishHTMLStackItem__
#define __StarFishHTMLStackItem__

#include "util/String.h"
#include "dom/Element.h"
#include "dom/Document.h"
#include "dom/parser/AtomicHTMLToken.h"

namespace StarFish {

class HTMLStackItem : public gc {
public:
    enum ItemType {
        ItemForContextElement,
        ItemForDocumentFragmentNode
    };

    HTMLStackItem(Node* node, ItemType type)
        : m_node(node)
        , m_tokenLocalName(AtomicString::emptyAtomicString())
        , m_namespaceURI(AtomicString::emptyAtomicString())
    {
        switch (type) {
        case ItemForDocumentFragmentNode:
            m_isDocumentFragmentNode = true;
            break;
        case ItemForContextElement:
            m_tokenLocalName = AtomicString::createAttrAtomicString(m_node->document()->window()->starFish(), m_node->localName());
            m_namespaceURI = m_node->asElement()->name().namespaceURIAtomic();
            m_isDocumentFragmentNode = false;
            break;
        }
    }

    HTMLStackItem(Node* node, AtomicHTMLToken* token, const AtomicString& namespaceURI)
        : m_node(node)
        , m_tokenLocalName(token->name())
        , m_tokenAttributes(token->attributes())
        , m_namespaceURI(namespaceURI)
        , m_isDocumentFragmentNode(false)
    {
    }

    Element* element() const { return m_node->asElement(); }
    Node* node() const { return m_node; }

    bool isDocumentFragmentNode() const { return m_isDocumentFragmentNode; }
    bool isElementNode() const { return !m_isDocumentFragmentNode; }

    const AtomicString& namespaceURI() const { return m_namespaceURI; }
    const AtomicString& localName() const { return m_tokenLocalName; }

    const AttributeVector& attributes() const { STARFISH_ASSERT(m_tokenLocalName.string()->length()); return m_tokenAttributes; }
    Attribute* getAttributeItem(const QualifiedName& attributeName)
    {
        STARFISH_ASSERT(m_tokenLocalName);
        return findAttributeInVector(m_tokenAttributes, attributeName);
    }
    bool hasLocalName(const AtomicString& name) const { return m_tokenLocalName == name; }
    bool hasTagName(const QualifiedName& name) const { return m_tokenLocalName == name.localNameAtomic() && m_namespaceURI == name.namespaceURIAtomic(); }

    bool matchesHTMLTag(const AtomicString& name) const { return m_tokenLocalName == name && m_namespaceURI == staticStrings().m_xhtmlNamespaceURI; }
    bool matchesHTMLTag(const QualifiedName& name) const { return m_tokenLocalName == name && m_namespaceURI == staticStrings().m_xhtmlNamespaceURI; }

    bool causesFosterParenting()
    {
        return hasTagName(staticStrings().m_tableTagName)
            || hasTagName(staticStrings().m_tbodyTagName)
            || hasTagName(staticStrings().m_tfootTagName)
            || hasTagName(staticStrings().m_theadTagName)
            || hasTagName(staticStrings().m_trTagName);
    }

    bool isInHTMLNamespace() const
    {
        // A DocumentFragment takes the place of the document element when parsing
        // fragments and should be considered in the HTML namespace.
        // return namespaceURI() == HTMLNames::xhtmlNamespaceURI
        //    || isDocumentFragmentNode(); // FIXME: Does this also apply to ShadowRoot?
        return true;
    }

    bool isNumberedHeaderElement() const
    {
        return hasTagName(staticStrings().m_h1TagName)
            || hasTagName(staticStrings().m_h2TagName)
            || hasTagName(staticStrings().m_h3TagName)
            || hasTagName(staticStrings().m_h4TagName)
            || hasTagName(staticStrings().m_h5TagName)
            || hasTagName(staticStrings().m_h6TagName);
    }

    bool isTableBodyContextElement() const
    {
        return hasTagName(staticStrings().m_tbodyTagName)
            || hasTagName(staticStrings().m_tfootTagName)
            || hasTagName(staticStrings().m_theadTagName);
    }

    // http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#special
    bool isSpecialNode() const
    {
        if (isDocumentFragmentNode())
            return true;
        if (!isInHTMLNamespace())
            return false;
        const AtomicString& localName = this->localName();
        const StaticStrings& ss = staticStrings();
        return localName == ss.m_addressTagName.localNameAtomic()
            || localName == ss.m_appletTagName.localNameAtomic()
            || localName == ss.m_areaTagName.localNameAtomic()
            || localName == ss.m_articleTagName.localNameAtomic()
            || localName == ss.m_asideTagName.localNameAtomic()
            || localName == ss.m_baseTagName.localNameAtomic()
            || localName == ss.m_basefontTagName.localNameAtomic()
            || localName == ss.m_bgsoundTagName.localNameAtomic()
            || localName == ss.m_blockquoteTagName.localNameAtomic()
            || localName == ss.m_bodyTagName.localNameAtomic()
            || localName == ss.m_brTagName.localNameAtomic()
            || localName == ss.m_buttonTagName.localNameAtomic()
            || localName == ss.m_captionTagName.localNameAtomic()
            || localName == ss.m_centerTagName.localNameAtomic()
            || localName == ss.m_colTagName.localNameAtomic()
            || localName == ss.m_colgroupTagName.localNameAtomic()
            || localName == ss.m_commandTagName.localNameAtomic()
            || localName == ss.m_ddTagName.localNameAtomic()
            || localName == ss.m_detailsTagName.localNameAtomic()
            || localName == ss.m_dirTagName.localNameAtomic()
            || localName == ss.m_divTagName.localNameAtomic()
            || localName == ss.m_dlTagName.localNameAtomic()
            || localName == ss.m_dtTagName.localNameAtomic()
            || localName == ss.m_embedTagName.localNameAtomic()
            || localName == ss.m_fieldsetTagName.localNameAtomic()
            || localName == ss.m_figcaptionTagName.localNameAtomic()
            || localName == ss.m_figureTagName.localNameAtomic()
            || localName == ss.m_footerTagName.localNameAtomic()
            || localName == ss.m_formTagName.localNameAtomic()
            || localName == ss.m_frameTagName.localNameAtomic()
            || localName == ss.m_framesetTagName.localNameAtomic()
            || isNumberedHeaderElement()
            || localName == ss.m_headTagName.localNameAtomic()
            || localName == ss.m_headerTagName.localNameAtomic()
            || localName == ss.m_hgroupTagName.localNameAtomic()
            || localName == ss.m_hrTagName.localNameAtomic()
            || localName == ss.m_htmlTagName.localNameAtomic()
            || localName == ss.m_iframeTagName.localNameAtomic()
            || localName == ss.m_imgTagName.localNameAtomic()
            || localName == ss.m_inputTagName.localNameAtomic()
            || localName == ss.m_liTagName.localNameAtomic()
            || localName == ss.m_linkTagName.localNameAtomic()
            || localName == ss.m_listingTagName.localNameAtomic()
            || localName == ss.m_mainTagName.localNameAtomic()
            || localName == ss.m_marqueeTagName.localNameAtomic()
            || localName == ss.m_menuTagName.localNameAtomic()
            || localName == ss.m_metaTagName.localNameAtomic()
            || localName == ss.m_navTagName.localNameAtomic()
            || localName == ss.m_noembedTagName.localNameAtomic()
            || localName == ss.m_noframesTagName.localNameAtomic()
            || localName == ss.m_noscriptTagName.localNameAtomic()
            || localName == ss.m_objectTagName.localNameAtomic()
            || localName == ss.m_olTagName.localNameAtomic()
            || localName == ss.m_pTagName.localNameAtomic()
            || localName == ss.m_paramTagName.localNameAtomic()
            || localName == ss.m_plaintextTagName.localNameAtomic()
            || localName == ss.m_preTagName.localNameAtomic()
            || localName == ss.m_scriptTagName.localNameAtomic()
            || localName == ss.m_sectionTagName.localNameAtomic()
            || localName == ss.m_selectTagName.localNameAtomic()
            || localName == ss.m_styleTagName.localNameAtomic()
            || localName == ss.m_summaryTagName.localNameAtomic()
            || localName == ss.m_tableTagName.localNameAtomic()
            || isTableBodyContextElement()
            || localName == ss.m_tdTagName.localNameAtomic()
            || localName == ss.m_templateTagName.localNameAtomic()
            || localName == ss.m_textareaTagName.localNameAtomic()
            || localName == ss.m_thTagName.localNameAtomic()
            || localName == ss.m_titleTagName.localNameAtomic()
            || localName == ss.m_trTagName.localNameAtomic()
            || localName == ss.m_ulTagName.localNameAtomic()
            || localName == ss.m_wbrTagName.localNameAtomic()
            || localName == ss.m_xmpTagName.localNameAtomic();
    }

private:
    Node* m_node;
    const StaticStrings& staticStrings() const
    {
        return *m_node->document()->window()->starFish()->staticStrings();
    }

    AtomicString m_tokenLocalName;
    AttributeVector m_tokenAttributes;
    AtomicString m_namespaceURI;
    bool m_isDocumentFragmentNode;
};

}

#endif
