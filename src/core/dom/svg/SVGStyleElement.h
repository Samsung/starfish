/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __StarFishSVGStyleElement__
#define __StarFishSVGStyleElement__

#include "core/dom/svg/SVGElement.h"

namespace StarFish {

class SVGStyleElement : public SVGElement {
public:
    SVGStyleElement(Document* document)
        : SVGElement(document)
        , m_generatedSheet(nullptr)
        , m_loaded(false)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGStyleElement() const override;

    virtual QualifiedName name();

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual bool needsGeometryAttributes()
    {
        return false;
    }

    virtual bool needsFillAttributes()
    {
        return false;
    }

    virtual bool needsStrokeAttributes()
    {
        return false;
    }

    virtual bool needsTransformAttributes()
    {
        return false;
    }

    /* Other methods (not in DOM API) */

    String* type();
    void setType(String* type);

    String* media();
    void setMedia(String* media);

    virtual void didCharacterDataModified(String* before, String* after);
    virtual void didNodeInsertedToDocumentTree();
    virtual void didNodeRemovedFromDocumentTree();
    virtual void didNodeInserted(Node* parent, Node* newChild);
    virtual void didNodeRemoved(Node* parent, Node* oldChild);
    virtual void finishParsing()
    {
        SVGElement::finishParsing();
        if (isInDocumentScopeAndDocumentParticipateInRendering()) {
            generateStyleSheet();
        }
    }

    void generateStyleSheet();
    void removeStyleSheet();
    CSSStyleSheet* generatedSheet()
    {
        return m_generatedSheet;
    }

    bool hasLoaded()
    {
        return m_loaded;
    }

    void setLoaded()
    {
        m_loaded = true;
    }

private:
    void dispatchLoadEvent();

protected:
    CSSStyleSheet* m_generatedSheet;
    bool m_loaded;
};
}

#endif
