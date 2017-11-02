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

#ifndef __StarFishCompositor__
#define __StarFishCompositor__

namespace StarFish {

class ImageData;
class PlatformWindow;
class Canvas;
class CanvasSurface;

class Compositor : public gc {
protected:
    Compositor()
    {
    }

public:
    static Canvas* createCanvasAdaptor(Compositor* compositor);
    static Compositor* create(StarFish* starfish, void* data);
    static Compositor* create(StarFish* starfish, CanvasSurface* surface);

    virtual ~Compositor()
    {
    }

    virtual void clearColor(const Unit::Color& clr) = 0;

    // state
    virtual void save() = 0;    // push state on state stack
    virtual void restore() = 0; // pop state stack and restore state
    // transformations (default transform is the identity matrix)
    virtual void scale(double x, double y) = 0;
    virtual void rotate(double angle) = 0;
    virtual void translate(double x, double y) = 0;
    virtual void translate(LayoutUnit x, LayoutUnit y) = 0;
    virtual void postMatrix(const SkMatrix& matrix) = 0;

    virtual void clip(const Unit::Rect& rt) = 0;

    // reset transform matrix & clip
    virtual void resetMatrixAndClip() = 0;
    // reset transform clip
    virtual void resetClip() = 0;

    virtual void setColor(const Unit::Color& clr) = 0;
    virtual void beginOpacityLayer(float c) = 0;
    virtual void endOpacityLayer() = 0;

    virtual void drawRect(const Unit::Rect& rt) = 0;
    virtual void drawRect(const LayoutRect& rt) = 0;
    virtual void punchHole(const Unit::Rect& rt) = 0;

    virtual void drawSurface(CanvasSurface* data, const Unit::Rect& dst) = 0;
    virtual void drawImage(ImageData* data, const Unit::Rect& dst) = 0;
    virtual void drawRepeatImage(ImageData* data, const Unit::Rect& dst,
                                 float imageWidth, float imageHeight,
                                 bool xRepeat, bool yRepeat) = 0;

    virtual void applyMatrixTo(LayoutLocation& lp) = 0;
    virtual void applyMatrixTo(LayoutRect& lp) = 0;
};
}

#endif
