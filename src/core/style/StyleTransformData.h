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

#ifndef __StarFishStyleTransformData__
#define __StarFishStyleTransformData__

#include "core/style/Style.h"
#include "core/style/MatrixTransform.h"
#include "core/style/InternalMatrixTransform.h"
#include "core/style/ScaleTransform.h"
#include "core/style/RotateTransform.h"
#include "core/style/SkewTransform.h"
#include "core/style/TranslateTransform.h"

namespace StarFish {

class NativeImageData;
class ComputedStyle;

class StyleTransformData : public gc {
public:
    enum OperationType {
        Matrix,
        Translate,
        Scale,
        Rotate,
        Skew,
        None,
        InternalMatrix
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

    void setInternalMatrix(const SkMatrix& matrix)
    {
        STARFISH_ASSERT(m_type == InternalMatrix);
        if (m_value.m_internalMatrix == NULL) {
            m_value.m_internalMatrix = new InternalMatrixTransform(matrix);
        } else {
            m_value.m_internalMatrix->setMatrix(matrix);
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

    InternalMatrixTransform* internalMatrix() const
    {
        STARFISH_ASSERT(type() == OperationType::InternalMatrix);
        return m_value.m_internalMatrix;
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
            snprintf(temp, sizeof(temp),
                     "matrix(%.3f, %.3f, %.3f, %.3f, %.3f, %.3f) ",
                     matrix()->a(), matrix()->b(), matrix()->c(), matrix()->d(),
                     matrix()->e(), matrix()->f());
        } else if (m_type == Scale) {
            snprintf(temp, sizeof(temp), "scale(%.3f, %.3f) ", scale()->x(),
                     scale()->y());
        } else if (m_type == Rotate) {
            snprintf(temp, sizeof(temp), "rotate(%.3f) ", rotate()->angle());
        } else if (m_type == Skew) {
            snprintf(temp, sizeof(temp), "skew(%.3f, %.3f) ", skew()->angleX(),
                     skew()->angleY());
        } else if (m_type == Translate) {
            auto utf8Data1 =
                translate()->tx().dumpString()->toUTF8NonGCString();
            auto utf8Data2 =
                translate()->ty().dumpString()->toUTF8NonGCString();
            snprintf(temp, sizeof(temp), "translate(%s, %s) ", utf8Data1.data(),
                     utf8Data2.data());
        } else {
            return String::emptyString;
        }
        return String::fromUTF8(temp);
    }

    void changeToFixedIfNeeded(Length curFontSize, Length rootFontSize,
                               Font* font, LayoutSize windowSize,
                               ComputedStyle* cs)
    {
        STARFISH_ASSERT(type() == OperationType::Translate);
        Length x = translate()->tx();
        Length y = translate()->ty();
        x.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                windowSize.width(), windowSize.height(), cs);
        y.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                windowSize.width(), windowSize.height(), cs);
        translate()->setData(x, y);
    }

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(StyleTransformData)] = { 0 };
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(StyleTransformData, m_value));
            descr =
                GC_make_descriptor(obj_bitmap, GC_WORD_LEN(StyleTransformData));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new(size_t size, StyleTransformData* transform)
    {
        return transform;
    }
    void* operator new[](size_t size) = delete;

private:
    friend inline bool operator==(const StyleTransformData& a,
                                  const StyleTransformData& b);
    friend inline bool operator!=(const StyleTransformData& a,
                                  const StyleTransformData& b);

    OperationType m_type;
    union TransformPointer {
        MatrixTransform* m_matrix;
        InternalMatrixTransform* m_internalMatrix;
        TranslateTransform* m_translate;
        ScaleTransform* m_scale;
        RotateTransform* m_rotate;
        SkewTransform* m_skew;
        TransformPointer(MatrixTransform* v)
        {
            m_matrix = v;
        }
    };

    TransformPointer m_value;
};

bool operator==(const StyleTransformData& a, const StyleTransformData& b)
{
    if (a.type() != b.type()) {
        return false;
    }

    switch (a.type()) {
    case StyleTransformData::OperationType::Matrix:
        if (*(a.matrix()) != *(b.matrix())) {
            return false;
        }
        break;
    case StyleTransformData::OperationType::Scale:
        if (*(a.scale()) != *(b.scale())) {
            return false;
        }
        break;
    case StyleTransformData::OperationType::Translate:
        if (*(a.translate()) != *(b.translate())) {
            return false;
        }
        break;
    case StyleTransformData::OperationType::Rotate:
        if (*(a.rotate()) != *(b.rotate())) {
            return false;
        }
        break;
    case StyleTransformData::OperationType::Skew:
        if (*(a.skew()) != *(b.skew())) {
            return false;
        }
        break;
    case StyleTransformData::OperationType::InternalMatrix:
        if (*(a.internalMatrix()) != *(b.internalMatrix())) {
            return false;
        }
        break;
    case StyleTransformData::OperationType::None:
        break;
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    return true;
}

bool operator!=(const StyleTransformData& a, const StyleTransformData& b)
{
    return !operator==(a, b);
}

class StyleTransformDataGroup : public gc {
    friend class StyleResolver;

public:
    StyleTransformDataGroup()
    {
        m_hasComplexTransform = false;
        m_has3DTransform = false;
    }

    ~StyleTransformDataGroup()
    {
    }

    void clear()
    {
        m_hasComplexTransform = false;
        m_has3DTransform = false;
        m_group.clear();
    }

    void reset(StyleTransformDataGroup* other)
    {
        if (!other) {
            clear();
        } else {
            m_hasComplexTransform = other->m_hasComplexTransform;
            m_has3DTransform = other->m_has3DTransform;
            m_group.assign(other->m_group.begin(), other->m_group.end());
        }
    }

    void append(StyleTransformData f)
    {
        m_group.push_back(f);
    }

    void removeAt(size_t idx)
    {
        m_group.erase(idx);
    }

    StyleTransformData at(size_t i) const
    {
        return m_group[i];
    }

    StyleTransformData& at(size_t i)
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

    bool hasComplexTransform()
    {
        return m_hasComplexTransform;
    }

    bool has3DTransform()
    {
        return m_has3DTransform;
    }

private:
    friend inline bool operator==(const StyleTransformDataGroup& a,
                                  const StyleTransformDataGroup& b);
    friend inline bool operator!=(const StyleTransformDataGroup& a,
                                  const StyleTransformDataGroup& b);

    bool m_hasComplexTransform;
    bool m_has3DTransform;
    GCVector<StyleTransformData> m_group;
};

bool operator==(const StyleTransformDataGroup& a,
                const StyleTransformDataGroup& b)
{
    if (a.size() != b.size()) {
        return false;
    }

    for (size_t i = 0; i < a.size(); i++) {
        if (a.at(i) != b.at(i)) {
            return false;
        }
    }

    return true;
}

bool operator!=(const StyleTransformDataGroup& a,
                const StyleTransformDataGroup& b)
{
    return !operator==(a, b);
}
}

#endif
