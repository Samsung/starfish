#ifndef __StarFishStyleTransformData__
#define __StarFishStyleTransformData__

#include "style/Style.h"
#include "style/MatrixTransform.h"

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
    {
    }

    StyleTransformData(OperationType type)
        : m_type(type)
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

    OperationType type()
    {
        return m_type;
    }

    String* dumpString()
    {
        char temp[100];
        if (m_type == Matrix) {
            sprintf(temp, "matrix(%.3f %.3f %.3f %.3f %.3f %.3f)", m_matrix->a(), m_matrix->b(), m_matrix->c(), m_matrix->d(), m_matrix->e(), m_matrix->f());    
        }
        // TODO
        return String::fromUTF8(temp);
    }

private:
    OperationType m_type;
    // TODO m_origin;
    MatrixTransform* m_matrix;
    // TODO
//    TranslateTransform* m_translate;
//    ScaleTransform* m_scale;
//    RotateTransform* m_rotate;
//    SkewTransform* m_skew;
};
}

#endif
