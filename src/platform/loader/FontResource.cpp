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
#include "StarFish.h"
#include "core/dom/Document.h"
#include "platform/loader/FontResource.h"
#include "platform/loader/ResourceLoader.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/page/Window.h"
#include "core/page/BrowsingContext.h"

namespace StarFish {

void FontResource::didLoadFinished()
{
    m_fontFace =
        FontFace::create((const uint8_t*)m_resourceRequest->response().data(),
                         m_resourceRequest->response().size());
    if (!m_fontFace) {
        Resource::didLoadFailed();
        return;
    }
    Resource::didLoadFinished();
}
}
