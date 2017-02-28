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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined(__StarFishMimeType__)
#define __StarFishMimeType__

namespace StarFish {

class String;

class MimeType {
public:
    MimeType();

    static MimeType parseFromString(String* str);

    bool isValid();
    bool hasParameter();

    String* type()
    {
        return m_type;
    }
    String* subtype()
    {
        return m_subtype;
    }
    String* parameter()
    {
        return m_parameter;
    }
    void setType(String* type)
    {
        m_type = type;
    }
    void setSubtype(String* subtype)
    {
        m_subtype = subtype;
    }
    void setParameter(String* param)
    {
        m_parameter = param;
    }

    String* string();
    void clear();

protected:
    String* m_type;
    String* m_subtype;
    // TODO : store parameter as dictionary
    String* m_parameter;
};
}

#endif
