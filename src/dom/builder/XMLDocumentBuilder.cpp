#include "StarFishConfig.h"
#include "XMLDocumentBuilder.h"
#include "tinyxml2.h"

#include "dom/DOM.h"
#include "dom/binding/ScriptBindingInstance.h"

namespace StarFish
{

void XMLDocumentBuilder::build(Document* document, String* filePath)
{
    document->setFirstChild(nullptr);

    tinyxml2::XMLDocument doc;
    doc.LoadFile(filePath->utf8Data());

    if (doc.Error()) {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    std::function<void(Node * parentNode, tinyxml2::XMLElement * xmlElement)> fn = [&fn, &document](Node* parentNode, tinyxml2::XMLElement* xmlElement) {
        Node* newNode = nullptr;
        int type = xmlElement->IntAttribute("nodeType");
        if (type == 0) {
            // invaild node.
            return ;
        }
        if (type == 9) {
            newNode = document;
        } else if (type == 10) {
            newNode = new DocumentType(document);
        } else if (type == 3) {
            newNode = new Text(document, String::fromUTF8(xmlElement->FirstChildElement()->FirstChild()->Value()));
            parentNode->appendChild(newNode);
            return;
        } else if (type == 8) {
            newNode = new Comment(document, String::emptyString);
            parentNode->appendChild(newNode);
            return;
        } else if (type == 1) {
            const char* name = xmlElement->Attribute("localName");
            // TODO use document.createElementMethod
            QualifiedName qname = QualifiedName::fromString(document->window()->starFish(), name);

            newNode = document->createElement(qname);
            if (newNode->isElement() && newNode->asElement()->isHTMLElement() && newNode->asElement()->asHTMLElement()->isHTMLScriptElement()) {
                if (parentNode) {
                    parentNode->appendChild(newNode);
                }

                tinyxml2::XMLElement* child = xmlElement->FirstChildElement();
                while (child) {
                    fn(newNode, child);
                    child = child->NextSiblingElement();
                }

                String* script = newNode->asElement()->firstChild()->asCharacterData()->asText()->data();
                document->window()->starFish()->evaluate(script);
                return;
            } else if (newNode->isElement() && newNode->asElement()->isHTMLElement() && newNode->asElement()->asHTMLElement()->isHTMLStyleElement()) {
                //resolve styles.
                tinyxml2::XMLElement* e = xmlElement->FirstChildElement();
                CSSStyleSheet* sheet = new CSSStyleSheet;
                while (e) {
                    if (strcmp(e->Value(), "style") == 0) {
                        CSSStyleRule::PseudoClass pc = CSSStyleRule::PseudoClass::None;

                        const char* selectorText1 = e->Attribute("selectorText");
                        // Split comma separated selectors if and and duplicate style
                        // e.g., #id1, #id2 { blah; } => #id1 { blah; } #id2 { blah; }
                        String* str = String::fromUTF8(selectorText1);
                        String::Vector tokens;
                        str->split(',', tokens);
                        for (unsigned i=0; i < tokens.size(); i++) {
                            const char* selectorText = tokens[i]->trim()->utf8Data();
                            CSSStyleRule::Kind kind;
                            String* st;
                            std::string cSelectorText;
                            char* pcPos = strchr((char *)selectorText, ':');
                            if (pcPos) {
                                cSelectorText = selectorText;
                                cSelectorText[pcPos - selectorText] = '\0';
                                selectorText = cSelectorText.data();
                                if (strcmp(pcPos + 1, "active") == 0) {
                                    pc = CSSStyleRule::PseudoClass::Active;
                                } else if (strcmp(pcPos + 1, "hover") == 0) {
                                    pc = CSSStyleRule::PseudoClass::Hover;
                                }
                            }

                            if (selectorText[0] == '.') {
                                kind = CSSStyleRule::Kind::ClassSelector;
                                st = String::fromUTF8(&selectorText[1]);
                            } else if (selectorText[0] == '#') {
                                kind = CSSStyleRule::Kind::IdSelector;
                                st = String::fromUTF8(&selectorText[1]);
                            } else if (selectorText[0] == '*') {
                                kind = CSSStyleRule::Kind::UniversalSelector;
                            } else {
                                kind = CSSStyleRule::Kind::TypeSelector;
                                if (selectorText[0] == '*') {
                                    st = String::emptyString;
                                } else {
                                    st = String::fromUTF8(selectorText);
                                }
                            }

                            CSSStyleRule *rule = new CSSStyleRule(kind, st, pc, document);
                            const tinyxml2::XMLAttribute* attr = e->FirstAttribute();
                            while (attr) {
                                if (strcmp(attr->Name(), "selectorText") != 0) {
                                    const char* n = attr->Name();
                                    const char* v = attr->Value();
                                    bool result;
                                    CSSStyleValuePair ret = CSSStyleValuePair::fromString(n, v, result);
                                    if (result)
                                        rule->styleDeclaration()->addValuePair(ret);
                                }
                                attr = attr->Next();
                            }
                            sheet->addRule(rule);
                        }
                    }
                    e = e->NextSiblingElement();
                }

                document->window()->styleResolver()->addSheet(sheet);

                // TODO add child
                return ;
            }
        } else {
            printf("invalid node type %d\n", type);
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }

        STARFISH_ASSERT(newNode);

        if (newNode->isElement()) {
            const tinyxml2::XMLAttribute* attr = xmlElement->FirstAttribute();
            while (attr) {
                if (strcmp(attr->Name(), "nodeType") == 0) {
                    attr = attr->Next();
                    continue;
                }

                newNode->asElement()->setAttribute(QualifiedName::fromString(document->window()->starFish(), attr->Name()), String::fromUTF8(attr->Value()));
                attr = attr->Next();
            }
        }

        if (parentNode) {
            parentNode->appendChild(newNode);
        }

        tinyxml2::XMLElement* child = xmlElement->FirstChildElement();
        while (child) {
            fn(newNode, child);
            child = child->NextSiblingElement();
        }

    };

    fn(nullptr, doc.FirstChildElement()->FirstChildElement());
}
}
