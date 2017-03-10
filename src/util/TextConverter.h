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

#ifndef __StarFishTextConverter__
#define __StarFishTextConverter__

#include "util/String.h"

namespace StarFish {

class TextConverter : public gc {
public:
    TextConverter(String* charsetName);
    TextConverter(String* mimetype, String* preferredEncoding,
                  const char* bytes, size_t len);
    ~TextConverter();
    String* convert(const char* bytes, size_t len, bool isEndOfStream);
    String* encoding()
    {
        return m_encoding;
    }

protected:
    void registerFinalizer();
    UConverter* m_converter;
    String* m_encoding;
    std::basic_string<char, std::char_traits<char>,
                      gc_allocator_ignore_off_page<char>>
        m_bufferToConvert;
};
}

#endif
