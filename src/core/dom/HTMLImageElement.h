/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishHTMLImageElement__
#define __StarFishHTMLImageElement__

#include "core/dom/HTMLElement.h"

namespace StarFish {

class ImageData;
class ImageResource;

class HTMLImageElement : public HTMLElement {
    friend class ImageDownloadClient;

public:
    HTMLImageElement(Document* document)
        : HTMLElement(document)
        , m_imageResource(nullptr)
        , m_imageData(nullptr)
    {
    }

    HTMLImageElement(Document* document, unsigned long width)
        : HTMLImageElement(document)
    {
        setWidth(width);
    }

    HTMLImageElement(Document* document, unsigned long width,
                     unsigned long height)
        : HTMLImageElement(document, width)
    {
        setHeight(height);
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLImageElement() const override;

    /* 4.4 Interface Node */
    virtual QualifiedName name();

    /* HTMLImageElement related */

    void setSrc(String* src);
    String* src();

    unsigned long width();
    void setWidth(unsigned long width);

    unsigned long height();
    void setHeight(unsigned long height);

    String* nameAttr();
    void setNameAttr(String* name);

    ImageData* imageData()
    {
        return m_imageData;
    }

    /* Other methods (not in DOM API) */

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved);
    virtual void didNodeAdopted();

private:
    void unloadImage();
    void loadImage(String* src);
    ImageResource* m_imageResource;
    ImageData* m_imageData;
};
}

#endif
