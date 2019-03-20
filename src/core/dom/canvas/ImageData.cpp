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
#include "core/dom/Document.h"
#include "core/dom/canvas/ImageData.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {
ImageData::ImageData(ExecutionContext* ownerExecutionContext)
    : ScriptWrappable(this, ownerExecutionContext)
    , m_data(createEmptyUint8ClampedArray(
          executionContext()->ownerScriptBindingInstance()))
{
}

ImageData::ImageData(ExecutionContext* ownerExecutionContext,
                     ScriptUint8ClampedArray array)
    : ScriptWrappable(this, ownerExecutionContext)
    , m_data(array)
{
}

ScriptBindingInstance* ImageData::scriptBindingInstance()
{
    return executionContext()->ownerScriptBindingInstance();
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
#endif
