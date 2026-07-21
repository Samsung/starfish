/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#ifndef __StarfishPathCairo__
#define __StarfishPathCairo__

namespace Starfish {

class Path;
enum class CanvasFillRule;

class PathCairo : public Path {
public:
    PathCairo();
    ~PathCairo();

    cairo_t* context()
    {
        return m_cairoContext;
    }
    void finalize();
    void clearNativeResources();

    void* operator new(size_t size);
    // Objects allocated via GC_finalized_malloc must not be freed with
    // GC_FREE or delete. The no-op operator delete below prevents this.
    void operator delete(void*)
    {
    }
    void operator delete[](void*) = delete;

    virtual void init() override;
    virtual void clear() override;
    virtual bool isEmpty() override;
    virtual void currentPoint(float& x, float& y) override;
    virtual void copy(Path* src) override;
    virtual void append(Path* path) override;

    virtual bool isPointInPath(float x, float y,
                               CanvasFillRule fillRule) override;
    virtual bool isPointInStroke(const StrokeStyle& style, float x,
                                 float y) override;

    // For CanvasPath
    virtual void closePath() override;
    virtual void moveTo(float x, float y) override;
    virtual void lineTo(float x, float y) override;
    virtual void quadraticCurveTo(float cpx, float cpy, float x,
                                  float y) override;
    virtual void bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y,
                               float x, float y) override;
    virtual void arcTo(float x1, float y1, float x2, float y2,
                       float radius) override;
    virtual void rect(float x, float y, float w, float h) override;
    virtual void arc(float x, float y, float radius, float startAngle,
                     float endAngle, bool anticlockwise = false) override;
    virtual void ellipse(float x, float y, float radiusX, float radiusY,
                         float rotation, float startAngle, float endAngle,
                         bool anticlockwise = false) override;
    virtual void postMatrix(const SkMatrix& matrix) override;
    virtual void setCTM(const SkMatrix& matrix) override;
    virtual void translate(float x, float y) override;
    virtual Unit::Rect fillBoundingRect() override;
    virtual Unit::Rect strokeBoundingRect(const StrokeStyle& style) override;
    virtual Unit::Rect boundingRect() override;
    virtual GCAtomicVector<Unit::FloatPoint> pointList() override;

private:
    void notifyBoundingRectDirty()
    {
        m_needsComputeStrokeBoundingRect.strokeWidth =
            std::numeric_limits<float>::quiet_NaN();
        m_needsComputeFillBoundingRect = true;
    }

    void applyStrokeStyle(const StrokeStyle& style);
    void updateBoundingRect();

    StrokeStyle m_needsComputeStrokeBoundingRect; // NaN stroke-width means
                                                  // needs computing
    bool m_needsComputeFillBoundingRect;
    Unit::Rect m_computedStrokeBoundingRect;
    Unit::Rect m_computedFillBoundingRect;
    Unit::Rect m_boundingRect{ FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX };
    cairo_t* m_cairoContext;
    cairo_surface_t* m_dumyCairoSurface;
};
} // namespace Starfish
#endif
