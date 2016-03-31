#include "StarFishConfig.h"
#include "ImageData.h"

#include <Elementary.h>

namespace StarFish {

Evas* internalCanvas();

class ImageDataEFL : public ImageData {
public:
    ImageDataEFL(String* imageSrc)
    {
        m_image = evas_object_image_add(internalCanvas());
        evas_object_image_file_set(m_image, imageSrc->utf8Data(), NULL);
        STARFISH_RELEASE_ASSERT(evas_object_image_colorspace_get(m_image) == EVAS_COLORSPACE_ARGB8888);
        int w, h;
        evas_object_image_size_get(m_image, &w, &h);
        m_width = w;
        m_height = h;
    }

    ~ImageDataEFL()
    {
        evas_object_hide(m_image);
        evas_object_del(m_image);
    }

    virtual void* unwrap()
    {
        return m_image;
    }

    virtual size_t width()
    {
        return m_width;
    }

    virtual size_t height()
    {
        return m_height;
    }

    virtual void clear()
    {
        void* address = evas_object_image_data_get(m_image, EINA_TRUE);
        size_t end = m_width * m_height * sizeof(uint32_t);
        memset(address, 0, end);
        evas_object_image_data_set(m_image, address);
    }

protected:
    Evas_Object* m_image;
    size_t m_width;
    size_t m_height;
};

ImageData* ImageData::create(String* imageSrc)
{
    return new ImageDataEFL(imageSrc);
}

}
