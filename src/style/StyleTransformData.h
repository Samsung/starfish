#ifndef __StarFishStyleTransformData__
#define __StarFishStyleTransformData__

#include "style/Style.h"
#include "style/MatrixTransform.h"
#include "style/ScaleTransform.h"

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
    {
    }

    StyleTransformData(OperationType type)
        : m_type(type)
        , m_matrix(NULL)
        , m_scale(NULL)
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

    MatrixTransform* matrix()
    {
        STARFISH_ASSERT(type() == OperationType::Matrix);
        return m_matrix;
    }

    OperationType type()
    {
        return m_type;
    }

    String* dumpString()
    {
        char temp[100];
        if (m_type == Matrix) {
            sprintf(temp, "matrix(%.3f %.3f %.3f %.3f %.3f %.3f)", m_matrix->a(), m_matrix->b(), m_matrix->c(), m_matrix->d(), m_matrix->e(), m_matrix->f());
        } else if (m_type == Scale) {
            sprintf(temp, "scale(%.3f %.3f)", m_scale->x(), m_scale->y());
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
//    RotateTransform* m_rotate;
//    SkewTransform* m_skew;
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
