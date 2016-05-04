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
        , m_tokenLocalName(QualifiedName::emptyQualifiedName())
    {
        switch (type) {
        case ItemForDocumentFragmentNode:
            m_isDocumentFragmentNode = true;
            break;
        case ItemForContextElement:
            m_tokenLocalName = QualifiedName::fromString(m_node->document()->window()->starFish(), m_node->localName());
            m_isDocumentFragmentNode = false;
            break;
        }
    }

    HTMLStackItem(Node* node, AtomicHTMLToken* token)
        : m_node(node)
        , m_tokenLocalName(token->name())
        , m_tokenAttributes(token->attributes())
        , m_isDocumentFragmentNode(false)
    {
    }

    Element* element() const { return m_node->asElement(); }
    Node* node() const { return m_node; }

    bool isDocumentFragmentNode() const { return m_isDocumentFragmentNode; }
    bool isElementNode() const { return !m_isDocumentFragmentNode; }

    const QualifiedName& localName() const { return m_tokenLocalName; }

    const AttributeVector& attributes() const { STARFISH_ASSERT(m_tokenLocalName.string()->length()); return m_tokenAttributes; }
    Attribute* getAttributeItem(const QualifiedName& attributeName)
    {
        STARFISH_ASSERT(m_tokenLocalName);
        return findAttributeInVector(m_tokenAttributes, attributeName);
    }

    bool hasLocalName(const QualifiedName& name) const { return m_tokenLocalName == name; }
    bool hasTagName(const QualifiedName& name) const { return m_tokenLocalName == name; }

    bool matchesHTMLTag(const QualifiedName& name) const { return m_tokenLocalName == name; }

    bool causesFosterParenting()
    {
        return hasTagName(staticStrings().m_tableLocalName)
            || hasTagName(staticStrings().m_tbodyLocalName)
            || hasTagName(staticStrings().m_tfootLocalName)
            || hasTagName(staticStrings().m_theadLocalName)
            || hasTagName(staticStrings().m_trLocalName);
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
        return hasTagName(staticStrings().m_h1LocalName)
            || hasTagName(staticStrings().m_h2LocalName)
            || hasTagName(staticStrings().m_h3LocalName)
            || hasTagName(staticStrings().m_h4LocalName)
            || hasTagName(staticStrings().m_h5LocalName)
            || hasTagName(staticStrings().m_h6LocalName);
    }

    bool isTableBodyContextElement() const
    {
        return hasTagName(staticStrings().m_tbodyLocalName)
            || hasTagName(staticStrings().m_tfootLocalName)
            || hasTagName(staticStrings().m_theadLocalName);
    }

    // http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#special
    bool isSpecialNode() const
    {
        /*
        if (hasTagName(MathMLNames::miTag)
            || hasTagName(MathMLNames::moTag)
            || hasTagName(MathMLNames::mnTag)
            || hasTagName(MathMLNames::msTag)
            || hasTagName(MathMLNames::mtextTag)
            || hasTagName(MathMLNames::annotation_xmlTag)
            || hasTagName(SVGNames::foreignObjectTag)
            || hasTagName(SVGNames::descTag)
            || hasTagName(SVGNames::titleTag))
            return true;
        */
        if (isDocumentFragmentNode())
            return true;
        if (!isInHTMLNamespace())
            return false;
        const QualifiedName& localName = this->localName();
        return localName == staticStrings().m_addressLocalName
            || localName == staticStrings().m_appletLocalName
            || localName == staticStrings().m_areaLocalName
            || localName == staticStrings().m_articleLocalName
            || localName == staticStrings().m_asideLocalName
            || localName == staticStrings().m_baseLocalName
            || localName == staticStrings().m_basefontLocalName
            || localName == staticStrings().m_bgsoundLocalName
            || localName == staticStrings().m_blockquoteLocalName
            || localName == staticStrings().m_bodyLocalName
            || localName == staticStrings().m_brLocalName
            || localName == staticStrings().m_buttonLocalName
            || localName == staticStrings().m_captionLocalName
            || localName == staticStrings().m_centerLocalName
            || localName == staticStrings().m_colLocalName
            || localName == staticStrings().m_colgroupLocalName
            || localName == staticStrings().m_commandLocalName
            || localName == staticStrings().m_ddLocalName
            || localName == staticStrings().m_detailsLocalName
            || localName == staticStrings().m_dirLocalName
            || localName == staticStrings().m_divLocalName
            || localName == staticStrings().m_dlLocalName
            || localName == staticStrings().m_dtLocalName
            || localName == staticStrings().m_embedLocalName
            || localName == staticStrings().m_fieldsetLocalName
            || localName == staticStrings().m_figcaptionLocalName
            || localName == staticStrings().m_figureLocalName
            || localName == staticStrings().m_footerLocalName
            || localName == staticStrings().m_formLocalName
            || localName == staticStrings().m_frameLocalName
            || localName == staticStrings().m_framesetLocalName
            || isNumberedHeaderElement()
            || localName == staticStrings().m_headLocalName
            || localName == staticStrings().m_headerLocalName
            || localName == staticStrings().m_hgroupLocalName
            || localName == staticStrings().m_hrLocalName
            || localName == staticStrings().m_htmlLocalName
            || localName == staticStrings().m_iframeLocalName
            || localName == staticStrings().m_imgLocalName
            || localName == staticStrings().m_inputLocalName
            || localName == staticStrings().m_liLocalName
            || localName == staticStrings().m_linkLocalName
            || localName == staticStrings().m_listingLocalName
            || localName == staticStrings().m_mainLocalName
            || localName == staticStrings().m_marqueeLocalName
            || localName == staticStrings().m_menuLocalName
            || localName == staticStrings().m_metaLocalName
            || localName == staticStrings().m_navLocalName
            || localName == staticStrings().m_noembedLocalName
            || localName == staticStrings().m_noframesLocalName
            || localName == staticStrings().m_noscriptLocalName
            || localName == staticStrings().m_objectLocalName
            || localName == staticStrings().m_olLocalName
            || localName == staticStrings().m_pLocalName
            || localName == staticStrings().m_paramLocalName
            || localName == staticStrings().m_plaintextLocalName
            || localName == staticStrings().m_preLocalName
            || localName == staticStrings().m_scriptLocalName
            || localName == staticStrings().m_sectionLocalName
            || localName == staticStrings().m_selectLocalName
            || localName == staticStrings().m_styleLocalName
            || localName == staticStrings().m_summaryLocalName
            || localName == staticStrings().m_tableLocalName
            || isTableBodyContextElement()
            || localName == staticStrings().m_tdLocalName
            || localName == staticStrings().m_templateLocalName
            || localName == staticStrings().m_textareaLocalName
            || localName == staticStrings().m_thLocalName
            || localName == staticStrings().m_titleLocalName
            || localName == staticStrings().m_trLocalName
            || localName == staticStrings().m_ulLocalName
            || localName == staticStrings().m_wbrLocalName
            || localName == staticStrings().m_xmpLocalName;
    }

private:
    Node* m_node;
    const StaticStrings& staticStrings() const
    {
        return *m_node->document()->window()->starFish()->staticStrings();
    }

    QualifiedName m_tokenLocalName;
    AttributeVector m_tokenAttributes;
    bool m_isDocumentFragmentNode;
};

}

#endif
