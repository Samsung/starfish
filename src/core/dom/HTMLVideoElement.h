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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && \
    !defined(__StarFishHTMLVideoElement__)
#define __StarFishHTMLVideoElement__

#include "core/dom/HTMLMediaElement.h"

namespace StarFish {

class HTMLVideoElement : public HTMLMediaElement {
public:
    HTMLVideoElement(Document* document)
        : HTMLMediaElement(document)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLVideoElement() const override;

    virtual QualifiedName name() override;

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    uint32_t width();
    void setWidth(uint32_t width);

    uint32_t height();
    void setHeight(uint32_t height);

    uint32_t videoWidth();
    uint32_t videoHeight();
    String* poster();
    void setPoster(String* poster);

protected:
};
}

#endif
