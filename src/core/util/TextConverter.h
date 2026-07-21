/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishTextConverter__
#define __StarfishTextConverter__

#include "core/util/String.h"

namespace Starfish {

class TextConverter : public gc {
public:
    TextConverter(String* charsetName);
    TextConverter(String* mimetype, String* preferredEncoding,
                  const char* bytes, size_t len);
    ~TextConverter()
    {
        clearNativeResources();
    }
    String* convert(const char* bytes, size_t len, bool isEndOfStream);
    String* encoding()
    {
        return m_encoding;
    }

    void* operator new(size_t size);
    void clearNativeResources();
    // Objects allocated via GC_finalized_malloc must not be freed with
    // GC_FREE or delete. The no-op operator delete below prevents this.
    void operator delete(void*)
    {
    }
    void operator delete[](void*) = delete;

protected:
    UConverter* m_converter;
    String* m_encoding;
    std::basic_string<char, std::char_traits<char>,
                      gc_allocator_ignore_off_page<char>>
        m_bufferToConvert;
};
} // namespace Starfish

#endif
