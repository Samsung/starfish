
#ifndef __StarFishHTMLDocument__
#define __StarFishHTMLDocument__

#include "Document.h"

namespace StarFish {

class Window;

class HTMLDocument : public Document {
public:
    HTMLDocument(Window* window, ScriptBindingInstance* scriptBindingInstance, const URL& url)
        : Document(window, scriptBindingInstance, url)
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
