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

#ifndef __StarfishImageBitmap__
#define __StarfishImageBitmap__

#include "binding/ScriptWrappable.h"
#include "core/page/Serializer.h"

namespace Starfish {

class NativeImageData;

class ImageBitmap : public ScriptWrappable,
                    public Serializable,
                    public Transferable {
public:
    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(ImageBitmap)

    ImageBitmap(ExecutionContext* executionContext,
                NativeImageData* nativeImageData);
    ~ImageBitmap()
    {
    }

    // Interface ImageBitmap
    uint32_t width();
    uint32_t height();
    void close();

    // Interface Serializable
    virtual bool isSerializable() const override;
    virtual Serializable* toSerializable() const override;
    virtual SerializedData* serialize(SerializingMap& memory) override;
    virtual void deserialize(SerializedData* serialized,
                             DeserializingMap& memory) const override;

    // Interface Transferable
    virtual bool isTransferable() const override;
    virtual Transferable* toTransferable() const override;
    virtual TransferedData* transfer() override;
    virtual void transferReceive(TransferedData* transfered) override;

    bool originCleanFlag()
    {
        return m_originCleanFlag;
    }

    void setOriginCleanFlag(bool value)
    {
        m_originCleanFlag = value;
    }

    NativeImageData* nativeImageData()
    {
        return m_nativeImageData;
    }

private:
    ExecutionContext* m_executionContext{ nullptr };
    NativeImageData* m_nativeImageData{ nullptr };
    bool m_originCleanFlag{ true };
};

class SerializedImageBitmap : public SerializedPlatformObjectData {
public:
    SerializedImageBitmap()
    {
    }

    virtual ScriptWrappable* createDeserializingInstance(
        ExecutionContext* executionContext) const override;
};

class TransferedImageBitmap : public TransferedPlatformObjectData {
public:
    TransferedImageBitmap(ImageBitmap* origin)
    {
    }

    ScriptWrappable* createTransferReceivingInstance(
        ExecutionContext* executionContext) const override;
};
} // namespace Starfish
#endif
