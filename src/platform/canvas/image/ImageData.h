#ifndef __ImageData__
#define __ImageData__

namespace StarFish {

class ImageData : public gc_cleanup {
protected:
    ImageData()
    {

    }
public:
    static ImageData* create(String* imageSrc);
    static ImageData* create(size_t w, size_t h);
    virtual void* unwrap() = 0;
    virtual size_t width() = 0;
    virtual size_t height() = 0;
    virtual void clear() = 0;
    virtual ~ImageData()
    {

    }
};

}

#endif
