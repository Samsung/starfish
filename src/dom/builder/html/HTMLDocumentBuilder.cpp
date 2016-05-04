#include "StarFishConfig.h"
#include "HTMLDocumentBuilder.h"
#include "tinyxml2.h"

#include "dom/DOM.h"
#include "dom/binding/ScriptBindingInstance.h"
#include "platform/file_io/FileIO.h"

#include "dom/parser/HTMLParser.h"

namespace StarFish {

void HTMLDocumentBuilder::build(Document* document, String* filePath)
{
    FileIO* fio = FileIO::create();
    String* string = String::emptyString;
    if (fio->open(filePath)) {
        size_t len = fio->length();
        char* fileContents = (char*)malloc(len + 1);
        fio->read(fileContents, sizeof(char), len);
        fileContents[len] = 0;
        string = String::fromUTF8(fileContents);
        free(fileContents);
        fio->close();
    } else
        STARFISH_RELEASE_ASSERT_NOT_REACHED();

    HTMLParser parser(document->window()->starFish(), document, string);
    parser.parse();
}

}
