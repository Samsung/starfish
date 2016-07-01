/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#include "dom/HTMLElement.h"
#include "loader/ImageResource.h"

namespace StarFish {

class HTMLImageElement : public HTMLElement {
    friend class ImageDownloadClient;
public:
    HTMLImageElement(Document* document)
        : HTMLElement(document)
        , m_imageResource(nullptr)
        , m_imageData(nullptr)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */

    virtual String* localName()
    {
        return document()->window()->starFish()->staticStrings()->m_imgTagName.localName();
    }

    virtual QualifiedName name()
    {
        return document()->window()->starFish()->staticStrings()->m_imgTagName;
    }

    /* HTMLImageElement related */

    void setSrc(String* src)
    {
        setAttribute(document()->window()->starFish()->staticStrings()->m_src, src);
    }

    String* src()
    {
        return getAttribute(document()->window()->starFish()->staticStrings()->m_src);
    }

    String* width()
    {
        return getAttribute(document()->window()->starFish()->staticStrings()->m_width);
    }

    void setWidth(int width)
    {
        setAttribute(document()->window()->starFish()->staticStrings()->m_width, String::fromInt(width));
    }

    String* height()
    {
        return getAttribute(document()->window()->starFish()->staticStrings()->m_height);
    }

    void setHeight(int height)
    {
        setAttribute(document()->window()->starFish()->staticStrings()->m_height, String::fromInt(height));
    }

    std::pair<Length, Length> intrinsicSize()
    {
        if (m_imageData) {
            String* widthString = width();
            String* heightString = height();
            bool widthIsEmpty = widthString->equals(String::emptyString);
            bool heightIsEmpty = heightString->equals(String::emptyString);
            if (widthIsEmpty && heightIsEmpty) {
                return std::make_pair(Length(Length::Fixed, m_imageData->width()), Length(Length::Fixed, m_imageData->height()));
            } else if (widthIsEmpty) {
                int h = String::parseInt(heightString);
                bool heightIsPercent = heightString->lastIndexOf('%') == heightString->length() - 1;
                Length height = heightIsPercent? Length(Length::Percent, (float)h / 100) : Length(Length::Fixed, h);
                return std::make_pair(height, height);
            } else if (heightIsEmpty) {
                int w = String::parseInt(widthString);
                bool widthIsPercent = widthString->lastIndexOf('%') == widthString->length() - 1;
                Length width = widthIsPercent? Length(Length::Percent, (float)w / 100) : Length(Length::Fixed, w);
                return std::make_pair(width, width);
            } else {
                int w = String::parseInt(widthString);
                int h = String::parseInt(heightString);
                bool heightIsPercent = heightString->lastIndexOf('%') == heightString->length() - 1;
                bool widthIsPercent = widthString->lastIndexOf('%') == widthString->length() - 1;
                Length width = widthIsPercent? Length(Length::Percent, (float)w / 100) : Length(Length::Fixed, w);
                Length height = heightIsPercent? Length(Length::Percent, (float)h / 100) : Length(Length::Fixed, h);
                return std::make_pair(width, height);
            }
        } else {
            return std::make_pair(Length(Length::Fixed, 0), Length(Length::Fixed, 0));
        }
    }

    ImageData* imageData()
    {
        return m_imageData;
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLImageElement() const
    {
        return true;
    }

    virtual void didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved);

private:
    ImageResource* m_imageResource;
    ImageData* m_imageData;
};

}

#endif
