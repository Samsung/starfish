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

#include "StarfishConfig.h"
#include "core/modules/canvas/image/SVGNativeImageData.h"
#include "core/modules/canvas/image/BufferedNativeImageData.h"
#include "core/modules/canvas/Canvas.h"
#include "core/dom/Document.h"
#include "core/dom/svg/SVGSVGElement.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/svg/FrameSVGSVGBox.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/style/ComputedStyle.h"
#include "core/style/FilterFunctions.h"

namespace Starfish {

class SVGNativeImageDataImpl : public SVGNativeImageData {
public:
    void* operator new(size_t size)
    {
        return GC_MALLOC(sizeof(SVGNativeImageDataImpl));
    }

    SVGNativeImageDataImpl(size_t w, size_t h, FrameSVGSVGBox* frameSVGSVGBox)
    {
        m_width = w;
        m_height = h;
        m_frameSVGSVGBox = frameSVGSVGBox;
    }

    virtual ~SVGNativeImageDataImpl()
    {
    }

    virtual size_t width() override
    {
        return m_width;
    }

    virtual size_t height() override
    {
        return m_height;
    }

    virtual void clear() override
    {
    }

    virtual size_t bufferSize() override
    {
        // TODO
        return 0;
    }

    virtual bool hasViewport()
    {
        return m_frameSVGSVGBox->intrinsicSize().m_hasViewport;
    }

    virtual bool hasViewBox()
    {
        return m_frameSVGSVGBox->node()->asSVGSVGElement()->hasViewBox();
    }

    virtual void updateContentSize(FrameBox* containingBlock)
    {
        FrameDocument* dummyDocument =
            m_frameSVGSVGBox->document()->frame()->asFrameDocument();
        LayoutContext dummyContext(dummyDocument->node()->starfish(),
                                   dummyDocument);
        if (!hasViewport() && !hasViewBox()) {
            if (dummyDocument->width() != containingBlock->width() ||
                dummyDocument->height() != containingBlock->height()) {
                size_t newWidth = containingBlock->width().toUnsigned();
                size_t newHeight = containingBlock->height().toUnsigned();
                m_frameSVGSVGBox->setDefaultWidth(newWidth);
                m_frameSVGSVGBox->setDefaultHeight(newHeight);
                dummyDocument->node()->window()->resize(newWidth, newHeight);
                dummyDocument->layout(dummyContext,
                                      Frame::LayoutWantToResolve::ResolveAll);
                m_width = ceil(m_frameSVGSVGBox->width().toFloat());
                m_height = ceil(m_frameSVGSVGBox->height().toFloat());
            }
        } else {
            m_frameSVGSVGBox->computeContentWidthAndHeight(dummyContext,
                                                           containingBlock);
            m_width = ceil(m_frameSVGSVGBox->width().toFloat());
            m_height = ceil(m_frameSVGSVGBox->height().toFloat());
        }
    }

    virtual void paintContent(Canvas* canvas, const Unit::Rect& dst,
                              ImageRenderingValue imageRenderingMode) override
    {
        if (!hasRootFilter()) {
            paintRoot(canvas, dst);
            return;
        }
        // Filtered on the CPU at the canvas' resolution, within the image.
        float scale = canvas->currentTransformMatrix().getMaxScale();
        scale = scale > 0 ? scale : 1;
        float w = dst.width() * scale;
        float h = dst.height() * scale;
        if (!(w > 0 && h > 0)) {
            return;
        }
        // shortcut: the image rasterizes whole, so past a screen and a half
        // of pixels (the compositor's filter budget) or a buffer's longest
        // side it rasterizes at a lower resolution and comes out softer;
        // rasterize only its visible part if that ever matters.
        const ScreenInfo& screen = m_frameSVGSVGBox->document()
                                       ->browsingContext()
                                       ->webView()
                                       ->screenInfo();
        float dpr = screen.devicePixelRatio;
        float budget = screen.rect.width().toFloat() * dpr *
                       screen.rect.height().toFloat() * dpr * 3 / 2;
        const float maxSide = BufferedNativeImageData::kMaxSide;
        float k = std::min(
            { 1.f, std::sqrt(budget / (w * h)), maxSide / std::max(w, h) });
        size_t pw = std::min(std::ceil(w * k), maxSide);
        size_t ph = std::min(std::ceil(h * k), maxSide);
        if (!pw || !ph) {
            return;
        }
        NativeImageData* img = rasterize(pw, ph);
        canvas->drawImage(img, dst, imageRenderingMode);
        delete img;
    }

    virtual void paintContent(Canvas* canvas, const Unit::Rect& src,
                              const Unit::Rect& dst,
                              const DrawImageInfo& borderinfo,
                              ImageRenderingValue imageRenderingMode) override
    {
        NativeImageData* img = rasterize(width(), height());
        canvas->drawImage(img, src, dst, borderinfo, imageRenderingMode);
        delete img;
    }

    virtual void paintRepeatContent(
        Canvas* canvas, const Unit::Rect& dst, float imageWidth,
        float imageHeight, bool xRepeat, bool yRepeat,
        ImageRenderingValue imageRenderingMode) override
    {
        NativeImageData* img = rasterize(width(), height());
        canvas->drawRepeatImage(img, dst, imageWidth, imageHeight, xRepeat,
                                yRepeat, imageRenderingMode);
        delete img;
    }

    virtual NativeImageData* rasterize(size_t w, size_t h) override
    {
        NativeImageData* rasterizedSVGImage =
            BufferedNativeImageData::create(w, h);
        rasterizedSVGImage->clear();
        // |w| and |h| are the buffer's pixels already: no device scale on top.
        Canvas* dummyCanvas = Canvas::create(
            rasterizedSVGImage->data(), rasterizedSVGImage->width(),
            rasterizedSVGImage->height(), rasterizedSVGImage->stride(), 1);
        if (w == width() && h == height()) {
            m_frameSVGSVGBox->paintReplaced(dummyCanvas);
        } else {
            // Another size scales the image the way <img> paints it.
            paintRoot(dummyCanvas, Unit::Rect(0, 0, w, h));
        }
        delete dummyCanvas;
        if (hasRootFilter()) {
            // Filter values are in the image document's CSS px.
            float scale = width() ? w / static_cast<float>(width()) : 1;
            WebView* webView =
                m_frameSVGSVGBox->document()->browsingContext()->webView();
            for (auto filter : *m_frameSVGSVGBox->style()->filter()) {
                filter->apply(webView, rasterizedSVGImage->data(),
                              rasterizedSVGImage->width(),
                              rasterizedSVGImage->height(),
                              rasterizedSVGImage->stride(), scale);
            }
        }
        return rasterizedSVGImage;
    }

protected:
    // The root element of an image's document filters what the image shows
    // too (Filter Effects 1), cut to the image.
    bool hasRootFilter()
    {
        return m_frameSVGSVGBox->style()->hasAvailableFilter();
    }

    void paintRoot(Canvas* canvas, const Unit::Rect& dst)
    {
        canvas->save();
        canvas->clip(dst);
        canvas->translate(dst.x(), dst.y());

        m_frameSVGSVGBox->setContainerViewport(dst);
        m_frameSVGSVGBox->paintReplaced(canvas);
        m_frameSVGSVGBox->setContainerViewport(Optional<Unit::Rect>());

        canvas->restore();
    }

    StorePositiveIntergerAsOdd m_width;
    StorePositiveIntergerAsOdd m_height;
};

NativeImageData* SVGNativeImageData::create(size_t width, size_t height,
                                            FrameSVGSVGBox* frameSVGSVGBox)
{
    return new SVGNativeImageDataImpl(width, height, frameSVGSVGBox);
}

} // namespace Starfish
