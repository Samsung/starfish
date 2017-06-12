/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "HTTPHeaderList.h"

namespace StarFish {
HTTPHeaderList::HTTPHeaderList()
    : m_length(0)
    , m_head(nullptr)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            STARFISH_LOG_INFO("HTTPHeaderList::~HTTPHeaderList\n");
            HTTPHeaderList* list = (HTTPHeaderList*)obj;
            if (list->m_head != nullptr) {
                curl_slist_free_all(list->m_head);
                list->m_head = nullptr;
            }
        },
        NULL, NULL, NULL);
}

void HTTPHeaderList::append(String* header)
{
    append(header->utf8Data());
}

void HTTPHeaderList::append(const char* header)
{
    this->m_head = curl_slist_append(this->m_head, header);
    if (this->m_head == nullptr) {
        STARFISH_ASSERT_NOT_REACHED();
    }
    ++this->m_length;
}
}
