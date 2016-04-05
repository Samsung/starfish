#include "StarFishConfig.h"
#include "XMLDocumentBuilder.h"
#include "tinyxml2.h"

#include "dom/DOM.h"
#include "dom/binding/ScriptBindingInstance.h"

namespace StarFish {

void XMLDocumentBuilder::build(Document* document, String* filePath)
{
    document->setFirstChild(nullptr);

    tinyxml2::XMLDocument doc;
    doc.LoadFile(filePath->utf8Data());

    if (doc.Error()) {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    std::function<void(Node * parentNode, tinyxml2::XMLElement * xmlElement)> fn = [&fn, &document](Node* parentNode, tinyxml2::XMLElement* xmlElement)
    {
        Node* newNode = nullptr;
        int type = xmlElement->IntAttribute("nodeType");
        if (type == 0) {
            // invaild node.
            return;
        }
        if (type == 9) {
            newNode = document;
        } else if (type == 10) {
            newNode = new DocumentType(document);
            parentNode->appendChildForParser(newNode);
            return;
        } else if (type == 3) {
            newNode = new Text(document, String::fromUTF8(xmlElement->FirstChildElement()->FirstChild()->Value()));
            parentNode->appendChildForParser(newNode);
            return;
        } else if (type == 8) {
            newNode = new Comment(document, String::emptyString);
            parentNode->appendChildForParser(newNode);
            return;
        } else if (type == 1) {
            const char* name = xmlElement->Attribute("localName");
            QualifiedName qname = QualifiedName::fromString(document->window()->starFish(), name);
            newNode = document->createElement(qname);

            STARFISH_ASSERT(newNode);

            if (newNode->isElement()) {
                const tinyxml2::XMLAttribute* attr = xmlElement->FirstAttribute();
                while (attr) {
                    if (strcmp(attr->Name(), "nodeType") == 0 || strcmp(attr->Name(), "localName") == 0) {
                        attr = attr->Next();
                        continue;
                    }
                    newNode->asElement()->setAttribute(QualifiedName::fromString(document->window()->starFish(), attr->Name()), String::fromUTF8(attr->Value()));
                    attr = attr->Next();
                }
            }

            if (newNode->isElement() && newNode->asElement()->isHTMLElement() && newNode->asElement()->asHTMLElement()->isHTMLScriptElement()) {
                parentNode->appendChildForParser(newNode);

                tinyxml2::XMLElement* child = xmlElement->FirstChildElement();
                while (child) {
                    fn(newNode, child);
                    child = child->NextSiblingElement();
                }
                newNode->asElement()->asHTMLElement()->asHTMLScriptElement()->executeScript();
                return;
            } else if (newNode->isElement() && newNode->asElement()->isHTMLElement() && newNode->asElement()->asHTMLElement()->isHTMLStyleElement()) {
                parentNode->appendChildForParser(newNode);

                tinyxml2::XMLElement* child = xmlElement->FirstChildElement();
                while (child) {
                    fn(newNode, child);
                    child = child->NextSiblingElement();
                }
                newNode->asElement()->asHTMLElement()->asHTMLStyleElement()->generateStyleSheet();
                return;
            } else if (newNode->isElement() && newNode->asElement()->isHTMLElement() && newNode->asElement()->asHTMLElement()->isHTMLLinkElement()) {
                if (parentNode) {
                    parentNode->appendChild(newNode);
                }
                return;
            }
        } else {
            printf("invalid node type %d\n", type);
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }


        if (parentNode) {
            parentNode->appendChildForParser(newNode);
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
