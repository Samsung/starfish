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
#include "core/dom/WebOrigin.h"
#include "core/dom/canvas/HTMLCanvasElement.h"
#include "core/dom/HTMLImageElement.h"
#include "core/dom/ImageBitmap.h"
#include "core/dom/svg/SVGImageElement.h"
#include "core/dom/DOMException.h"

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
        STARFISH_UNSUPPORTED("video type is not supported (%s)",
                             __PRETTY_FUNCTION__);
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

std::pair<NULLABLE NativeImageData*, bool>
CanvasImageSourceUtils::toNativeImageData(ExecutionContext* executionContext,
                                          CanvasImageSource& image)
{
    NativeImageData* nativeImageData = nullptr;
    bool clean = true;

    if (image.isHTMLImageElementValue() || image.isSVGImageElementValue()) {
        if (image.isHTMLImageElementValue()) {
            auto htmlImage = image.getHTMLImageElementValue();

            if (executionContext->document()->webOrigin()->isSameOrigin(
                    htmlImage->webOrigin()) == false) {
                clean = false;
            }

            nativeImageData = htmlImage->imageData();
        } else if (image.isSVGImageElementValue()) {
            auto svgImage = image.getSVGImageElementValue();

            if (executionContext->document()->webOrigin()->isSameOrigin(
                    svgImage->webOrigin()) == false) {
                clean = false;
            }
            nativeImageData = svgImage->imageData();
        } else {
            STARFISH_ASSERT(image.isNoneValue());
        }
    } else if (image.isHTMLCanvasElementValue()) {
        auto htmlCanvas = image.getHTMLCanvasElementValue();
        auto context = htmlCanvas->canvasRenderingContext();
        if (context != nullptr) {
            auto context2d = (CanvasRenderingContext2DMixIn*)context;
            context2d->flushForReadback();
            nativeImageData = NativeImageData::attach(context2d->canvas());
            clean = context->originCleanFlag();
        }
    } else if (image.isImageBitmapValue()) {
        auto imageBitmap = image.getImageBitmapValue();
        nativeImageData = imageBitmap->nativeImageData();
        clean = imageBitmap->originCleanFlag();
    } else {
        STARFISH_UNSUPPORTED("Unsupported image type(%s)", __PRETTY_FUNCTION__);
    }

    return std::make_pair(nativeImageData, clean);
}
} // namespace Starfish
#endif
