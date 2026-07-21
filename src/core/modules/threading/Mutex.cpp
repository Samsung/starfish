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

#include "StarfishConfig.h"
#include "Mutex.h"

namespace Starfish {

static void mutexClear(void* obj, void* cd)
{
    Mutex* self = reinterpret_cast<Mutex*>(obj);
    self->clearNativeResources();
}

void* Mutex::operator new(size_t size)
{
    constexpr static GC_finalizer_closure data = { mutexClear, nullptr };
    return GC_finalized_malloc(size, &data);
}

void* Mutex::operator new(size_t size, GCPlacement placement)
{
    // Only NoGC placement is allowed for Mutex.
    // GC-allocated Mutex would never have its finalizer called
    // (GC_finalized_malloc is used by the default operator new instead).
    STARFISH_ASSERT(placement == NoGC);
    return gc::operator new(size, placement);
}

void Mutex::clearNativeResources()
{
    auto check = pthread_mutex_destroy(&m_mutex);
    STARFISH_ASSERT(check == 0);
}

Mutex::Mutex(const char* name)
{
#ifndef NDEBUG
    m_name = name;
#endif

    pthread_mutex_init(&m_mutex, nullptr);
}

Mutex::~Mutex()
{
    clearNativeResources();
}

void Mutex::lock()
{
#ifndef NDEBUG
    if (!m_name.empty()) {
        STARFISH_LOG_WARN("Lock: %s", m_name.c_str());
    }
#endif
    pthread_mutex_lock(&m_mutex);
}

void Mutex::unlock()
{
#ifndef NDEBUG
    if (!m_name.empty()) {
        STARFISH_LOG_WARN("Unlock: %s", m_name.c_str());
    }
#endif
    pthread_mutex_unlock(&m_mutex);
}
} // namespace Starfish
