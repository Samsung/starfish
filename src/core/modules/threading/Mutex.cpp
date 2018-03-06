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

#include "StarFishConfig.h"
#include "Mutex.h"

namespace StarFish {

Mutex::Mutex()
{
    m_mutex = new pthread_mutex_t;
    pthread_mutex_init(m_mutex, NULL);

    GC_REGISTER_FINALIZER_NO_ORDER(this,
                                   [](void* obj, void* cd) {
                                       // STARFISH_LOG_INFO("Mutex::~Mutex\n");
                                       pthread_mutex_t* m =
                                           (pthread_mutex_t*)cd;
                                       auto check = pthread_mutex_destroy(m);
                                       delete m;
                                       STARFISH_ASSERT(check == 0);
                                   },
                                   m_mutex, NULL, NULL);
}

void Mutex::lock()
{
    pthread_mutex_lock(m_mutex);
}

void Mutex::unlock()
{
    pthread_mutex_unlock(m_mutex);
}
}
