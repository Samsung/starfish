/*
 * Copyright (c) 2020-present Samsung Electronics Co., Ltd
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
#include "WindowProxy.h"

#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptBindingSecurity.h"
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"

#include <EscargotPublic.h>

using namespace Escargot;

namespace Starfish {

WindowProxy::WindowProxy(Window* window)
    : ScriptWrappable(this)
{
    updateSource(window);
}

void WindowProxy::updateSource(Window* window)
{
    m_window = window;
    scriptObject()->setExtraData(m_window);
    m_window->scriptBindingInstance()->scriptContext()->setGlobalObjectProxy(
        scriptObject());
    m_object->asGlobalObjectProxyObject()->setTarget(
        window->scriptBindingInstance()->scriptContext()->globalObject());
}

void WindowProxy::init(ScriptBindingInstance* instance, void* domObjectPointer)
{
    Evaluator::execute(
        instance->scriptContext(),
        [](ExecutionStateRef* state, WindowProxy* self) -> ValueRef* {
            self->m_object = GlobalObjectProxyObjectRef::create(
                state, state->context()->globalObject(),
                [](ExecutionStateRef* state, GlobalObjectProxyObjectRef* proxy,
                   GlobalObjectRef* targetGlobalObject,
                   GlobalObjectProxyObjectRef::AccessOperationType
                       operationType,
                   OptionalRef<AtomicStringRef>
                       nonIndexedStringPropertyNameIfExists) {
                    Window* window = ((Window*)proxy->extraData());
                    Document* accessorSourceDocument =
                        ((Window*)state->context()
                             ->globalObjectProxy()
                             ->extraData())
                            ->document();
                    Document* proxyDocument = window->document();

                    if (accessorSourceDocument != proxyDocument &&
                        !ScriptBindingSecurity::canAccess(
                            accessorSourceDocument, proxyDocument)) {
                        bool allow = false;
                        if (nonIndexedStringPropertyNameIfExists) {
#define ALLOW_READ(keyword)                                               \
    else if (nonIndexedStringPropertyNameIfExists.value()                 \
                 ->string()                                               \
                 ->equalsWithASCIIString(keyword, sizeof(keyword) - 1) && \
             operationType == GlobalObjectProxyObjectRef::Read)           \
    {                                                                     \
        allow = true;                                                     \
    }

#define ALLOW_READ_WRITE(keyword)                                       \
    else if (nonIndexedStringPropertyNameIfExists.value()               \
                 ->string()                                             \
                 ->equalsWithASCIIString(keyword, sizeof(keyword) - 1)) \
    {                                                                   \
        allow = true;                                                   \
    }
                            if (false) {
                            }
                            ALLOW_READ("window")
                            ALLOW_READ("closed")
                            ALLOW_READ("frames")
                            ALLOW_READ("length")
                            ALLOW_READ_WRITE("location")
                            ALLOW_READ("opener")
                            ALLOW_READ("parent")
                            ALLOW_READ("self")
                            ALLOW_READ("top")
                            ALLOW_READ("postMessage")
                            ALLOW_READ("blur")
                            ALLOW_READ("close")
                            ALLOW_READ("focus")
                            ALLOW_READ_WRITE("pagePopupController")
                        }

                        if (!allow) {
                            DOMException* exception = new DOMException(
                                accessorSourceDocument->executionContext(),
                                DOMException::Code::SECURITY_ERR);
                            state->throwException(exception->scriptValue());
                        }
                    }
                });
            return ValueRef::createUndefined();
        },
        this);

    postInit(instance);
}

ScriptBindingInstance* WindowProxy::scriptBindingInstance()
{
    return m_window->scriptBindingInstance();
}
} // namespace Starfish
