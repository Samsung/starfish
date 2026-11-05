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
#ifdef STARFISH_ENABLE_CANVAS

#include "core/dom/ExecutionContext.h"
#include "core/dom/Document.h"
#include "core/dom/canvas/CanvasImageSource.h"
#include "core/dom/canvas/CanvasRenderingContext.h"
#include "core/dom/canvas/CanvasRenderingContext2DMixIn.h"
#include "core/dom/canvas/HTMLCanvasElement.h"
#include "core/dom/HTMLImageElement.h"
#include "core/dom/ImageBitmap.h"
#include "core/dom/svg/SVGImageElement.h"
#include "core/dom/DOMException.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/image/BufferedNativeImageData.h"
#if defined(STARFISH_ENABLE_MULTIMEDIA)
#include "core/dom/HTMLVideoElement.h"
#include "platform/multimedia/MediaPlayer.h"
#endif
#if defined(STARFISH_ENABLE_WEBGL)
#include "core/dom/canvas/webgl/WebGLRenderingContext.h"
#endif

namespace Starfish {
DOMExceptionOr<bool> CanvasImageSourceUtils::checkUsability(
    ExecutionContext* executionContext, CanvasImageSource image)
{
    // https://html.spec.whatwg.org/multipage/canvas.html#check-the-usability-of-the-image-argument
    if (image.isHTMLImageElementValue() || image.isSVGImageElementValue()) {
        NativeImageData* imageData = nullptr;
        if (image.isHTMLImageElementValue()) {
            if (image.getHTMLImageElementValue()->hasRequestError()) {
                return false;
            }
            imageData = image.getHTMLImageElementValue()->imageData();
        } else if (image.isSVGImageElementValue()) {
            if (image.getSVGImageElementValue()->hasRequestError()) {
                return false;
            }
            imageData = image.getSVGImageElementValue()->imageData();
        } else {
            STARFISH_ASSERT(image.isNoneValue());
            return false;
        }

        if (imageData == executionContext->document()->brokenImage()) {
            return new DOMException(executionContext,
                                    DOMException::Code::INVALID_STATE_ERR);
        }

        if (imageData == nullptr || imageData->width() == 0 ||
            imageData->height() == 0) {
            return false;
        }

        return true;
    } else if (image.isHTMLCanvasElementValue()) {
        auto canvas = image.getHTMLCanvasElementValue();
        if (canvas->width() == 0 || canvas->height() == 0) {
            return new DOMException(executionContext,
                                    DOMException::Code::INVALID_STATE_ERR,
                                    "The image argument is a canvas element "
                                    "with a width or height of 0.");
        }
        return true;
    } else if (image.isImageBitmapValue()) {
        auto imageBitmap = image.getImageBitmapValue();
        if (imageBitmap->isDetached()) {
            return new DOMException(
                executionContext, DOMException::Code::INVALID_STATE_ERR,
                "The image argument is a detached ImageBitmap");
        }
        return true;
#if defined(STARFISH_ENABLE_MULTIMEDIA)
    } else if (image.isHTMLVideoElementValue()) {
        // A video without data for its current frame is bad.
        return image.getHTMLVideoElementValue()->readyState() >=
               HTMLMediaElement::HAVE_CURRENT_DATA;
#endif
    } else {
        STARFISH_ASSERT(image.isNoneValue());
        return new DOMException(
            executionContext, DOMException::Code::SCRIPT_TYPE_ERR,
            "The image is not of type '(CSSImageValue or HTMLImageElement or "
            "SVGImageElement or HTMLVideoElement or HTMLCanvasElement or "
            "ImageBitmap or OffscreenCanvas)");
    }
    return false;
}

CanvasImageSourceImage CanvasImageSourceUtils::toNativeImageData(
    ExecutionContext* executionContext, CanvasImageSource& image)
{
    CanvasImageSourceImage result;

    if (image.isHTMLImageElementValue() || image.isSVGImageElementValue()) {
        if (image.isHTMLImageElementValue()) {
            auto htmlImage = image.getHTMLImageElementValue();
            result.originClean = htmlImage->isImageCORSSameOrigin();
            result.image = htmlImage->imageData();
        } else if (image.isSVGImageElementValue()) {
            auto svgImage = image.getSVGImageElementValue();
            result.originClean = svgImage->isImageCORSSameOrigin();
            result.image = svgImage->imageData();
        } else {
            STARFISH_ASSERT(image.isNoneValue());
        }
    } else if (image.isHTMLCanvasElementValue()) {
        auto htmlCanvas = image.getHTMLCanvasElementValue();
        auto context = htmlCanvas->canvasRenderingContext();
        // A bitmaprenderer canvas yields no image here.
        if (context != nullptr && context->isCanvasRenderingContext2D()) {
            auto context2d =
                static_cast<CanvasRenderingContext2DMixIn*>(context);
            context2d->flushForReadback();
            result.image = NativeImageData::attach(context2d->canvas());
            result.originClean = context->originCleanFlag();
#if defined(STARFISH_ENABLE_WEBGL)
        } else if (context != nullptr &&
                   (context->isWebGLRenderingContext() ||
                    context->isWebGL2RenderingContext())) {
            // A WebGL canvas draws its drawing buffer (WebGL 1.0 §2.2).
            Optional<NativeImageData*> copy =
                static_cast<WebGLRenderingContext*>(context)
                    ->copyDrawingBuffer();
            if (copy) {
                result.image = copy.value();
                result.isCopy = true;
            }
            result.originClean = context->originCleanFlag();
#endif
        }
    } else if (image.isImageBitmapValue()) {
        auto imageBitmap = image.getImageBitmapValue();
        result.image = imageBitmap->premultipliedImageData();
        result.originClean = imageBitmap->originCleanFlag();
#if defined(STARFISH_ENABLE_MULTIMEDIA)
    } else if (image.isHTMLVideoElementValue()) {
        // A video's image is its current frame (HTML "drawImage",
        // createImageBitmap()).
        HTMLVideoElement* video = image.getHTMLVideoElementValue();
        result.originClean = video->mediaDataIsCORSSameOrigin();
        MediaPlayer* player = video->activeMediaPlayer();
        std::vector<uint8_t> pixels;
        size_t width = 0;
        size_t height = 0;
        if (player && player->copyCurrentVideoFrame(pixels, width, height)) {
            BufferedNativeImageData* frame =
                BufferedNativeImageData::create(width, height);
            for (size_t row = 0; row < height; row++) {
                uint8_t* dest = frame->data() + row * frame->stride();
                memcpy(dest, pixels.data() + row * width * 4, width * 4);
#if defined(PORT_PIXEL_ORDER_BGRA)
                for (size_t col = 0; col < width; col++) {
                    std::swap(dest[col * 4], dest[col * 4 + 2]);
                }
#endif
            }
            // Canvas images hold premultiplied colors.
            convertImageBufferAsPremultipliedAlphaIfNeeds(
                frame->data(), width, frame->stride(), height);
            result.image = frame;
            result.isCopy = true;
        }
#endif
    } else {
        STARFISH_UNSUPPORTED("Unsupported image type(%s)", __PRETTY_FUNCTION__);
    }

    return result;
}
} // namespace Starfish
#endif
