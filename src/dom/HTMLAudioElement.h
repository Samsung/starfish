#ifndef __StarFishHTMLAudioElement__
#define __StarFishHTMLAudioElement__

#include "dom/HTMLElement.h"

namespace StarFish {

class HTMLAudioElement : public HTMLElement {
public:
    HTMLAudioElement(Document* document)
        : HTMLElement(document)
    {
        initScriptWrappable(this);
        m_isPlaying = false;
    }

    /* 4.4 Interface Node */

    virtual String* localName()
    {
        return document()->window()->starFish()->staticStrings()->m_audioLocalName;
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLAudioElement() const
    {
        return true;
    }

    void play() {
        printf("Audio, play\n");
        m_isPlaying = true;
    }

    void pause() {
        printf("Audio, pause\n");
        m_isPlaying = false;
    }

    bool paused() { return !m_isPlaying; }

protected:
    bool m_isPlaying;
};

}

#endif
