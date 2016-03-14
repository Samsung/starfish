#ifndef __StarFishStyleTransformData__
#define __StarFishStyleTransformData__

#include "style/Style.h"
#include "style/MatrixTransform.h"
#include "style/ScaleTransform.h"
#include "style/RotateTransform.h"
#include "style/SkewTransform.h"

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
        , m_matrix(NULL)
        , m_scale(NULL)
        , m_rotate(NULL)
        , m_skew(NULL)
    {
    }

    StyleTransformData(OperationType type)
        : m_type(type)
        , m_matrix(NULL)
        , m_scale(NULL)
        , m_rotate(NULL)
        , m_skew(NULL)
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
        if (m_matrix == NULL) {
            m_matrix = new MatrixTransform(a, b, c, d, e, f);
        } else {
            m_matrix->setData(a, b, c, d, e, f);
        }
    }

    void setScale(double a, double b)
    {
        STARFISH_ASSERT(m_type == Scale);
        if (m_scale == NULL) {
            m_scale = new ScaleTransform(a, b);
        } else {
            m_scale->setData(a, b);
        }
    }

    void setRotate(double ang)
    {
        STARFISH_ASSERT(m_type == Rotate);
        if (m_rotate == NULL) {
            m_rotate = new RotateTransform(ang);
        } else {
            m_rotate->setData(ang);
        }
    }

    void setSkew(double angX, double angY)
    {
        STARFISH_ASSERT(m_type == Skew);
        if (m_skew == NULL) {
            m_skew = new SkewTransform(angX, angY);
        } else {
            m_skew->setData(angX, angY);
        }
    }

    MatrixTransform* matrix()
    {
        STARFISH_ASSERT(type() == OperationType::Matrix);
        return m_matrix;
    }

    ScaleTransform* scale()
    {
        STARFISH_ASSERT(type() == OperationType::Scale);
        return m_scale;
    }

    RotateTransform* rotate()
    {
        STARFISH_ASSERT(type() == OperationType::Rotate);
        return m_rotate;
    }

    SkewTransform* skew()
    {
        STARFISH_ASSERT(type() == OperationType::Skew);
        return m_skew;
    }

    OperationType type()
    {
        return m_type;
    }

    String* dumpString()
    {
        char temp[100];
        if (m_type == Matrix) {
            sprintf(temp, "matrix(%.3f %.3f %.3f %.3f %.3f %.3f) ", m_matrix->a(), m_matrix->b(), m_matrix->c(), m_matrix->d(), m_matrix->e(), m_matrix->f());
        } else if (m_type == Scale) {
            sprintf(temp, "scale(%.3f %.3f) ", m_scale->x(), m_scale->y());
        } else if (m_type == Rotate) {
            sprintf(temp, "rotate(%.3f) ", m_rotate->angle());
        } else if (m_type == Skew) {
            sprintf(temp, "skew(%.3f %.3f) ", m_skew->angleX(), m_skew->angleY());
        }
        // TODO
        return String::fromUTF8(temp);
    }

private:
    OperationType m_type;
    // TODO m_origin;
    // TODO save pointers in union
    MatrixTransform* m_matrix;
    // TODO
//    TranslateTransform* m_translate;
    ScaleTransform* m_scale;
    RotateTransform* m_rotate;
    SkewTransform* m_skew;
};

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

    StyleTransformData at(int i)
    {
        return m_group[i];
    }

    size_t size()
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
    std::vector<StyleTransformData, gc_allocator<StyleTransformData> > m_group;
};

}

#endif
