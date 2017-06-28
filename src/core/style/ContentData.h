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

#ifndef __StarFishContentData__
#define __StarFishContentData__

namespace StarFish {

class TextContentData {
public:
    TextContentData(String* text)
        : m_text(text)
    {
    }

    String* text()
    {
        return m_text;
    }

    void setText(String* text)
    {
        m_text = text;
    }

private:
    String* m_text;
};

class ImageContentData {
public:
    ImageContentData(String* image)
        : m_image(image)
    {
    }

    String* image()
    {
        return m_image;
    }

    void setImage(String* image)
    {
        m_image = image;
    }

private:
    String* m_image;
};

class ContentData : public gc {
public:
    enum ContentType {
        None,
        Text,
        Image,
    };

    ContentData()
        : m_type(None)
        , m_value(nullptr)
    {
    }

    ContentData(ContentType type)
        : m_type(type)
        , m_value(nullptr)
    {
    }

    ~ContentData()
    {
    }

    ContentType type() const
    {
        return m_type;
    }

    void setType(ContentType type)
    {
        m_type = type;
    }

    bool isText()
    {
        return m_type == Text;
    }

    bool isImage()
    {
        return m_type == Image;
    }

    TextContentData* text() const
    {
        STARFISH_ASSERT(m_type == Text);
        return m_value.m_text;
    }

    void setText(String* text)
    {
        STARFISH_ASSERT(m_type == Text);
        if (!m_value.m_text) {
            m_value.m_text = new TextContentData(text);
        } else {
            m_value.m_text->setText(text);
        }
    }

    ImageContentData* image() const
    {
        STARFISH_ASSERT(m_type == Image);
        return m_value.m_image;
    }

    void setImage(String* image)
    {
        STARFISH_ASSERT(m_type == Image);
        if (!m_value.m_image) {
            m_value.m_image = new ImageContentData(image);
        } else {
            m_value.m_image->setImage(image);
        }
    }

private:
    friend inline bool operator==(const ContentData& a, const ContentData& b);
    friend inline bool operator!=(const ContentData& a, const ContentData& b);

    ContentType m_type;
    union ContentPointer {
        TextContentData* m_text;
        ImageContentData* m_image;
        ContentPointer(TextContentData* v)
        {
            m_text = v;
        }
    };
    ContentPointer m_value;
};

bool operator==(const ContentData& a, const ContentData& b)
{
    if (a.type() != b.type()) {
        return false;
    }

    switch (a.type()) {
    case ContentData::ContentType::Text:
        if (!(a.text()->text()->equals(b.text()->text()))) {
            return false;
        }
        break;
    case ContentData::ContentType::Image:
        if (!(a.image()->image()->equals(b.image()->image()))) {
            return false;
        }
        break;
    default:
        break;
    }

    return true;
}

bool operator!=(const ContentData& a, const ContentData& b)
{
    return !operator==(a, b);
}

typedef GCVector<ContentData> ContentDataGroup;

inline bool operator==(const ContentDataGroup& a, const ContentDataGroup& b)
{
    if (a.size() != b.size()) {
        return false;
    }

    for (size_t i = 0; i < a.size(); i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }

    return true;
}

inline bool operator!=(const ContentDataGroup& a, const ContentDataGroup& b)
{
    return !operator==(a, b);
}

} /* namespace StarFish */

#endif /* __StarFishContentData__ */
