#include "StarFishConfig.h"
#include "dom/Document.h"
#include "HTMLScriptElement.h"

#include "platform/message_loop/MessageLoop.h"
#include "platform/file_io/FileIO.h"

namespace StarFish {

void HTMLScriptElement::executeScript()
{
    if (!m_isAlreadyStarted) {
        size_t idx = hasAttribute(document()->window()->starFish()->staticStrings()->m_src);
        if (idx == SIZE_MAX) {
            String* script = textContent();
            document()->window()->starFish()->evaluate(script);
        } else {
            String* url = getAttribute(idx);
            FileIO* fio = FileIO::create();
            if (fio->open(document()->window()->starFish()->makeResourcePath(url)->utf8Data())) {
                fio->seek(0, SEEK_END);
                size_t siz = fio->tell();
                fio->rewind();

                char* fileContents = (char*)malloc(siz + 1);
                fio->read(fileContents, sizeof(char), siz);

                fileContents[siz] = 0;

                document()->window()->starFish()->evaluate(String::fromUTF8(fileContents));

                free(fileContents);
                fio->close();

                document()->window()->starFish()->messageLoop()->addIdler([](void* data) {
                    HTMLScriptElement* element = (HTMLScriptElement*)data;
                    String* eventType = element->document()->window()->starFish()->staticStrings()->m_load.string();
                    Event* e = new Event(eventType, EventInit(false, false));
                    element->EventTarget::dispatchEvent(element, e);
                }, this);
            }
        }
        m_isAlreadyStarted = true;
    }
}

void HTMLScriptElement::didNodeInsertedToDocumenTree()
{
    HTMLElement::didNodeInsertedToDocumenTree();
    executeScript();
}

}
