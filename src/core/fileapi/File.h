/*
 * Copyright (c) 2021-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishFile__
#define __StarfishFile__

#include "core/fileapi/Blob.h"
#include "binding/generated/BufferSourceOrBlobOrDOMStringUnion.h"
#include "core/fileapi/FilePropertyBag.h"

namespace Starfish {

class File : public Blob {
public:
    File(ExecutionContext* executionContext,
         const GCVector<BufferSourceOrBlobOrDOMString>& fileBits,
         String* fileName, const FilePropertyBag& options = {});
    File(ExecutionContext* executionContext, BlobData blobData);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(File)

    virtual bool isSerializable() const override;

    virtual Serializable* toSerializable() const override;

    virtual SerializedData* serialize(SerializingMap& memory) override
    {
        return nullptr;
    }

    virtual void deserialize(SerializedData* serialized,
                             DeserializingMap& memory) const override
    {
    }

    String* name()
    {
        return m_name;
    }

    int64_t lastModified()
    {
        return m_lastModified;
    }

private:
    void init(const GCVector<BufferSourceOrBlobOrDOMString>& fileBits,
              const FilePropertyBag& options);

    GCVector<BufferSourceOrBlobOrDOMString> m_fileBits;
    String* m_name = nullptr;
    int64_t m_lastModified = 0;
};

} // namespace Starfish

#endif
