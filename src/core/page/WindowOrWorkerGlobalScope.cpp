/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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
#include "WindowOrWorkerGlobalScope.h"
#include "core/dom/ExecutionContext.h"
#include "core/style/Style.h"
#include "core/modules/canvas/Canvas.h"
#include "core/dom/DOMException.h"
#include "core/dom/DOMExceptionOr.h"
#include "core/page/WebBase.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "core/dom/canvas/ImageSmoothingQuality.h"
#include "core/dom/canvas/CanvasImageSource.h"
#include "core/modules/message_loop/MessageLoop.h"

namespace Starfish {
namespace WindowOrWorkerGlobalScope {
#ifdef STARFISH_ENABLE_CANVAS

    struct ImageBitmapCreateContext {
        ImageBitmapCreateContext(ImageBitmapSource& image,
                                 ImageBitmapOptions& options, int32_t sx,
                                 int32_t sy, int32_t sw, int32_t sh)
            : m_image(image)
            , m_options(options)
            , m_sx(sx)
            , m_sy(sy)
            , m_sw(sw)
            , m_sh(sh)
            , m_hasSrcRect(true)
        {
        }
        ImageBitmapCreateContext(ImageBitmapSource& image,
                                 ImageBitmapOptions& options)
            : m_image(image)
            , m_options(options)
        {
        }

        ImageBitmapSource& m_image;
        ImageBitmapOptions& m_options;
        int32_t m_sx{ 0 }, m_sy{ 0 }, m_sw{ 0 }, m_sh{ 0 };
        bool m_hasSrcRect{ false };
    };

    static NULLABLE DOMException* isValidResizeValue(
        ExecutionContext* executionContext, ImageBitmapOptions& options)
    {
        DOMException* ret = nullptr;
        if ((options.hasResizeWidth() && options.resizeWidth() == 0) ||
            (options.hasResizeHeight() && options.resizeHeight() == 0)) {
            ret = new DOMException(executionContext,
                                   DOMException::Code::INVALID_STATE_ERR);
        }
        return ret;
    }

    static void resolvePromise(ExecutionContext* executionContext,
                               Promise* promise, ImageBitmap* imageBitmap)
    {
        executionContext->webBase()->messageLoop()->addIdler(
            executionContext->globalScope(),
            [](size_t handle, void* data, void* data1) {
                Promise* promise = (Promise*)data;
                promise->fulfill(((ImageBitmap*)data1)->scriptValue());
            },
            promise, imageBitmap);
    }

    static Promise* rejectPromiseWithDOMException(
        ExecutionContext* executionContext, Promise* promise,
        DOMException::Code code)
    {
        auto domException = new DOMException(executionContext, code);
        promise->reject(domException->scriptValue());
        return promise;
    }

    static void resolveOutputSize(int32_t sw, int32_t sh,
                                  ImageBitmapOptions& options,
                                  size_t& outputWidth, size_t& outputHight)
    {
        if (options.hasResizeWidth()) {
            outputWidth = options.resizeWidth();
        } else if (!options.hasResizeWidth() && options.hasResizeHeight()) {
            outputWidth = round(((double)(sw * options.resizeHeight())) / sh);
        } else {
            outputWidth = sw;
        }

        if (options.hasResizeHeight()) {
            outputHight = options.resizeHeight();
        } else if (!options.hasResizeHeight() && options.hasResizeWidth()) {
            outputHight = round(((double)(sh * options.resizeWidth())) / sw);
        } else {
            outputHight = sh;
        }
    }

    static NativeImageData* cropBitmapDataToSourceRectangleWithFormatting(
        ExecutionContext* executionContext, NativeImageData* srcImage,
        int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        ImageBitmapOptions& options)
    {
        // https://html.spec.whatwg.org/multipage/imagebitmap-and-animations.html#cropped-to-the-source-rectangle-with-formatting
        if (srcImage == nullptr ||
            (!srcImage->width() || !srcImage->height())) {
            return nullptr;
        }

        size_t outputWidth, outputHight;
        resolveOutputSize(sw, sh, options, outputWidth, outputHight);

        NativeImageData* destImage =
            NativeImageData::create(outputWidth, outputHight);
        Canvas* canvas =
            Canvas::create(executionContext->webBase()->asWebView(), destImage);

        canvas->clearColor(Unit::Color(0, 0, 0, 0));
        canvas->setImageSmoothingEnabled(true);
        canvas->setImageSmoothingQuality(options.toImageRenderingValue());

        Unit::Rect destRect(0, 0, outputWidth, outputHight);
        ImageRenderingValue imageRenderingValue = toImageRenderingValue(
            canvas->imageSmoothingEnabled(), canvas->imageSmoothingQuality());

        if (options.getImageOrientation() == ImageOrientation::FlipY) {
            canvas->translate(0, outputHight);
            canvas->scale(1, -1);
        }
        // TODO : Apply colorSpaceConversion, premultiplyAlpha

        Unit::Rect srcRect = Unit::Rect(sx, sy, sw, sh);
        Unit::Rect adjustSrcRect(0, 0, srcImage->width(), srcImage->height());

        if (adjustSrcRect.contains(srcRect) == true) {
            adjustSrcRect = srcRect;
        } else {
            adjustSrcRect.intersect(srcRect);
        }

        DrawImageInfo drawImageInfo = { 1.0, 1.0,
                                        BorderImageRepeatValue::StretchValue,
                                        BorderImageRepeatValue::StretchValue };
        canvas->drawImage(srcImage, adjustSrcRect, destRect, drawImageInfo,
                          imageRenderingValue);
        canvas->flush();
        delete canvas;

        return destImage;
    }

    Promise* createImageBitmapInternal(ExecutionContext* executionContext,
                                       ImageBitmapCreateContext context)
    {
        // https://html.spec.whatwg.org/multipage/imagebitmap-and-animations.html#dom-createimagebitmap
        Promise* promise =
            new Promise(executionContext->scriptBindingInstance());

        if (!executionContext->webBase()->isWebView()) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            return rejectPromiseWithDOMException(
                executionContext, promise,
                DOMException::Code::INVALID_STATE_ERR);
        }

        if (context.m_hasSrcRect) {
            if (!context.m_sw || !context.m_sh) {
                auto error =
                    scriptTypeError(executionContext->scriptBindingInstance(),
                                    String::fromUTF8("sw or sh is 0."));
                promise->reject(createScriptValue(error));
                return promise;
            }
        }

        auto domException =
            isValidResizeValue(executionContext, context.m_options);
        if (domException != nullptr) {
            promise->reject(domException->scriptValue());
            return promise;
        }

        if (context.m_image
                .isHTMLOrSVGImageElementOrHTMLVideoElementOrHTMLCanvasElementOrImageBitmapValue()) {
            auto canvasImageSource =
                context.m_image
                    .getHTMLOrSVGImageElementOrHTMLVideoElementOrHTMLCanvasElementOrImageBitmapValue();
            // Check the usability of the image argument. If this throws an
            // exception or returns bad, then return p rejected with an
            // "InvalidStateError" DOMException.
            auto usability = CanvasImageSourceUtils::checkUsability(
                executionContext, canvasImageSource);

            if (usability.isDOMException() || !usability.asOtherType()) {
                return rejectPromiseWithDOMException(
                    executionContext, promise,
                    DOMException::Code::INVALID_STATE_ERR);
            }

            auto pair = CanvasImageSourceUtils::toNativeImageData(
                executionContext, canvasImageSource);
            NativeImageData* srcImage = pair.first;
            NativeImageData* destImage = nullptr;
            if (context.m_hasSrcRect) {
                destImage = cropBitmapDataToSourceRectangleWithFormatting(
                    executionContext, srcImage, context.m_sx, context.m_sy,
                    context.m_sw, context.m_sh, context.m_options);
            } else {
                destImage = cropBitmapDataToSourceRectangleWithFormatting(
                    executionContext, srcImage, 0, 0, srcImage->width(),
                    srcImage->height(), context.m_options);
            }

            if (destImage == nullptr) {
                return rejectPromiseWithDOMException(
                    executionContext, promise,
                    DOMException::Code::INVALID_STATE_ERR);
            }

            ImageBitmap* imageBitmap =
                new ImageBitmap(executionContext, destImage);
            imageBitmap->setOriginCleanFlag(pair.second);

            resolvePromise(executionContext, promise, imageBitmap);
        } else {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            return rejectPromiseWithDOMException(
                executionContext, promise,
                DOMException::Code::INVALID_STATE_ERR);
        }
        return promise;
    }

    Promise* createImageBitmap(ExecutionContext* executionContext,
                               ImageBitmapSource& image,
                               ImageBitmapOptions& options)
    {
        ImageBitmapCreateContext context(image, options);
        return createImageBitmapInternal(executionContext, context);
    }

    Promise* createImageBitmap(ExecutionContext* executionContext,
                               ImageBitmapSource& image, int32_t sx, int32_t sy,
                               int32_t sw, int32_t sh,
                               ImageBitmapOptions& options)
    {
        ImageBitmapCreateContext context(image, options, sx, sy, sw, sh);
        return createImageBitmapInternal(executionContext, context);
    }

#endif
} // namespace WindowOrWorkerGlobalScope
} // namespace Starfish
