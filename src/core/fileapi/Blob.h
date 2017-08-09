/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __StarFishBlob__
#define __StarFishBlob__

#include "binding/ScriptWrappable.h"
#include "binding/DocumentHoldable.h"
#include "core/page/Serializer.h"

namespace StarFish {

class Document;

class Blob : public ScriptWrappable,
             public DocumentHoldable,
             public Serializable {
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

    Blob(Document* document, uint64_t size, String* type, void* data,
         bool isClosed, bool isEntryOfBlobURLStore)
        : ScriptWrappable(this)
        , DocumentHoldable(document)
        , m_blobData(size, type, data, isClosed, isEntryOfBlobURLStore)
    {
        if (m_blobData.m_isEntryOfBlobURLStore) {
            addBlobToBlobURLStore();
        }
    }

    Blob(Document* document, BlobData blobData);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isBlob() const override;
    virtual bool isSerializable() const override;
    virtual Serializable* toSerializable() const override;
    virtual ScriptBindingInstance* scriptBindingInstance();

    virtual SerializedData* serialized() override;

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

    ScriptWrappable* deserialized(Document* document) const override
    {
        return new Blob(document, m_data);
    }

private:
    Blob::BlobData m_data;
};
}

#endif
