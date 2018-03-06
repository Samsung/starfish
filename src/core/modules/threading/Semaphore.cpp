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
#include "Semaphore.h"

namespace StarFish {

Semaphore::Semaphore(size_t cnt)
{
    m_semaphore = new sem_t;
    sem_init(m_semaphore, 0, cnt);
    GC_REGISTER_FINALIZER_NO_ORDER(this,
                                   [](void* obj, void* cd) {
                                       sem_t* m = (sem_t*)cd;
                                       sem_destroy(m);
                                       delete m;
                                   },
                                   m_semaphore, NULL, NULL);
}

void Semaphore::lock()
{
    sem_wait(m_semaphore);
}

void Semaphore::unlock()
{
    sem_post(m_semaphore);
}
}
