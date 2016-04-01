
#ifndef __StarFishHTMLDocument__
#define __StarFishHTMLDocument__

#include "Document.h"

namespace StarFish {

class Window;

class HTMLDocument : public Document {
public:
    HTMLDocument(Window* window, ScriptBindingInstance* scriptBindingInstance)
        : Document(window, scriptBindingInstance)
    {
        initScriptWrappable(this);
    }

protected:
};

}

#endif
