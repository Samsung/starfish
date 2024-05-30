/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishGCDescriptor__
#define __StarfishGCDescriptor__

#define _NEW_WITH_GC_DESC_ARG1(Class)                             \
    void* operator new(size_t size)                               \
    {                                                             \
        static bool typeInited = false;                           \
        static GC_descr descr;                                    \
        if (!typeInited) {                                        \
            GC_word desc[GC_BITMAP_SIZE(Class)] = { 0 };          \
            Class::fillGCDescriptor(desc);                        \
            descr = GC_make_descriptor(desc, GC_WORD_LEN(Class)); \
            typeInited = true;                                    \
        }                                                         \
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);           \
    }                                                             \
    void* operator new[](size_t size) = delete;                   \
                                                                  \
protected:                                                        \
    static inline void fillGCDescriptor(GC_word* desc)            \
    {
#define _NEW_WITH_GC_DESC_ARG2(Class, Parent) \
    _NEW_WITH_GC_DESC_ARG1(Class)             \
    Parent::fillGCDescriptor(desc);

#define _VA_MACRO(_1, _2, x, ...) x

// After this macro is used, class access specifier is changed to `protected`.
#define BEGIN_IMPLEMENT_NEW_WITH_GC_DESC(...)                              \
    _VA_MACRO(__VA_ARGS__, _NEW_WITH_GC_DESC_ARG2, _NEW_WITH_GC_DESC_ARG1) \
    (__VA_ARGS__)

#define FILL_GC_POINTER(Class, name) \
    GC_set_bit(desc, GC_WORD_OFFSET(Class, name));
#define FILL_GC_COLLECTION(Class, name) \
    markHashTable(desc, GC_WORD_OFFSET(Class, name));

#define END_IMPLEMENT_NEW_WITH_GC_DESC() }

#endif
