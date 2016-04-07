#include "StarFishConfig.h"
#include "XMLDocumentBuilder.h"
#include "tinyxml2.h"

#include "dom/DOM.h"
#include "dom/binding/ScriptBindingInstance.h"
#include "platform/file_io/FileIO.h"

namespace StarFish {

void XMLDocumentBuilder::build(Document* document, String* filePath)
{
    document->setFirstChild(nullptr);

    tinyxml2::XMLDocument doc;
    char* fileContents;
    long int len;
    FileIO* fio = FileIO::create();
    if (fio->open(filePath->utf8Data())) {
        len = fio->length();
        fileContents = (char*)malloc(len + 1);
        fio->read(fileContents, sizeof(char), len);
        fileContents[len] = 0;
        doc.Parse(fileContents, len);
        free(fileContents);
        fio->close();
    } else
        STARFISH_RELEASE_ASSERT_NOT_REACHED();

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
                tinyxml2::XMLElement* firstChild = xmlElement->FirstChildElement();
                if (firstChild && strcmp(firstChild->Name(), "attributes") == 0) {
                    tinyxml2::XMLElement* attr = firstChild->FirstChildElement();
                    while (attr) {
                        STARFISH_ASSERT(strcmp(attr->Name(), "attr") == 0);
                        tinyxml2::XMLElement* nameElement = attr->FirstChildElement();
                        tinyxml2::XMLElement* valueElement = nameElement->NextSiblingElement();
                        STARFISH_ASSERT(nameElement && valueElement);
                        STARFISH_ASSERT(strcmp(nameElement->Name(), "name") == 0);
                        STARFISH_ASSERT(strcmp(valueElement->Name(), "value") == 0);
                        STARFISH_ASSERT(nameElement->FirstChild() && valueElement->FirstChild());
                        // STARFISH_ASSERT(attr->FirstAttribute() && strcmp(attr->FirstAttribute()->Name(), "value") == 0);
                        // STARFISH_ASSERT(attr->FirstChild());
                        // QualifiedName name = QualifiedName::fromString(document->window()->starFish(), attr->FirstChild()->Value());
                        // String* value = String::fromUTF8(attr->FirstAttribute()->Value());
                        QualifiedName name = QualifiedName::fromString(document->window()->starFish(), nameElement->FirstChild()->Value());
                        String* value = String::fromUTF8(valueElement->FirstChild()->Value());
                        newNode->asElement()->setAttribute(name, value);
                        attr = attr->NextSiblingElement();
                    }
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
