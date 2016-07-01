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

#include "StarFishConfig.h"
#include "Semaphore.h"

namespace StarFish {

Semaphore::Semaphore(size_t cnt)
{
    m_semaphore = new sem_t;
    sem_init(m_semaphore, 0, cnt);
    GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
        sem_t* m = (sem_t*)cd;
        sem_destroy(m);
        delete m;
    }, m_semaphore, NULL, NULL);
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
