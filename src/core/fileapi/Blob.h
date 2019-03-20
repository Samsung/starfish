/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishBlob__
#define __StarfishBlob__

#include "binding/ScriptWrappable.h"
#include "core/page/Serializer.h"

namespace Starfish {

class ExecutionContext;

class Blob : public ScriptWrappable, public Serializable {
public:
    struct BlobData {
        uint64_t m_size;
        String* m_type;
        void* m_data;
        bool m_isClosed;
        bool m_isEntryOfBlobURLStore;

        BlobData(uint64_t size, String* type, void* data, bool isClosed,
                 bool isEntryOfBlobURLStore)
            : m_size(size)
            , m_type(type)
            , m_data(data)
            , m_isClosed(isClosed)
            , m_isEntryOfBlobURLStore(isEntryOfBlobURLStore)
        {
        }
    };

    Blob(ExecutionContext* executionContext, uint64_t size, String* type,
         void* data, bool isClosed, bool isEntryOfBlobURLStore)
        : ScriptWrappable(this, executionContext)
        , m_blobData(size, type, data, isClosed, isEntryOfBlobURLStore)
    {
        if (m_blobData.m_isEntryOfBlobURLStore) {
            addBlobToBlobURLStore();
        }
    }

    Blob(ExecutionContext* executionContext, BlobData blobData);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(Blob)

    virtual bool isSerializable() const override;
    virtual Serializable* toSerializable() const override;

    virtual SerializedData* serialize(SerializingMap& memory) override;
    virtual void deserialize(SerializedData* serialized,
                             DeserializingMap& memory) const override
    {
    }

    void* data()
    {
        return m_blobData.m_data;
    }

    uint64_t size()
    {
        return m_blobData.m_size;
    }

    String* type()
    {
        return m_blobData.m_type;
    }

    bool isClosed()
    {
        return m_blobData.m_isClosed;
    }

    bool isEntryOfBlobURLStore()
    {
        return m_blobData.m_isEntryOfBlobURLStore;
    }

    void close()
    {
        if (m_blobData.m_isClosed) {
            return;
        } else {
            m_blobData.m_isClosed = true;
        }
        if (m_blobData.m_isEntryOfBlobURLStore) {
            removeBlobFromBlobURLStore();
            m_blobData.m_isEntryOfBlobURLStore = false;
        }
    }

    Blob* slice(int64_t start = 0);
    Blob* slice(int64_t start, int64_t end,
                String* contentType = String::emptyString);

protected:
    void addBlobToBlobURLStore();
    void removeBlobFromBlobURLStore();

    BlobData m_blobData;
};

class SerializedBlobData : public SerializedPlatformObjectData {
public:
    SerializedBlobData(Blob::BlobData data)
        : m_data(data)
    {
    }

    ScriptWrappable* createDeserializingInstance(
        ExecutionContext* executionContext) const override
    {
        return new Blob(executionContext, m_data);
    }

private:
    Blob::BlobData m_data;
};
}

#endif
