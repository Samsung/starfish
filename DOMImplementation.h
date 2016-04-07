#ifndef __StarFishDOMImplementation__
#define __StarFishDOMImplementation__

#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class DocumentType;
class Window;

class DOMImplementation : public ScriptWrappable {
public:
    DOMImplementation(Window* window, ScriptBindingInstance* instance)
        : ScriptWrappable(this)
        , m_window(window)
        , m_instance(instance)
    {
        initScriptWrappable(this, instance);
    }

    DocumentType* createDocumentType(String* qualifiedName, String* publicId, String* systemId);
    Document* createHTMLDocument(String* title = String::spaceString);

    bool hasFeature() { return true; } // useless; always returns true
private:
    Window* m_window;
    ScriptBindingInstance* m_instance;
};
}

#endif
