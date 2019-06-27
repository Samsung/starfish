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
#ifdef STARFISH_ENABLE_CANVAS

#include "core/dom/ExecutionContext.h"
#include "core/dom/Document.h"
#include "core/dom/canvas/CanvasImageSource.h"
#include "core/dom/canvas/CanvasRenderingContext.h"
#include "core/dom/canvas/CanvasRenderingContext2DMixIn.h"
#include "core/dom/WebOrigin.h"

namespace Starfish {
DOMExceptionOr<bool> CanvasImageSourceUtils::checkUsability(
    ExecutionContext* executionContext, CanvasImageSource image)
{
    // https://html.spec.whatwg.org/multipage/canvas.html#check-the-usability-of-the-image-argument
    if (image.isHTMLImageElementOrSVGImageElementValue()) {
        auto imgOrSvg = image.getHTMLImageElementOrSVGImageElementValue();

        NativeImageData* imageData = nullptr;
        if (imgOrSvg.isHTMLImageElementValue()) {
            if (imgOrSvg.getHTMLImageElementValue()->hasRequestError() ==
                true) {
                return false;
            }
            imageData = imgOrSvg.getHTMLImageElementValue()->imageData();
        } else if (imgOrSvg.isSVGImageElementValue()) {
            if (imgOrSvg.getSVGImageElementValue()->hasRequestError()) {
                return false;
            }
            imageData = imgOrSvg.getSVGImageElementValue()->imageData();
        } else {
            STARFISH_ASSERT(imgOrSvg.isNoneValue());
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
    } else if (image.isHTMLVideoElementValue()) {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
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

    if (image.isHTMLImageElementOrSVGImageElementValue()) {
        if (image.getHTMLImageElementOrSVGImageElementValue()
                .isHTMLImageElementValue()) {
            auto htmlImage = image.getHTMLImageElementOrSVGImageElementValue()
                                 .getHTMLImageElementValue();

            if (executionContext->document()->webOrigin()->isSameOrigin(
                    htmlImage->webOrigin()) == false) {
                clean = false;
            }

            nativeImageData = htmlImage->imageData();
        } else if (image.getHTMLImageElementOrSVGImageElementValue()
                       .isSVGImageElementValue()) {
            auto svgImage = image.getHTMLImageElementOrSVGImageElementValue()
                                .getSVGImageElementValue();

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
            context2d->flush();
            nativeImageData = NativeImageData::attach(context2d->canvas());
            clean = context->originCleanFlag();
        }
    } else if (image.isImageBitmapValue()) {
        auto imageBitmap = image.getImageBitmapValue();
        nativeImageData = imageBitmap->nativeImageData();
        clean = imageBitmap->originCleanFlag();
    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    return std::make_pair(nativeImageData, clean);
}
} // namespace Starfish
#endif
