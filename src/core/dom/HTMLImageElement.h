/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#ifndef __StarfishHTMLImageElement__
#define __StarfishHTMLImageElement__

#include "core/dom/HTMLElement.h"

namespace Starfish {

class NativeImageData;
class ImageResource;
class Document;
class WebOrigin;
class ElementResourceClient;
enum class RequestErrorType;

class HTMLImageElement : public HTMLElement {
    friend class ImageDownloadClient;

public:
    HTMLImageElement(Document* document);
    HTMLImageElement(Document* document, const QualifiedName& qname);
    HTMLImageElement(Document* document, uint32_t width);
    HTMLImageElement(Document* document, uint32_t width, uint32_t height);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLImageElement() const override;

    /* HTMLImageElement related */

    void setSrc(String* src);
    String* src();

    Optional<String*> crossOrigin();
    void setCrossOrigin(Optional<String*> crossOrigin);

    uint32_t width();
    void setWidth(uint32_t width);

    uint32_t height();
    void setHeight(uint32_t height);

    uint32_t naturalWidth();
    uint32_t naturalHeight();

    String* referrerPolicy();
    void setReferrerPolicy(String* policy);

    String* nameAttr();
    void setNameAttr(String* name);

    String* usemap();
    void setUsemap(String* usemap);

    bool complete()
    {
        return !!m_imageData;
    }

    NativeImageData* imageData()
    {
        return m_imageData;
    }

    ImageResource* imageResource()
    {
        return m_imageResource;
    }

    /* Other methods (not in DOM API) */

    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;
    virtual void didNodeAdopted(Document* oldDocument) override;

    virtual void didNodeInsertedToDocumentTree() override;

    WebOrigin* webOrigin();
    bool hasRequestError();

    void updateFrame(size_t delay);

private:
    void unloadImage();
    void loadImage(String* src);
    ResourceURL* origin();

    ImageResource* m_imageResource;
    ElementResourceClient* m_elementResourceClient;
    NativeImageData* m_imageData;
    RequestErrorType m_requestErrorType;

    size_t m_updateFrameTimer{ 0 };
};
} // namespace Starfish

#endif
