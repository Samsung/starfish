/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "binding/ScriptEngineInstance.h"
#include "binding/ScriptBindingInstance.h"

#include "core/page/WebView.h"
#include "core/page/Window.h"

#include "core/modules/message_loop/MessageLoop.h"

#include <EscargotPublic.h>

namespace Starfish {

ScriptEngineInstance::ScriptEngineInstance(const char* locale,
                                           const char* timezone, WebView* wv)
{
    class EscargotStarfishPlatform : public Escargot::PlatformRef {
    public:
        EscargotStarfishPlatform(WebView* wv)
            : m_webView(wv)
        {
        }

        virtual void didPromiseJobEnqueued(
            Escargot::ContextRef* relatedContext,
            Escargot::PromiseObjectRef* obj) override
        {
            Window* window =
                (Window*)relatedContext->globalObject()->extraData();

            window->webView()->messageLoop()->addMicroTask(
                window,
                [](size_t handle, void* data) {
                    ContextRef* relatedContext = (ContextRef*)data;
                    Window* window =
                        (Window*)relatedContext->globalObject()->extraData();

                    if (relatedContext->vmInstance()->hasPendingPromiseJob()) {
                        auto jobResult = relatedContext->vmInstance()
                                             ->executePendingPromiseJob();
                        if (jobResult.error) {
                            STARFISH_LOG_ERROR(
                                "Uncaught %s in Promise job\n",
                                toBrowserString(window->scriptBindingInstance(),
                                                jobResult.error.value())
                                    ->toUTF8NonGCString()
                                    .data());
                        }
                    }
                },
                relatedContext);
        }

        virtual LoadModuleResult onLoadModule(
            Escargot::ContextRef* relatedContext,
            Escargot::ScriptRef* whereRequestFrom,
            Escargot::StringRef* moduleSrc) override
        {
            return LoadModuleResult(Escargot::ErrorObjectRef::Code::None,
                                    Escargot::StringRef::emptyString());
        }

        virtual void didLoadModule(
            Escargot::ContextRef* relatedContext,
            Escargot::OptionalRef<Escargot::ScriptRef> referrer,
            Escargot::ScriptRef* loadedModule) override
        {
        }

        WebView* m_webView;
    };

    m_engineInstance = Escargot::VMInstanceRef::create(
        new EscargotStarfishPlatform(wv), locale, timezone);
}

void ScriptEngineInstance::dispose()
{
    m_engineInstance = nullptr;
}
}
