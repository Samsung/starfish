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

#ifdef STARFISH_ENABLE_CANVAS

#include "StarfishConfig.h"
#include "EscargotPublic.h"
#include "binding/ScriptBindingInstance.h"
#include "core/dom/Document.h"
#include "core/dom/canvas/ImageData.h"
#include "core/dom/ExecutionContext.h"

#ifndef CRASH
#define CRASH STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE
#endif
#include "../third_party/escargot/third_party/checked_arithmetic/CheckedArithmetic.h"

namespace Starfish {
ImageData::ImageData(ExecutionContext* ownerExecutionContext)
    : ScriptWrappable(this)
    , m_executionContext(ownerExecutionContext)
{
}

ImageData::ImageData(ExecutionContext* ownerExecutionContext, uint32_t sw,
                     uint32_t sh)
    : ImageData(ownerExecutionContext)
{
    if (!sw || !sh) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INDEX_SIZE_ERR,
                               "sw and sh are must not zero");
    }
    Checked<int, RecordOverflow> dataSize = 4;
    dataSize *= sw;
    dataSize *= sh;
    if (dataSize.hasOverflowed()) {
        throw new DOMException(
            executionContext(), DOMException::Code::INDEX_SIZE_ERR,
            "The requested image size exceeds the supported range.");
    }

    initialize(sh, sw);
}

ImageData::ImageData(ExecutionContext* ownerExecutionContext,
                     ScriptUint8ClampedArray data, uint32_t sw,
                     Nullable<uint32_t> sh)
    : ImageData(ownerExecutionContext)
{
    size_t length = data->byteLength();
    if (!length || length % 4 != 0) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INVALID_STATE_ERR);
    }
    length = length / 4;
    if (!sw || length % sw != 0) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INDEX_SIZE_ERR);
    }
    size_t height = length / sw;
    if (sh.hasValue()) {
        if (height != sh.getValue()) {
            throw new DOMException(executionContext(),
                                   DOMException::Code::INDEX_SIZE_ERR);
        }
    }

    initialize(height, sw, data);
}

void ImageData::initialize(int32_t rows, int32_t pixelsPerRow,
                           ScriptUint8ClampedArray source)
{
    // https://html.spec.whatwg.org/multipage/canvas.html#create-an-imagedata-object
    if (source) {
        setData(source);
    } else {
        size_t destSize = rows * pixelsPerRow * 4;
        // TODO : If the Canvas Pixel ArrayBuffer cannot be allocated, then
        // throw the RangeError thrown by JavaScript, and return.
        auto scriptArrayBuffer = createScriptArrayBuffer(
            executionContext()->scriptBindingInstance(), destSize);
        auto canvasPixelArrayBuffer = createScriptValue(scriptArrayBuffer);
        ContextRef* ctx =
            executionContext()->scriptBindingInstance()->scriptContext();

        Evaluator::execute(
            ctx,
            [](ExecutionStateRef* state, ScriptValue canvasPixelArrayBuffer,
               ImageData* self, size_t destSize) -> ValueRef* {
                uint8_t* dest = canvasPixelArrayBuffer->toObject(state)
                                    ->asArrayBufferObject()
                                    ->rawBuffer();
                auto uint8ClampedArray = createEmptyUint8ClampedArray(
                    self->executionContext()->scriptBindingInstance());

                uint8ClampedArray->setBuffer(
                    canvasPixelArrayBuffer->toObject(state)
                        ->asArrayBufferObject(),
                    0, destSize, destSize);
                self->setData(uint8ClampedArray);

                return ValueRef::createUndefined();
            },
            canvasPixelArrayBuffer, this, destSize);
    }
    setWidth(pixelsPerRow);
    setHeight(rows);
}

ScriptBindingInstance* ImageData::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

SerializedData* ImageData::serialize(SerializingMap& memory)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return new SerializedImageData();
}

void ImageData::deserialize(SerializedData* serialized,
                            DeserializingMap& memory) const
{
}

uint32_t ImageData::width()
{
    return m_width;
}

void ImageData::setWidth(uint32_t value)
{
    m_width = value;
}

uint32_t ImageData::height()
{
    return m_height;
}

void ImageData::setHeight(uint32_t value)
{
    m_height = value;
}

ScriptUint8ClampedArray ImageData::data()
{
    return m_data;
}

void ImageData::setData(ScriptUint8ClampedArray value)
{
    m_data = value;
}

ScriptWrappable* SerializedImageData::createDeserializingInstance(
    ExecutionContext* executionContext) const
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return nullptr;
}
}
#undef CRASH
#endif
