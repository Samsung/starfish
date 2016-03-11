#include "StarFishConfig.h"
#include "dom/Document.h"
#include "HTMLScriptElement.h"


namespace StarFish {

void HTMLScriptElement::executeScript()
{
    if (!m_isAlreadyStarted) {
        size_t idx = hasAtttibute(document()->window()->starFish()->staticStrings()->m_src);
        if (idx == SIZE_MAX) {
            String* script = textContent();
            document()->window()->starFish()->evaluate(script);
        } else {
            String* url = getAttribute(idx);
            FILE* fp = fopen(document()->window()->starFish()->makeResourcePath(url)->utf8Data(), "r");
            if (fp) {
                fseek(fp, 0, SEEK_END);
                size_t siz = ftell(fp);
                rewind(fp);

                char* fileContents = (char*)malloc(siz + 1);
                fread(fileContents, sizeof(char), siz, fp);

                fileContents[siz] = 0;

                document()->window()->starFish()->evaluate(String::fromUTF8(fileContents));

                free(fileContents);
                fclose(fp);
            }
        }
        m_isAlreadyStarted = true;
    }
}

void HTMLScriptElement::didNodeInserted()
{
    HTMLElement::didNodeInserted();
    if (isInDocumentScope())
        executeScript();
}

}
