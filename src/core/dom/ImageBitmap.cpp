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
#include "core/dom/ExecutionContext.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "core/dom/ImageBitmap.h"
#ifdef STARFISH_ENABLE_CANVAS
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/image/BufferedNativeImageData.h"
#endif

namespace Starfish {

ImageBitmap::ImageBitmap(ExecutionContext* executionContext,
                         NativeImageData* nativeImageData)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_nativeImageData(nativeImageData)
{
    STARFISH_ASSERT(executionContext != nullptr);
}

ScriptBindingInstance* ImageBitmap::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

uint32_t ImageBitmap::width()
{
    if (isDetached()) {
        return 0;
    }
    return m_nativeImageData->width();
}

uint32_t ImageBitmap::height()
{
    if (isDetached()) {
        return 0;
    }
    return m_nativeImageData->height();
}

void ImageBitmap::close()
{
    setDetached();
    m_nativeImageData = nullptr;
    m_premultipliedImageData = nullptr;
}

#ifdef STARFISH_ENABLE_CANVAS
NativeImageData* ImageBitmap::premultipliedImageData()
{
    if (m_isPremultiplied || !m_nativeImageData) {
        return m_nativeImageData;
    }
    // Kept, so a page that paints the bitmap every frame copies it once; the
    // copy's pixels are malloc'ed, out of the GC's sight.
    if (!m_premultipliedImageData) {
        const size_t width = m_nativeImageData->width();
        const size_t height = m_nativeImageData->height();
        NativeImageData* copy = BufferedNativeImageData::create(width, height);
        STARFISH_ASSERT(copy->stride() == m_nativeImageData->stride());
        memcpy(copy->data(), m_nativeImageData->data(), copy->bufferSize());
        convertImageBufferAsPremultipliedAlphaIfNeeds(copy->data(), width,
                                                      copy->stride(), height);
        m_premultipliedImageData = copy;
    }
    return m_premultipliedImageData;
}
#endif

SerializedData* ImageBitmap::serialize(SerializingMap& memory)
{
    STARFISH_UNSUPPORTED_METHOD();
    // 1.Set serialized.[[BitmapData]] to a copy of value's bitmap data.
    // 2.Set serialized.[[OriginClean]] to true if value's origin-clean flag is
    // set, and false otherwise.
    return nullptr;
}

void ImageBitmap::deserialize(SerializedData* serialized,
                              DeserializingMap& memory) const
{
    STARFISH_UNSUPPORTED_METHOD();
    // 1.Set value's bitmap data to serialized.[[BitmapData]].
    // 2.If serialized.[[OriginClean]] is true, set value's origin-clean flag.
}

TransferedData* ImageBitmap::transfer()
{
    STARFISH_UNSUPPORTED_METHOD();
    // 1.Set dataHolder.[[BitmapData]] to value's bitmap data.
    // 2.Set dataHolder.[[OriginClean]] to true if value's origin-clean flag is
    // set, and false otherwise.
    // 3.Unset value's bitmap data.
    return nullptr;
}

void ImageBitmap::transferReceive(TransferedData* transfered)
{
    STARFISH_UNSUPPORTED_METHOD();
    // 1.Set value's bitmap data to dataHolder.[[BitmapData]].
    // 2.If dataHolder.[[OriginClean]] is true, set value's origin-clean flag.
}

} // namespace Starfish
