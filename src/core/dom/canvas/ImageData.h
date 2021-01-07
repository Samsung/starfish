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

#ifndef __StarfishImageData__
#define __StarfishImageData__

#ifdef STARFISH_ENABLE_CANVAS

#include "binding/ScriptWrappable.h"
#include "core/page/Serializer.h"

namespace Starfish {
class ImageData : public ScriptWrappable, public Serializable {
public:
    ImageData(ExecutionContext* ownerExecutionContext, uint32_t sw,
              uint32_t sh);
    ImageData(ExecutionContext* ownerExecutionContext,
              ScriptUint8ClampedArray data, uint32_t sw, Nullable<uint32_t> sh);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isImageData() const override;
    virtual ScriptBindingInstance* scriptBindingInstance();

    virtual bool isSerializable() const override;
    virtual Serializable* toSerializable() const override;
    virtual SerializedData* serialize(SerializingMap& memory) override;
    virtual void deserialize(SerializedData* serialized,
                             DeserializingMap& memory) const override;

    uint32_t width();
    void setWidth(uint32_t value);

    uint32_t height();
    void setHeight(uint32_t value);

    ScriptUint8ClampedArray data();
    void setData(ScriptUint8ClampedArray value);

    ExecutionContext* executionContext()
    {
        return m_executionContext;
    }

private:
    ImageData(ExecutionContext* ownerExecutionContext);

    void initialize(int32_t rows, int32_t pixelsPerRow,
                    ScriptUint8ClampedArray source = nullptr);

    ExecutionContext* m_executionContext;
    ScriptUint8ClampedArray m_data;
    uint32_t m_width;
    uint32_t m_height;
};

class SerializedImageData : public SerializedPlatformObjectData {
public:
    SerializedImageData()
    {
    }

    virtual ScriptWrappable* createDeserializingInstance(
        ExecutionContext* executionContext) const override;
};
} // namespace Starfish
#endif
#endif
