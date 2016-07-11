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

#ifndef __StarFishStyleTransformData__
#define __StarFishStyleTransformData__

#include "style/Style.h"
#include "style/MatrixTransform.h"
#include "style/ScaleTransform.h"
#include "style/RotateTransform.h"
#include "style/SkewTransform.h"
#include "style/TranslateTransform.h"

namespace StarFish {

class ImageData;

class StyleTransformData : public gc {
public:
    enum OperationType {
        Matrix,
        Translate,
        Scale,
        Rotate,
        Skew,
        None
    };

    StyleTransformData()
        : m_type(None)
        , m_value(NULL)
    {
    }

    StyleTransformData(OperationType type)
        : m_type(type)
        , m_value(NULL)
    {
    }

    ~StyleTransformData()
    {
    }

    void setType(OperationType type)
    {
        m_type = type;
    }

    void setMatrix(double a, double b, double c, double d, double e, double f)
    {
        STARFISH_ASSERT(m_type == Matrix);
        if (m_value.m_matrix == NULL) {
            m_value.m_matrix = new MatrixTransform(a, b, c, d, e, f);
        } else {
            m_value.m_matrix->setData(a, b, c, d, e, f);
        }
    }

    void setScale(double a, double b)
    {
        STARFISH_ASSERT(m_type == Scale);
        if (m_value.m_scale == NULL) {
            m_value.m_scale = new ScaleTransform(a, b);
        } else {
            m_value.m_scale->setData(a, b);
        }
    }

    void setRotate(double ang)
    {
        STARFISH_ASSERT(m_type == Rotate);
        if (m_value.m_rotate == NULL) {
            m_value.m_rotate = new RotateTransform(ang);
        } else {
            m_value.m_rotate->setData(ang);
        }
    }

    void setSkew(double angX, double angY)
    {
        STARFISH_ASSERT(m_type == Skew);
        if (m_value.m_skew == NULL) {
            m_value.m_skew = new SkewTransform(angX, angY);
        } else {
            m_value.m_skew->setData(angX, angY);
        }
    }

    void setTranslate(Length x, Length y)
    {
        STARFISH_ASSERT(m_type == Translate);
        if (m_value.m_translate == NULL) {
            m_value.m_translate = new TranslateTransform(x, y);
        } else {
            m_value.m_translate->setData(x, y);
        }
    }

    MatrixTransform* matrix() const
    {
        STARFISH_ASSERT(type() == OperationType::Matrix);
        return m_value.m_matrix;
    }

    TranslateTransform* translate() const
    {
        STARFISH_ASSERT(type() == OperationType::Translate);
        return m_value.m_translate;
    }

    ScaleTransform* scale() const
    {
        STARFISH_ASSERT(type() == OperationType::Scale);
        return m_value.m_scale;
    }

    RotateTransform* rotate() const
    {
        STARFISH_ASSERT(type() == OperationType::Rotate);
        return m_value.m_rotate;
    }

    SkewTransform* skew() const
    {
        STARFISH_ASSERT(type() == OperationType::Skew);
        return m_value.m_skew;
    }

    OperationType type() const
    {
        return m_type;
    }

    String* dumpString()
    {
        char temp[100];
        if (m_type == Matrix) {
            snprintf(temp, sizeof temp, "matrix(%.3f %.3f %.3f %.3f %.3f %.3f) ", matrix()->a(), matrix()->b(), matrix()->c(), matrix()->d(), matrix()->e(), matrix()->f());
        } else if (m_type == Scale) {
            snprintf(temp, sizeof temp, "scale(%.3f %.3f) ", scale()->x(), scale()->y());
        } else if (m_type == Rotate) {
            snprintf(temp, sizeof temp, "rotate(%.3f) ", rotate()->angle());
        } else if (m_type == Skew) {
            snprintf(temp, sizeof temp, "skew(%.3f %.3f) ", skew()->angleX(), skew()->angleY());
        } else if (m_type == Translate) {
            snprintf(temp, sizeof temp, "translate(%s %s) ", translate()->tx().dumpString()->utf8Data(), translate()->ty().dumpString()->utf8Data());
        } else {
            return String::emptyString;
        }
        return String::fromUTF8(temp);
    }

    void changeToFixedIfNeeded(Length fontSize, Font* font)
    {
        STARFISH_ASSERT(type() == OperationType::Translate);
        Length x = translate()->tx();
        Length y = translate()->ty();
        x.changeToFixedIfNeeded(fontSize, font);
        y.changeToFixedIfNeeded(fontSize, font);
        translate()->setData(x, y);
    }

private:
    friend inline bool operator==(const StyleTransformData& a, const StyleTransformData& b);
    friend inline bool operator!=(const StyleTransformData& a, const StyleTransformData& b);

    OperationType m_type;
    union TransformPointer {
        MatrixTransform* m_matrix;
        TranslateTransform* m_translate;
        ScaleTransform* m_scale;
        RotateTransform* m_rotate;
        SkewTransform* m_skew;
        TransformPointer(MatrixTransform* v) { m_matrix = v; }
    };

    TransformPointer m_value;
};

bool operator==(const StyleTransformData& a, const StyleTransformData& b)
{
    if (a.type() != b.type())
        return false;

    switch (a.type()) {
    case StyleTransformData::OperationType::Matrix:
        if (*(a.matrix()) != *(b.matrix()))
            return false;
        break;
    case StyleTransformData::OperationType::Scale:
        if (*(a.scale()) != *(b.scale()))
            return false;
        break;
    case StyleTransformData::OperationType::Translate:
        if (*(a.translate()) != *(b.translate()))
            return false;
        break;
    case StyleTransformData::OperationType::Rotate:
        if (*(a.rotate()) != *(b.rotate()))
            return false;
        break;
    case StyleTransformData::OperationType::Skew:
        if (*(a.skew()) != *(b.skew()))
            return false;
        break;
    case StyleTransformData::OperationType::None:
    default:
        break;
    }
    return true;
}

bool operator!=(const StyleTransformData& a, const StyleTransformData& b)
{
    return !operator==(a, b);
}

class StyleTransformDataGroup : public gc {
public:
    StyleTransformDataGroup()
    {
    }

    ~StyleTransformDataGroup()
    {
    }

    void append(StyleTransformData f)
    {
        m_group.push_back(f);
    }

    StyleTransformData at(int i) const
    {
        return m_group[i];
    }

    size_t size() const
    {
        return m_group.size();
    }

    String* dumpString()
    {
        String* str = String::emptyString;
        for (size_t i = 0; i < size(); i++) {
            str = str->concat(at(i).dumpString());
        }
        return str;
    }

private:
    friend inline bool operator==(const StyleTransformDataGroup& a, const StyleTransformDataGroup& b);
    friend inline bool operator!=(const StyleTransformDataGroup& a, const StyleTransformDataGroup& b);

    std::vector<StyleTransformData, gc_allocator<StyleTransformData> > m_group;
};

bool operator==(const StyleTransformDataGroup& a, const StyleTransformDataGroup& b)
{
    if (a.size() != b.size())
        return false;

    for (size_t i = 0; i < a.size(); i++) {
        if (a.at(i) != b.at(i))
            return false;
    }

    return true;
}

bool operator!=(const StyleTransformDataGroup& a, const StyleTransformDataGroup& b)
{
    return !operator==(a, b);
}

}

#endif
