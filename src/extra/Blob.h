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

namespace StarFish {

class StarFish;

class Blob : public ScriptWrappable {
public:
    Blob(StarFish* starFish, uint64_t size, String* type, void* data,
         bool isClosed, bool isEntryOfBlobURLStore)
        : ScriptWrappable(this)
    {
        m_starFish = starFish;
        m_size = size;
        m_type = type;
        m_data = data;
        m_isClosed = isClosed;
        if (isEntryOfBlobURLStore) {
            addBlobToBlobURLStore();
        }
    }

    virtual void init(ScriptBindingInstance* instance) override
    {
        scriptObject()->set__proto__(
            fetchData(instance)->fnBlob()->protoType());
    }

    virtual bool isBlob() const override
    {
        return true;
    }

    StarFish* starFish()
    {
        return m_starFish;
    }

    void* data()
    {
        return m_data;
    }

    uint64_t size()
    {
        return m_size;
    }

    String* type()
    {
        return m_type;
    }

    bool isClosed()
    {
        return m_isClosed;
    }

    void close()
    {
        if (m_isClosed) {
            return;
        } else {
            m_isClosed = true;
        }
        if (m_isEntryOfBlobURLStore) {
            removeBlobFromBlobURLStore();
            m_isEntryOfBlobURLStore = false;
        }
    }

    Blob* slice(int64_t start = 0);
    Blob* slice(int64_t start, int64_t end,
                String* contentType = String::emptyString);

protected:
    void addBlobToBlobURLStore();
    void removeBlobFromBlobURLStore();

    bool m_isEntryOfBlobURLStore;
    bool m_isClosed;
    StarFish* m_starFish;
    String* m_type;
    void* m_data;
    uint64_t m_size;
};
}

#endif
