#ifndef __Canvas__
#define __Canvas__

namespace StarFish {

class ImageData;
class Font;

class Canvas : public gc {
protected:
    Canvas()
    {

    }
public:
    static Canvas* createDirect(void* data);

    virtual ~Canvas()
    {

    }

    virtual void clearColor(const Color& clr) = 0;

    // state
    virtual void save() = 0; // push state on state stack
    virtual void restore() = 0; // pop state stack and restore state
    // transformations (default transform is the identity matrix)
    virtual void scale(double x, double y) = 0;
    virtual void scale(double x, double y, double ox, double oy) = 0;
    virtual void rotate(double angle) = 0;
    virtual void rotate(double angle, double ox, double oy) = 0;
    virtual void translate(double x, double y) = 0;
    virtual void translate(LayoutUnit x, LayoutUnit y) = 0;

    virtual void clip(const Rect& rt) = 0;

    virtual void setColor(const Color& clr) = 0;
    virtual Color color() = 0;
    virtual void beginOpacityLayer(float c) = 0;
    virtual void endOpacityLayer() = 0;
    virtual void setFont(Font* font) = 0;
    virtual void setNeedsUnderline(bool b) = 0;
    virtual void setNeedsLineThrough(bool b) = 0;
    virtual void setUnderlineColor(Color clr) = 0;
    virtual void setLineThroughColor(Color clr) = 0;
    virtual void drawRect(const Rect& rt) = 0;
    virtual void drawRect(const LayoutRect& rt) = 0;
    virtual void drawText(LayoutUnit x, LayoutUnit y, String* text) = 0;
    virtual void drawImage(ImageData* data, const Rect& dst) = 0;

    virtual void setMatrix(const SkMatrix& matrix) = 0;
    virtual SkMatrix matrix() = 0;

    virtual void setVisible(bool visible) = 0;

    virtual void* unwrap() = 0;
};

}

#endif
