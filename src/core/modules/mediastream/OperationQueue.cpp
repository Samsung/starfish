/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_WEBRTC)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "EscargotPublic.h"
#include "core/modules/mediastream/OperationQueue.h"

#include "core/dom/ExecutionContext.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/WebBase.h"
#include "core/page/GlobalScope.h"

namespace Starfish {

// https://w3c.github.io/webrtc-pc/#enqueue-an-operation
void OperationQueue::enqueue(OperationFunction fn, Promise* fnPromise,
                             void* data)
{
    // TODO: Impl operation queue
    struct Params : public gc {
        OperationFunction fn;
        Promise* fnPromise;
        void* data;
    };
    Params* p = new Params();
    p->fn = fn;
    p->fnPromise = fnPromise;
    p->data = data;

    m_executionContext->webBase()->messageLoop()->addIdler(
        m_executionContext->globalScope(),
        [](size_t, void* data) {
            Params* p = castTo<Params*>(data);
            p->fn(p->fnPromise, p->data);
        },
        p);
}
}

#endif
