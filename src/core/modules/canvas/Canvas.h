/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishCanvas__
#define __StarfishCanvas__

#define STARFISH_CANVAS_LENGTH_MAX 65535

#include <SkMatrix.h>

#include "core/modules/canvas/TextDecorationData.h"
#include "core/modules/canvas/CanvasFillStrokeSource.h"
#include "core/modules/canvas/CanvasShadowData.h"
#include "core/modules/canvas/image/SVGNativeImageData.h"

namespace Starfish {

class Frame;
class NativeImageData;
class PlatformWindow;
class NativeGradient;
class Path;

struct GradientDrawingInfo;

enum class CanvasLineCap;
enum class CanvasLineJoin;
enum class CanvasTextAlign;
enum class CanvasTextBaseline;
enum class CanvasDirection;
enum class ImageSmoothingQuality;

// https://drafts.fxtf.org/compositing/#compositemode

enum class CanvasCompositeOperator {
    Clear,
    Copy,
    SourceOver,
    DestinationOver,
    SourceIn,
    DestinationIn,
    SourceOut,
    DestinationOut,
    SourceAtop,
    DestinationAtop,
    XOR,
    Lighter,
    PlusDarker,
    PlusLighter
};

// https://drafts.fxtf.org/compositing/#ltblendmodegt
enum class CanvasBlendMode {
    Normal,
    Multiply,
    Screen,
    Overlay,
    Darken,
    Lighten,
    ColorDodge,
    ColorBurn,
    HardLight,
    SoftLight,
    Difference,
    Exclusion,
    Hue,
    Saturation,
    Color,
    Luminosity
};

namespace CanvasCompositing {

    static const char* const canvasCompositeOperatorNames[] = {
        "clear",       "copy",
        "source-over", "destination-over",
        "source-in",   "destination-in",
        "source-out",  "destination-out",
        "source-atop", "destination-atop",
        "xor",         "lighter",
        "plus-darker", "plus-lighter"
    };

    static const char* const canvasBlendModeNames[] = {
        "normal",     "multiply",   "screen",      "overlay",
        "darken",     "lighten",    "color-dodge", "color-burn",
        "hard-light", "soft-light", "difference",  "exclusion",
        "hue",        "saturation", "color",       "luminosity"
    };

    const int sizeOfCanvasCompositeOperatorNames =
        sizeof(canvasCompositeOperatorNames) /
        sizeof(*canvasCompositeOperatorNames);
    const int sizeOfCanvasBlendModeNames =
        sizeof(canvasBlendModeNames) / sizeof(*canvasBlendModeNames);
} // namespace CanvasCompositing

class CanvasState : public gc {
public:
    CanvasState();
    void* operator new(size_t size)
    {
        STARFISH_ASSERT(size == sizeof(CanvasState));
        static bool typeInited = false;
        static GC_descr descr;
        if (typeInited == false) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(CanvasState)] = { 0 };
            CanvasState::fillGCDescriptor(obj_bitmap);
            descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(CanvasState));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

    CanvasFillStrokeSource* m_fillSource;
    CanvasFillStrokeSource* m_strokeSource;
    float m_layerOpacity;
    Font* m_font;
    TextDecorationData m_textDecorationData;
    SkMatrix m_pathTM;
    float m_globalAlpha;
    CanvasCompositeOperator m_compositeOperator;
    CanvasBlendMode m_blendMode;
    double m_dashOffset;
    std::vector<double> m_dashes;
    CanvasTextAlign m_canvasTextAlign;
    CanvasTextBaseline m_canvasTextBaseline;
    CanvasDirection m_canvasDirection;
    String* m_canvasFontOrginalStr;
    bool m_imageSmoothingEnabled;
    ImageSmoothingQuality m_imageSmoothingQuality;
    size_t m_canvasFontState;
    bool m_visible;
    bool m_hasNonInvertableCTM;
    CanvasShadowData m_shadowData;

    void* m_maskPattern;
    void* m_maskPatternData;
    bool m_shouldRemoveImmediately;
    SkMatrix m_maskTM;

protected:
    static inline void fillGCDescriptor(GC_word* obj_bitmap)
    {
        STARFISH_ASSERT(obj_bitmap != nullptr);

        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(CanvasState, m_fillSource));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(CanvasState, m_strokeSource));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(CanvasState, m_font));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(CanvasState, m_canvasFontOrginalStr));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(CanvasState, m_maskPatternData));
    }
};

class CanvasSurface : public gc {
public:
    virtual ~CanvasSurface()
    {
    }

    enum CanvasSurfaceFlag {
        PlainElement = 0,
        ElementHasFilterEffect = 1,
        CanvasElement = 1 << 1
    };
    static CanvasSurface* create(PlatformWindow* window, size_t w, size_t h,
                                 float additionalPixelRatio = 1,
                                 CanvasSurfaceFlag flag = PlainElement);
    static CanvasSurface* createCanvasTarget(uint8_t* buffer, size_t w,
                                             size_t h, size_t stride);

    virtual bool attachNativeBuffer(
        size_t w, size_t h,
        CanvasSurfaceFlag flag = PlainElement) = 0; // returns surface updated
    virtual void detachNativeBuffer() = 0;

    struct MappedNativeBuffer {
        uint8_t* m_bufferAddress;
        size_t m_mappedBufferX;
        size_t m_mappedBufferY;
        size_t m_mappedBufferWidth;
        size_t m_mappedBufferHeight;
        size_t m_mappedBufferStride;
    };

    // this function try to map area you specified. but not every port can map
    // area you specified.
    // so you should look {x, y, width, height} of return value
    virtual MappedNativeBuffer mapBuffer(size_t bufferX, size_t bufferY,
                                         size_t bufferWidth,
                                         size_t bufferHeight) = 0;
    uint8_t* mapBuffer() // maps whole area
    {
        MappedNativeBuffer m = mapBuffer(0, 0, bufferWidth(), bufferHeight());
        STARFISH_ASSERT(m.m_mappedBufferStride == bufferStride());
        STARFISH_ASSERT(m.m_mappedBufferWidth == bufferWidth());
        STARFISH_ASSERT(m.m_mappedBufferHeight == bufferHeight());
        STARFISH_ASSERT(m.m_mappedBufferX == 0);
        STARFISH_ASSERT(m.m_mappedBufferY == 0);
        return m.m_bufferAddress;
    }
    virtual void unmapBufferAndNotifyUpdatedRegion(size_t x, size_t y, size_t w,
                                                   size_t h)
    {
    }

    virtual size_t width() = 0;
    virtual size_t height() = 0;

    virtual size_t bufferWidth() = 0;
    virtual size_t bufferHeight() = 0;
    virtual size_t bufferStride() = 0;

    virtual void attachPlatformExternalBuffer(void* buffer)
    {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    float additionalPixelRatio()
    {
        return m_additionalPixelRatio;
    }

    static size_t g_totalAllocatedCanvasSurfaceSize;
    static size_t g_canvasSurfaceTileSize;

protected:
    CanvasSurface(float additionalPixelRatio)
        : m_additionalPixelRatio(additionalPixelRatio)
    {
    }

    float m_additionalPixelRatio;
};

namespace CanvasSurfaceFactory {
#ifdef PORT_WINDOW_BACKEND_GL
    CanvasSurface* createGl(PlatformWindow* window, size_t w, size_t h,
                            float additionalPixelRatio,
                            CanvasSurface::CanvasSurfaceFlag flag);
#endif
#if defined(PORT_WINDOW_BACKEND_GB) || defined(PORT_WINDOW_BACKEND_HEADLESS)
    CanvasSurface* createSimple(PlatformWindow* window, size_t w, size_t h,
                                float additionalPixelRatio,
                                CanvasSurface::CanvasSurfaceFlag flag);
#endif
    CanvasSurface* createCanvasTargetSimple(uint8_t* buffer, size_t w, size_t h,
                                            size_t stride);
}; // namespace CanvasSurfaceFactory

struct DrawImageInfo {
    double hScale;
    double vScale;
    BorderImageRepeatValue hRepeat;
    BorderImageRepeatValue vRepeat;
};

struct CanvasRenderTargetInfo {
    uint8_t* m_buffer;
    size_t m_width;
    size_t m_height;
    size_t m_stride;

    CanvasRenderTargetInfo(uint8_t* buffer = nullptr, size_t width = 0,
                           size_t height = 0, size_t stride = 0)
        : m_buffer(buffer)
        , m_width(width)
        , m_height(height)
        , m_stride(stride)
    {
    }
};

class Canvas : public gc {
protected:
    Canvas()
    {
    }

public:
    enum CanvasFlag {
        PlainElement = 0,
        CanvasElement = 1,
    };
    static Canvas* create(WebView* webView, CanvasSurface* data,
                          CanvasFlag flag = PlainElement);
    static Canvas* create(WebView* webView, uint8_t* data, size_t w, size_t h,
                          size_t stride);
    static Canvas* create(WebView* webView, NativeImageData* data);

    virtual ~Canvas()
    {
    }

    virtual void clearColor(const Unit::Color& clr) = 0;
    virtual void flush() = 0;
    // state
    virtual void save();    // push state on state stack
    virtual void restore(); // pop state stack and restore state
    // transformations (default transform is the identity matrix)
    virtual void scale(double x, double y) = 0;
    virtual void rotate(double angle) = 0;
    virtual void translate(double x, double y) = 0;
    virtual void translate(LayoutUnit x, LayoutUnit y) = 0;
    virtual void postMatrix(const SkMatrix& matrix) = 0;
    virtual void setMatrix(const SkMatrix& matrix) = 0;
    virtual SkMatrix currentTransformMatrix() = 0;

    virtual CanvasLineCap lineCap() = 0;
    virtual void setLineCap(CanvasLineCap lineCap) = 0;
    virtual CanvasLineJoin lineJoine() = 0;
    virtual void setLineJoin(CanvasLineJoin lineJoin) = 0;
    virtual double miterLimit() = 0;
    virtual void setMiterLimit(double limit) = 0;

    virtual double shadowOffsetX() = 0;
    virtual void setShadowOffsetX(double offset) = 0;
    virtual double shadowOffsetY() = 0;
    virtual void setShadowOffsetY(double offset) = 0;
    virtual double shadowBlur() = 0;
    virtual void setShadowBlur(double blur) = 0;
    virtual Unit::Color shadowColor() = 0;
    virtual void setShadowColor(const Unit::Color& color) = 0;

    virtual bool imageSmoothingEnabled() = 0;
    virtual void setImageSmoothingEnabled(bool value) = 0;
    virtual ImageSmoothingQuality imageSmoothingQuality() = 0;
    virtual void setImageSmoothingQuality(ImageSmoothingQuality quality) = 0;

    virtual void clip(const Unit::Rect& rt) = 0;
    virtual LayoutRect pixelSnappedClip(const LayoutRect& rt)
    {
        clip(Unit::Rect(rt.x(), rt.y(), rt.width(), rt.height()));
        return rt;
    }

    virtual void unsetDevicePixelRatio() = 0;

    // reset transform matrix & clip
    virtual void resetMatrixAndClip(bool needsApplyDPR = true) = 0;
    // reset transform clip
    virtual void resetClip() = 0;
    virtual void resetMatrix(bool needsApplyDPR = true) = 0;

    virtual void setFillColor(const Unit::Color& clr) = 0;
    virtual void setFillSource(CanvasFillStrokeSource* source) = 0;
    virtual CanvasFillStrokeSource* fillSource() = 0;

    virtual void setStrokeColor(const Unit::Color& clr) = 0;
    virtual void setStrokeSource(CanvasFillStrokeSource* source) = 0;
    virtual CanvasFillStrokeSource* strokeSource() = 0;

    virtual void setGlobalAlpha(float c) = 0;
    virtual float globalAlpha() = 0;

    virtual void setCompositeOperator(CanvasCompositeOperator oper,
                                      CanvasBlendMode mode) = 0;
    virtual CanvasCompositeOperator compositeOperator() = 0;
    virtual CanvasBlendMode blendMode() = 0;

    virtual void beginOpacityLayer(float c) = 0;
    virtual void endOpacityLayer() = 0;
    virtual void setFont(Font* font) = 0;
    virtual void resetTextDecorationData() = 0;
    virtual void mergeTextDecorationData(ComputedStyle* style) = 0;
    virtual TextDecorationData textDecorationData() = 0;
    virtual void setTextDecorationData(TextDecorationData d) = 0;
    virtual void drawRect(const Unit::Rect& rt) = 0;
    virtual void drawRect(const LayoutRect& rt) = 0;
    virtual void strokeRect(const Unit::Rect& rt) = 0;
    virtual void strokeRect(const LayoutRect& rt) = 0;
    void drawPixelSnappedRect(const LayoutRect& rt)
    {
#if defined(STARFISH_ENABLE_TEST)
        // retain old method for our test suite
        LayoutUnit rx = rt.x();
        LayoutUnit ry = rt.y();
        int xx = rx.floor();
        int yy = ry.floor();
        int ww = snapSizeToPixel(rt.width(), rx);
        int hh = snapSizeToPixel(rt.height(), ry);
        drawRect(LayoutRect(xx, yy, ww, hh));
#else
        auto ctm = currentTransformMatrix();
        auto tp = ctm.getType();
        if (!(tp & SkMatrix::TypeMask::kScale_Mask) &&
            !(tp & SkMatrix::TypeMask::kAffine_Mask) &&
            !(tp & SkMatrix::TypeMask::kPerspective_Mask)) {
            save();
            LayoutUnit tx = ctm.getTranslateX();
            LayoutUnit ty = ctm.getTranslateY();
            resetMatrix();
            LayoutRect newRt(rt.x() + tx, rt.y() + ty, rt.width(), rt.height());
            drawRect(newRt.snapSizeToPixel());
            restore();
        } else {
            drawRect(rt);
        }
#endif
    }
    virtual void drawRect(LayoutLocation p1, LayoutLocation p2,
                          LayoutLocation p3,
                          LayoutLocation p4) = 0; // left, top, right, bottom

    virtual void setDash(const std::vector<double>& dashes) = 0;
    virtual std::vector<double> dash() = 0;
    virtual double dashOffset() = 0;
    virtual void setDashOffset(double offset) = 0;

    virtual void punchHole(const Unit::Rect& rt) = 0;

    virtual void drawText(LayoutUnit x, LayoutUnit y, LayoutUnit stringWidth,
                          const StringView& text,
                          bool shouldSkipUnresolvedWebFont = true) = 0;
    virtual void drawStrokeText(LayoutUnit x, LayoutUnit y,
                                LayoutUnit stringWidth, const StringView& text,
                                bool shouldSkipUnresolvedWebFont = true) = 0;

    void drawImage(NativeImageData* data, const Unit::Rect& dst,
                   ImageRenderingValue imageRenderingMode =
                       ImageRenderingValue::ImageRenderingAutoValue)
    {
        if (data->isSVGNativeImageData()) {
            data->asSVGNativeImageData()->paintContent(this, dst,
                                                       imageRenderingMode);
        } else {
            drawNativeImageData(data, dst, imageRenderingMode);
        }
    }
    void drawImage(NativeImageData* data, const Unit::Rect& src,
                   const Unit::Rect& dst, const DrawImageInfo& borderinfo,
                   ImageRenderingValue imageRenderingMode =
                       ImageRenderingValue::ImageRenderingAutoValue)
    {
        if (data->isSVGNativeImageData()) {
            data->asSVGNativeImageData()->paintContent(
                this, src, dst, borderinfo, imageRenderingMode);
        } else {
            drawNativeImageData(data, src, dst, borderinfo, imageRenderingMode);
        }
    }

    void drawRepeatImage(NativeImageData* data, const Unit::Rect& dst,
                         float imageWidth, float imageHeight, bool xRepeat,
                         bool yRepeat,
                         ImageRenderingValue imageRenderingMode =
                             ImageRenderingValue::ImageRenderingAutoValue)
    {
        if (data->isSVGNativeImageData()) {
            data->asSVGNativeImageData()->paintRepeatContent(
                this, dst, imageWidth, imageHeight, xRepeat, yRepeat,
                imageRenderingMode);
        } else {
            drawRepeatNativeImageData(data, dst, imageWidth, imageHeight,
                                      xRepeat, yRepeat, imageRenderingMode);
        }
    }
    virtual void drawLinearGradient(const Unit::Rect& dst,
                                    GradientDrawingInfo* info,
                                    NativeGradient* gradient) = 0;
    virtual void drawRadialGradient(const Unit::Rect& dst,
                                    GradientDrawingInfo* info,
                                    NativeGradient* gradient) = 0;

    virtual void applyMatrixTo(LayoutLocation& lp) = 0;
    virtual void applyMatrixTo(LayoutRect& lp) = 0;

    virtual void setVisible(bool visible) = 0;
    virtual void setNonInvertableCTM(bool validation) = 0;
    virtual bool hasNonInvertableCTM() = 0;
    virtual void setPathTransformMatrix(const SkMatrix& marix) = 0;
    virtual SkMatrix pathTransformMatrix() = 0;
    virtual void setOriginalFontStr(String* fontStr) = 0;
    virtual void setCanvasWebFontState(size_t version) = 0;
    virtual size_t canvasWebFontState() = 0;
    virtual Font* font() = 0;
    virtual String* originalFontStr() = 0;
    virtual void setCanvasTextAlign(CanvasTextAlign textAlign) = 0;
    virtual CanvasTextAlign canvasTextAlign() = 0;
    virtual void setCanvasTextBaseline(CanvasTextBaseline textBaseline) = 0;
    virtual CanvasTextBaseline canvasTextBaseline() = 0;
    virtual void setCanvasTextDirection(CanvasDirection textDirection) = 0;
    virtual CanvasDirection canvasTextDirection() = 0;

    virtual bool canRejectPainting(const LayoutRect& rect)
    {
        STARFISH_UNIMPLEMENTED();
        return false;
    }

    // Generic canvas functions
    virtual void beginPath()
    {
        STARFISH_UNIMPLEMENTED();
    }
    virtual void closePath()
    {
        STARFISH_UNIMPLEMENTED();
    }
    virtual void moveTo(float x, float y)
    {
        STARFISH_UNIMPLEMENTED();
    }
    virtual void lineTo(float x, float y)
    {
        STARFISH_UNIMPLEMENTED();
    }
    virtual void curveTo(float x1, float y1, float x2, float y2, float x3,
                         float y3)
    {
        STARFISH_UNIMPLEMENTED();
    }
    virtual void quadraticCurveTo(float x1, float y1, float x2, float y2)
    {
        STARFISH_UNIMPLEMENTED();
    }
    virtual void arc(double xc, double yc, double radius, double angle1,
                     double angle2)
    {
        STARFISH_UNIMPLEMENTED();
    }
    virtual void arcNegative(double xc, double yc, double radius, double angle1,
                             double angle2)
    {
        STARFISH_UNIMPLEMENTED();
    }
    virtual void stroke()
    {
        STARFISH_UNIMPLEMENTED();
    }
    virtual void strokePreserve()
    {
        STARFISH_UNIMPLEMENTED();
    }
    virtual void strokePath(Path* path)
    {
        STARFISH_UNIMPLEMENTED();
    }
    virtual void setFillRule(bool shouldUseNonZeroFillRule = true)
    {
        STARFISH_UNIMPLEMENTED();
    }
    virtual void fill()
    {
        STARFISH_UNIMPLEMENTED();
    }
    virtual void fillPreserve()
    {
        STARFISH_UNIMPLEMENTED();
    }
    virtual void fillPath(Path* path)
    {
        STARFISH_UNIMPLEMENTED();
    }
    virtual void clipPath()
    {
        STARFISH_UNIMPLEMENTED();
    }
    virtual void clipPath(Path* path)
    {
        STARFISH_UNIMPLEMENTED();
    }
    virtual void clipPathPreserve()
    {
        STARFISH_UNIMPLEMENTED();
    }
    virtual float lineWidth()
    {
        STARFISH_UNIMPLEMENTED();
        return 1.0f;
    }
    virtual void setLineWidth(float width)
    {
        STARFISH_UNIMPLEMENTED();
    }
    virtual void setNeedsNoneAntialias()
    {
        STARFISH_UNIMPLEMENTED();
    }
    virtual void setNeedsFastAntialias()
    {
        STARFISH_UNIMPLEMENTED();
    }
    virtual void setNeedsGoodQualityAntialias()
    {
        STARFISH_UNIMPLEMENTED();
    }
    virtual void markDirtyRect(const Unit::Rect& rt)
    {
        STARFISH_UNIMPLEMENTED();
    }
    CanvasRenderTargetInfo& renderTargetInfo()
    {
        return m_renderTargetInfo;
    }
    virtual void maskNativeImage(NativeImageData* data, const Unit::Rect& dst,
                                 bool removeImmediately = true)
    {
        STARFISH_UNIMPLEMENTED();
    }
#if defined(STARFISH_ENABLE_TEST)
    virtual void dump(const char* path)
    {
        STARFISH_UNIMPLEMENTED();
    }
#endif
protected:
    void drawFillRectShadow(float x, float y, float w, float h);
    void drawStrokeRectShadow(float x, float y, float w, float h);
    void drawRectShadowInner(float x, float y, float w, float h, bool isFill);
    void drawFillTextShadow(float x, float y, float stringWidth,
                            const StringView& sv);
    void drawStrokeTextShadow(float x, float y, float stringWidth,
                              const StringView& sv);
    void drawTextShadowInner(float x, float y, float stringWidth,
                             const StringView& sv, bool isFill);

    void drawFillPathShadow(Path* path);
    void drawStrokePathShadow(Path* path);
    void drawPathShadowInner(Path* path, bool isFill);

    void drawImageShadow(NativeImageData* data, const Unit::Rect& dst);

    virtual void drawRectInner(float x, float y, float w, float h) = 0;
    virtual void drawStrokeRectInner(float x, float y, float w, float h) = 0;
    virtual void drawTextInner(float x, float y, float stringWidth,
                               const StringView& sv,
                               bool shouldSkipUnresolvedWebFont) = 0;
    virtual void drawStrokeTextInner(float x, float y, float stringWidth,
                                     const StringView& sv,
                                     bool shouldSkipUnresolvedWebFont) = 0;

    virtual void drawPathInner(Path* path) = 0;
    virtual void drawStrokePathInner(Path* path) = 0;
    virtual void drawImageInner(
        NativeImageData* data, const Unit::Rect& dst,
        ImageRenderingValue imageRenderingMode =
            ImageRenderingValue::ImageRenderingAutoValue) = 0;
    virtual void drawImageInner(
        NativeImageData* data, const Unit::Rect& src, const Unit::Rect& dst,
        const DrawImageInfo& borderinfo,
        ImageRenderingValue imageRenderingMode =
            ImageRenderingValue::ImageRenderingAutoValue) = 0;
    virtual void drawNativeImageData(
        NativeImageData* data, const Unit::Rect& dst,
        ImageRenderingValue imageRenderingMode) = 0;
    virtual void drawNativeImageData(
        NativeImageData* data, const Unit::Rect& src, const Unit::Rect& dst,
        const DrawImageInfo& borderinfo,
        ImageRenderingValue imageRenderingMode) = 0;

    virtual void drawRepeatNativeImageData(
        NativeImageData* data, const Unit::Rect& dst, float imageWidth,
        float imageHeight, bool xRepeat, bool yRepeat,
        ImageRenderingValue imageRenderingMode) = 0;
    CanvasState* lastState()
    {
        STARFISH_ASSERT(m_state.size() != 0);
        return m_state[m_state.size() - 1];
    }
    CanvasRenderTargetInfo m_renderTargetInfo;
    WebView* m_webView{ nullptr };
    Nullable<CanvasSurface*> m_targetSurface;
    GCVector<CanvasState*> m_state{};
    GCVector<CanvasState*> m_stateMemoryPool{};
    bool m_shouldApplyCanvasFillStrokeSource{ false };
};
} // namespace Starfish

#endif
