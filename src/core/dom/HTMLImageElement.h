/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#ifndef __StarFishHTMLImageElement__
#define __StarFishHTMLImageElement__

#include "core/dom/HTMLElement.h"

namespace StarFish {

class NativeImageData;
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

    String* referrerPolicy();
    void setReferrerPolicy(String* policy);

    String* nameAttr();
    void setNameAttr(String* name);

    NativeImageData* imageData()
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
    NativeImageData* m_imageData;
};
}

#endif
