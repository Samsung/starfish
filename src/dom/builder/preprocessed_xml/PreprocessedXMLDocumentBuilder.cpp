#include "StarFishConfig.h"
#include "PreprocessedXMLDocumentBuilder.h"
#include "tinyxml2.h"

#include "dom/DOM.h"
#include "dom/binding/ScriptBindingInstance.h"
#include "platform/file_io/FileIO.h"

namespace StarFish {

void PreprocessedXMLDocumentBuilder::build(Document* document, String* filePath)
{
    tinyxml2::XMLDocument doc;
    char* fileContents;
    long int len;
    FileIO* fio = FileIO::create();
    if (fio->open(filePath)) {
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
            newNode->beginParsing();
        } else if (type == 10) {
            newNode = new DocumentType(document);
            newNode->beginParsing();
            parentNode->parserAppendChild(newNode);
            return;
        } else if (type == 3) {
            newNode = new Text(document, String::fromUTF8(xmlElement->FirstChildElement()->FirstChild()->Value()));
            newNode->beginParsing();
            parentNode->parserAppendChild(newNode);
            return;
        } else if (type == 8) {
            newNode = new Comment(document, String::fromUTF8(xmlElement->FirstChildElement()->FirstChild()->Value()));
            newNode->beginParsing();
            parentNode->parserAppendChild(newNode);
            return;
        } else if (type == 1) {
            const char* name = xmlElement->Attribute("localName");
            QualifiedName qname = QualifiedName(document->window()->starFish()->staticStrings()->m_xhtmlNamespaceURI, AtomicString::createAttrAtomicString(document->window()->starFish(), name));
            newNode = document->createElement(qname);
            newNode->beginParsing();
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
                        QualifiedName name = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAttrAtomicString(document->window()->starFish(), nameElement->FirstChild()->Value()));
                        String* value = String::fromUTF8(valueElement->FirstChild()->Value());
                        newNode->asElement()->setAttribute(name, value);
                        attr = attr->NextSiblingElement();
                    }
                }
            }
        } else {
            printf("invalid node type %d\n", type);
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }


        if (parentNode) {
            parentNode->parserAppendChild(newNode);
        }

        tinyxml2::XMLElement* child = xmlElement->FirstChildElement();
        while (child) {
            fn(newNode, child);
            child = child->NextSiblingElement();
        }

        newNode->finishParsing();

    };

    fn(nullptr, doc.FirstChildElement()->FirstChildElement());
}
}
