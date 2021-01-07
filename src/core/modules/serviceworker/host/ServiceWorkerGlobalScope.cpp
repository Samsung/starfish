/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifdef STARFISH_WEBWORKER_HOST

#include "StarfishConfig.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptBindingWorkerInstance.h"
#include "core/modules/worker/host/WebWorker.h"
#include "core/modules/serviceworker/host/ServiceWorkerGlobalScope.h"

namespace Starfish {

ServiceWorkerGlobalScope::ServiceWorkerGlobalScope(WebWorker* webWorker,
                                                   ResourceURL* url,
                                                   String* charSet)
    : WorkerGlobalScope(webWorker)
{
    STARFISH_ASSERT(webWorker != nullptr);
    STARFISH_ASSERT(url != nullptr);
    STARFISH_ASSERT(charSet != nullptr);

    m_scriptBindingInstance =
        new ScriptBindingWorkerInstance<ServiceWorkerGlobalScope>(
            webWorker->scriptEngineInstance(), this);

    initGlobalScope(url, charSet);
}

void* ServiceWorkerGlobalScope::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(ServiceWorkerGlobalScope));
    static bool typeInited = false;
    static GC_descr descr;
    if (typeInited == false) {
        GC_word desc[GC_BITMAP_SIZE(ServiceWorkerGlobalScope)] = { 0 };
        ServiceWorkerGlobalScope::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(ServiceWorkerGlobalScope));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}
} // namespace Starfish

#endif /* STARFISH_WEBWORKER_HOST */
