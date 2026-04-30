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

Mutex::Mutex(const char* name)
{
#ifndef NDEBUG
    m_name = name;
#endif

    pthread_mutex_init(&m_mutex, nullptr);

    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            Mutex* self = static_cast<Mutex*>(obj);
            auto check = pthread_mutex_destroy(&self->m_mutex);
            STARFISH_ASSERT(check == 0);
        },
        nullptr, nullptr, nullptr);
}

Mutex::~Mutex()
{
    GC_REGISTER_FINALIZER_NO_ORDER(this, nullptr, nullptr, nullptr, nullptr);
    auto check = pthread_mutex_destroy(&m_mutex);
    STARFISH_ASSERT(check == 0);
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
