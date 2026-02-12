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
#include "WindowOrWorkerGlobalScope.h"
#include "core/dom/ExecutionContext.h"
#include "core/page/WebBase.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "EscargotPublic.h"
#include "core/dom/StructuredSerializeOptions.h"
#include "core/serialize/Serializer.h"

#ifdef STARFISH_ENABLE_CANVAS
#include "core/style/Style.h"
#include "core/modules/canvas/Canvas.h"
#include "core/dom/DOMException.h"
#include "core/dom/DOMExceptionOr.h"
#include "core/page/WebView.h"
#include "core/modules/canvas/image/BufferedNativeImageData.h"
#include "core/modules/canvas/image/CompressedNativeImageData.h"
#include "core/dom/canvas/ImageSmoothingQuality.h"
#include "core/dom/canvas/CanvasImageSource.h"
#include "core/modules/canvas/image/ImageDecoder.h"
#include "core/fetch/ResponseData.h"
#include "core/dom/ImageBitmap.h"
#include "core/dom/canvas/ImageData.h"
#include "core/fileapi/Blob.h"
#endif

namespace Starfish {
namespace WindowOrWorkerGlobalScope {

    String* btoa(ExecutionContext* executionContext, String* data)
    {
        // TODO : Throws an "InvalidCharacterError" DOMException exception if
        // the input string contains any out-of-range characters.
        UTF8StringDataNonGCStd orginalStr = data->toUTF8NonGCString();
        std::string result = Base64Utils::encodeBase64(orginalStr);
        return String::createASCIIString(result.c_str(), result.length());
    }

    String* atob(ExecutionContext* executionContext, String* data)
    {
        // TODO : Throws an "InvalidCharacterError" DOMException if the input
        // string is not valid base64 data.
        UTF8StringDataNonGCStd orginalStr = data->toUTF8NonGCString();
        std::string result = Base64Utils::decodeBase64(orginalStr);
        return String::createASCIIString(result.c_str(), result.length());
    }

    void queueMicrotask(ExecutionContext* executionContext,
                        ScriptObject callback)
    {
        struct Param : public gc {
            ExecutionContext* executionContext;
            ScriptObject callback;
        };

        Param* param = new Param();
        param->executionContext = executionContext;
        param->callback = callback;

        enqueueMicrotask(
            executionContext->scriptBindingInstance(),
            [](void* data) {
                Param* param = (Param*)data;
                callScriptFunction(
                    param->executionContext->scriptBindingInstance(),
                    Escargot::ValueRef::create(param->callback), nullptr, 0,
                    scriptUndefined());
            },
            param);
    }

    ScriptValue structuredClone(ExecutionContext* executionContext,
                                ScriptValue value)
    {
        return structuredClone(executionContext, value,
                               StructuredSerializeOptions());
    }

    ScriptValue structuredClone(ExecutionContext* executionContext,
                                ScriptValue value,
                                StructuredSerializeOptions options)
    {
        SerializeWithTransferResult serializedRecord;
        DeserializeWithTransferResult deserializedRecord;
        // Needs to handle exception
        Serializer::serializeWithTransfer(executionContext, value,
                                          options.transfer(), serializedRecord);
        STARFISH_ASSERT(serializedRecord.m_deserializer);
        serializedRecord.m_deserializer(executionContext, serializedRecord,
                                        deserializedRecord);
        return deserializedRecord.m_deserialized;
    }

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
            if (sw < 0) {
                m_sx = sx + sw;
                m_sw = std::abs(sw);
            }
            if (sh < 0) {
                m_sy = sy + sh;
                m_sh = std::abs(sh);
            }
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
            BufferedNativeImageData::create(outputWidth, outputHight);
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

    static NativeImageData* createNativeImageDataWithDecoding(
        const char* buffer, size_t buffer_size,
        uint32_t needsDownScaleImageResourceLargerThan, float devicePixelRatio)
    {
        NativeImageData* result = nullptr;
        ImageDecoder::DecodeResult decodeResult;
        ResponseBody internalBuffer;
        internalBuffer.insert(internalBuffer.begin(), buffer,
                              buffer + buffer_size);
        ImageDecoder id(internalBuffer, needsDownScaleImageResourceLargerThan,
                        devicePixelRatio);
        decodeResult = id.decode();

        if (!decodeResult.m_isSuccessful) {
            return nullptr;
        }

        result = CompressedNativeImageData::create(
            internalBuffer, UTF8StringDataNonGCStd(),
            needsDownScaleImageResourceLargerThan, devicePixelRatio,
            decodeResult.m_buffer, decodeResult.m_width, decodeResult.m_height,
            decodeResult.m_stride);
        return result;
    }

    static NativeImageData* createNativeImageDataWithoutDecoding(
        const char* buffer, size_t buffer_size, size_t width, size_t height,
        size_t stride, uint32_t needsDownScaleImageResourceLargerThan,
        float devicePixelRatio)
    {
        NativeImageData* result = nullptr;
        ResponseBody internalBuffer;
        internalBuffer.insert(internalBuffer.begin(), buffer,
                              buffer + buffer_size);
        result = CompressedNativeImageData::create(
            internalBuffer, UTF8StringDataNonGCStd(),
            needsDownScaleImageResourceLargerThan, devicePixelRatio,
            const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(buffer)),
            width, height, stride);
        return result;
    }

    Promise* createImageBitmapInternal(ExecutionContext* executionContext,
                                       ImageBitmapCreateContext context)
    {
        // https://html.spec.whatwg.org/multipage/imagebitmap-and-animations.html#dom-createimagebitmap
        Promise* promise =
            new Promise(executionContext->scriptBindingInstance());

        if (!executionContext->webBase()->isWebView()) {
            STARFISH_UNIMPLEMENTED();
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

        NativeImageData* srcImage = nullptr;
        NativeImageData* destImage = nullptr;
        bool needsToSetOriginCleanFlag = false;
        bool setOriginCleanFlagValue = false;
        uint32_t needsDownScaleImageResourceLargerThan =
            executionContext->webBase()
                ->asWebView()
                ->needsDownScaleImageResourceLargerThan();
        float devicePixelRatio = executionContext->webBase()
                                     ->asWebView()
                                     ->screenInfo()
                                     .devicePixelRatio;
        if (context.m_image
                .isHTMLImageElementOrSVGImageElementOrHTMLVideoElementOrHTMLCanvasElementOrImageBitmapValue()) {
            auto canvasImageSource =
                context.m_image
                    .getHTMLImageElementOrSVGImageElementOrHTMLVideoElementOrHTMLCanvasElementOrImageBitmapValue();
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
            srcImage = pair.first;
            needsToSetOriginCleanFlag = true;
            setOriginCleanFlagValue = pair.second;

        } else if (context.m_image.isBlobValue()) {
            auto blob = context.m_image.getBlobValue();
            // 1. Let imageData be the result of reading image's data. If an
            // error occurs during reading of the object, then reject p with an
            // "InvalidStateError" DOMException and abort these steps.
            if (blob == nullptr || blob->data() == nullptr ||
                blob->size() == 0) {
                return rejectPromiseWithDOMException(
                    executionContext, promise,
                    DOMException::Code::INVALID_STATE_ERR);
            }
            NativeImageData* srcImage = createNativeImageDataWithDecoding(
                (const char*)blob->data(), (size_t)blob->size(),
                needsDownScaleImageResourceLargerThan, devicePixelRatio);
            NativeImageData* destImage = nullptr;

            // 3.If imageData is not in a supported image file format (e.g.,
            // it's not an image at all), or if imageData is corrupted in some
            // fatal way such that the image dimensions cannot be obtained
            // (e.g., a vector graphic with no intrinsic size), then reject p
            // with an "InvalidStateError" DOMException and abort these steps.
        } else if (context.m_image.isImageDataValue()) {
            auto imageData = context.m_image.getImageDataValue();
            size_t imagaDataWidth = imageData->width();
            size_t imagaDataHeight = imageData->height();
            size_t stride = imagaDataWidth * 4;
            // 2.If IsDetachedBuffer(buffer) is true, then return p rejected
            // with an "InvalidStateError" DOMException.
            if (imageData->data()
                    ->asArrayBufferView()
                    ->buffer()
                    ->isArrayBufferObject() &&
                imageData->data()
                    ->asArrayBufferView()
                    ->buffer()
                    ->asArrayBufferObject()
                    ->isDetachedBuffer()) {
                return rejectPromiseWithDOMException(
                    executionContext, promise,
                    DOMException::Code::INVALID_STATE_ERR);
            }
            uint8_t* srcPtr =
                imageData->data()->asArrayBufferView()->buffer()->rawBuffer();
            uint8_t* dstPtr =
                (uint8_t*)malloc(imagaDataWidth * imagaDataHeight * stride);
            size_t bufferLength = (size_t)imageData->data()
                                      ->asArrayBufferView()
                                      ->buffer()
                                      ->byteLength();
#if defined(PORT_PIXEL_ORDER_RGBA)
            memcpy(dstPtr, srcPtr, bufferLength);
#else
            {
                for (size_t y = 0; y < imagaDataHeight; y++) {
                    for (size_t x = 0; x < imagaDataWidth; x++) {
                        uint8_t* srcPixel =
                            srcPtr + (y * imagaDataWidth * 4) + (x * 4);
                        uint8_t* dstPixel =
                            dstPtr + (y * imagaDataWidth * 4) + (x * 4);
                        uint8_t r, g, b, a;
                        r = srcPixel[0];
                        g = srcPixel[1];
                        b = srcPixel[2];
                        a = srcPixel[3];
                        dstPixel[2] = r;
                        dstPixel[1] = g;
                        dstPixel[0] = b;
                        dstPixel[3] = a;
                    }
                }
            }
#endif
            srcImage = createNativeImageDataWithoutDecoding(
                (char*)dstPtr, bufferLength, imagaDataWidth, imagaDataHeight,
                stride, needsDownScaleImageResourceLargerThan,
                devicePixelRatio);

        } else {
            STARFISH_UNIMPLEMENTED();
            return rejectPromiseWithDOMException(
                executionContext, promise,
                DOMException::Code::INVALID_STATE_ERR);
        }

        if (srcImage == nullptr) {
            return rejectPromiseWithDOMException(
                executionContext, promise,
                DOMException::Code::INVALID_STATE_ERR);
        }

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

        ImageBitmap* imageBitmap = new ImageBitmap(executionContext, destImage);

        if (needsToSetOriginCleanFlag) {
            imageBitmap->setOriginCleanFlag(setOriginCleanFlagValue);
        }
        resolvePromise(executionContext, promise, imageBitmap);
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
