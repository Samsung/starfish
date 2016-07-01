/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#ifndef __StarFishBlobObject__
#define __StarFishBlobObject__

#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class Blob : public ScriptWrappable {
public:
    Blob(uint32_t size, String* type, void* data)
        : ScriptWrappable(this)
    {
        m_size = size;
        m_type = type;
        m_data = data;
        // init
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    void* data()
    {
        return m_data;
    }

    uint32_t size()
    {
        return m_size;
    }

protected:
    uint32_t m_size;
    String* m_type;
    void* m_data;
};
}

#endif
