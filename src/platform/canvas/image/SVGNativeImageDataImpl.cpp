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
#include "core/layout/svg/FrameSVGSVGBox.h"
#include "core/page/BrowsingContext.h"

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

    virtual void updateContentSize(FrameBox* containingBlock)
    {
        LayoutContext dummyContext(
            containingBlock->node()->starfish(),
            containingBlock->node()->document()->frame()->asFrameDocument());
        m_frameSVGSVGBox->computeContentWidthAndHeight(dummyContext,
                                                       containingBlock);
        m_width = ceil(m_frameSVGSVGBox->width().toFloat());
        m_height = ceil(m_frameSVGSVGBox->height().toFloat());
    }

    virtual void paintContent(Canvas* canvas, const Unit::Rect& dst,
                              ImageRenderingValue imageRenderingMode) override
    {
        canvas->save();
        canvas->clip(dst);
        canvas->translate(dst.x(), dst.y());

        m_frameSVGSVGBox->setContainerViewport(dst);
        m_frameSVGSVGBox->paintReplaced(canvas);
        m_frameSVGSVGBox->setContainerViewport(Nullable<Unit::Rect>());

        canvas->restore();
    }

    virtual void paintContent(Canvas* canvas, const Unit::Rect& src,
                              const Unit::Rect& dst,
                              const DrawImageInfo& borderinfo,
                              ImageRenderingValue imageRenderingMode) override
    {
        auto img = rasterizedImage();
        canvas->drawImage(img, src, dst, borderinfo, imageRenderingMode);
        delete img;
    }

    virtual void paintRepeatContent(
        Canvas* canvas, const Unit::Rect& dst, float imageWidth,
        float imageHeight, bool xRepeat, bool yRepeat,
        ImageRenderingValue imageRenderingMode) override
    {
        canvas->save();
        canvas->clip(dst);
        canvas->translate(dst.x(), dst.y());

        Unit::Rect baseRect = Unit::Rect(0, 0, dst.width(), dst.height());
        Unit::Rect current = Unit::Rect(0, 0, imageWidth, imageHeight);

        while (baseRect.intersects(current)) {
            paintContent(canvas, current, imageRenderingMode);
            while (baseRect.intersects(current) && xRepeat) {
                current.setX(current.x() + imageWidth);
                paintContent(canvas, current, imageRenderingMode);
            }
            if (yRepeat) {
                current.setY(current.y() + imageHeight);
            }
            current.setX(0);
        }
        canvas->restore();
    }

    NativeImageData* rasterizedImage()
    {
        NativeImageData* rasterizedSVGImage =
            BufferedNativeImageData::create(width(), height());
        rasterizedSVGImage->clear();
        Canvas* dummyCanvas = Canvas::create(
            m_frameSVGSVGBox->document()->browsingContext()->webView(),
            rasterizedSVGImage->data(), rasterizedSVGImage->width(),
            rasterizedSVGImage->height(), rasterizedSVGImage->stride());
        m_frameSVGSVGBox->paintReplaced(dummyCanvas);
        delete dummyCanvas;
        return rasterizedSVGImage;
    }

protected:
    StorePositiveIntergerAsOdd m_width;
    StorePositiveIntergerAsOdd m_height;
};

NativeImageData* SVGNativeImageData::create(size_t width, size_t height,
                                            FrameSVGSVGBox* frameSVGSVGBox)
{
    return new SVGNativeImageDataImpl(width, height, frameSVGSVGBox);
}

} // namespace Starfish
