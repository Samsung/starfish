
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
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

protected:
};

}

#endif
