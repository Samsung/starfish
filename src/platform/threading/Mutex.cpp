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
#include "Mutex.h"

namespace StarFish {

Mutex::Mutex()
{
    m_mutex = new pthread_mutex_t;
    pthread_mutex_init(m_mutex, NULL);

    GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
        // STARFISH_LOG_INFO("Mutex::~Mutex\n");
        pthread_mutex_t* m = (pthread_mutex_t*)cd;
        auto check = pthread_mutex_destroy(m);
        delete m;
        STARFISH_ASSERT(check == 0);
    }, m_mutex, NULL, NULL);
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
