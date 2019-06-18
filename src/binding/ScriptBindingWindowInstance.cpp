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

#include "StarfishConfig.h"
#include "core/page/BrowsingContext.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptBindingWindowInstance.h"
#include "binding/ScriptWrappable.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/dom/Event.h"

#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
#include "core/extra/Avplay.h"
#endif

#include <EscargotPublic.h>

namespace Starfish {

static NullablePtr<ValueRef> virtualIdentifierCallback(ExecutionStateRef* state,
                                                       ValueRef* key)
{
    Window* self = fetchWindow(state->context());

    auto callee = state->resolveCallee();
    if (callee) {
        void* data = callee.getValue()->asObject()->extraData();
        if (data) {
            ScriptWrappable* w = (ScriptWrappable*)data;
            if (w->isAttributeEventFunction()) {
                auto elementDOMObject =
                    ((AttributeEventFunction*)w)->target()->scriptValue();
                if (elementDOMObject->isObject()) {
                    bool exist = elementDOMObject->asObject()->hasOwnProperty(
                        state, key);
                    if (exist) {
                        return elementDOMObject->asObject()->getOwnProperty(
                            state, key);
                    }
                    return ValueRef::createEmpty();
                }
            }
        }
    }

    uint32_t idx = key->toArrayIndex(state);
    if (idx != ValueRef::InvalidArrayIndexValue) {
        Window* result = self->defaultIndexedGetter(idx);
        if (result != nullptr) {
            return result->scriptValue();
        }
    }
    String* name = toBrowserString(state, key);
    Nullable<ScriptObject> coll = self->defaultNamedGetter(name);
    if (coll.hasValue()) {
        return ValueRef::create(coll.getValue());
    }

    if (name->equals("self")) {
        return self->scriptValue();
    }
#ifdef STARFISH_ENABLE_OBSOLETE_SPEC
    else if (name->equals("event") && self->event()) {
        return self->event()->scriptValue();
    }
#endif

    return ValueRef::createEmpty();
}

#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
static ValueRef* _openAvplayFunction(ExecutionStateRef* state,
                                     ValueRef* thisValue, size_t argc,
                                     ValueRef** argv, bool isNewExpression)
{
    fetchWebView(state->context())
        ->avplay()
        ->open(toBrowserString(state, argv[0]));
    return ValueRef::createUndefined();
}

static ValueRef* _prepareAvplayFunction(ExecutionStateRef* state,
                                        ValueRef* thisValue, size_t argc,
                                        ValueRef** argv, bool isNewExpression)
{
    fetchWebView(state->context())->avplay()->prepare();
    return ValueRef::createUndefined();
}

static ValueRef* _setDisplayRectAvplayFunction(ExecutionStateRef* state,
                                               ValueRef* thisValue, size_t argc,
                                               ValueRef** argv,
                                               bool isNewExpression)
{
    double arg1 = argv[0]->toNumber(state);
    double arg2 = argv[1]->toNumber(state);
    double arg3 = argv[2]->toNumber(state);
    double arg4 = argv[3]->toNumber(state);
    fetchWebView(state->context())
        ->avplay()
        ->setDisplayRect(arg1, arg2, arg3, arg4);
    return ValueRef::createUndefined();
}

static ValueRef* _playAvplayFunction(ExecutionStateRef* state,
                                     ValueRef* thisValue, size_t argc,
                                     ValueRef** argv, bool isNewExpression)
{
    fetchWebView(state->context())->avplay()->play();
    return ValueRef::createUndefined();
}

static ValueRef* _closeAvplayFunction(ExecutionStateRef* state,
                                      ValueRef* thisValue, size_t argc,
                                      ValueRef** argv, bool isNewExpression)
{
    fetchWebView(state->context())->avplay()->close();
    return ValueRef::createUndefined();
}

static ValueRef* _pauseAvplayFunction(ExecutionStateRef* state,
                                      ValueRef* thisValue, size_t argc,
                                      ValueRef** argv, bool isNewExpression)
{
    fetchWebView(state->context())->avplay()->pause();
    return ValueRef::createUndefined();
}

static ValueRef* _stopAvplayFunction(ExecutionStateRef* state,
                                     ValueRef* thisValue, size_t argc,
                                     ValueRef** argv, bool isNewExpression)
{
    fetchWebView(state->context())->avplay()->stop();
    return ValueRef::createUndefined();
}

static ValueRef* _suspendAvplayFunction(ExecutionStateRef* state,
                                        ValueRef* thisValue, size_t argc,
                                        ValueRef** argv, bool isNewExpression)
{
    fetchWebView(state->context())->avplay()->suspend();
    return ValueRef::createUndefined();
}

static ValueRef* _restoreAvplayFunction(ExecutionStateRef* state,
                                        ValueRef* thisValue, size_t argc,
                                        ValueRef** argv, bool isNewExpression)
{
    fetchWebView(state->context())->avplay()->restore();
    return ValueRef::createUndefined();
}

static ValueRef* _getStateAvplayFunction(ExecutionStateRef* state,
                                         ValueRef* thisValue, size_t argc,
                                         ValueRef** argv, bool isNewExpression)
{
    String* ret = fetchWebView(state->context())->avplay()->getState();
    return ValueRef::create(toJSString(ret));
}

static ValueRef* _getCurrentTimeAvplayFunction(ExecutionStateRef* state,
                                               ValueRef* thisValue, size_t argc,
                                               ValueRef** argv,
                                               bool isNewExpression)
{
    return ValueRef::create(
        fetchWebView(state->context())->avplay()->getCurrentTime());
}

static ValueRef* _getDurationAvplayFunction(ExecutionStateRef* state,
                                            ValueRef* thisValue, size_t argc,
                                            ValueRef** argv,
                                            bool isNewExpression)
{
    return ValueRef::create(
        fetchWebView(state->context())->avplay()->getDuration());
}

static ValueRef* _setStreamingPropertyAvplayFunction(ExecutionStateRef* state,
                                                     ValueRef* thisValue,
                                                     size_t argc,
                                                     ValueRef** argv,
                                                     bool isNewExpression)
{
    String* arg1 = toBrowserString(state, argv[0]);
    String* arg2 = toBrowserString(state, argv[1]);
    fetchWebView(state->context())->avplay()->setStreamingProperty(arg1, arg2);
    return ValueRef::createUndefined();
}

static ValueRef* _prepareAsyncAvplayFunction(ExecutionStateRef* state,
                                             ValueRef* thisValue, size_t argc,
                                             ValueRef** argv,
                                             bool isNewExpression)
{
    fetchWebView(state->context())->avplay()->prepareAsync(argv[0]);
    return ValueRef::createUndefined();
}

static ValueRef* _setListenerAvplayFunction(ExecutionStateRef* state,
                                            ValueRef* thisValue, size_t argc,
                                            ValueRef** argv,
                                            bool isNewExpression)
{
    fetchWebView(state->context())->avplay()->setListener(argv[0]);
    return ValueRef::createUndefined();
}

static ValueRef* _seekToAvplayFunction(ExecutionStateRef* state,
                                       ValueRef* thisValue, size_t argc,
                                       ValueRef** argv, bool isNewExpression)
{
    fetchWebView(state->context())->avplay()->seekTo(argv[0]->toNumber(state));
    return ValueRef::createUndefined();
}
#endif /* defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY) */

ScriptBindingWindowInstance::ScriptBindingWindowInstance(
    ScriptEngineInstance* engineInstance, Window* ownerWindow)
    : ScriptBindingInstance(engineInstance)
    , m_ownerWindow(ownerWindow)
{
    STARFISH_ASSERT(engineInstance != nullptr && ownerWindow != nullptr);
}

void ScriptBindingWindowInstance::initJavaScriptBinding(
    Escargot::ContextRef* context, Escargot::ExecutionStateRef* state)
{
    STARFISH_ASSERT(context != nullptr && state != nullptr);
    ScriptBindingInstance::initJavaScriptBinding(context, state);

#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
    ObjectRef* avplay = ObjectRef::create(state);

#define DECLARE_AVPLAY_APIS(name)                                           \
    avplay->defineDataProperty(                                             \
        state, ValueRef::create(StringRef::fromASCII(#name)),               \
        ValueRef::create(FunctionObjectRef::createBuiltinFunction(          \
            state, FunctionObjectRef::NativeFunctionInfo(                   \
                       AtomicStringRef::create(context, #name),             \
                       _##name##AvplayFunction, 1, nullptr, true, false))), \
        true, true, true);
    AVPLAY_APIS(DECLARE_AVPLAY_APIS)
#undef DECLARE_AVPLAY_APIS

    ObjectRef* webapis = ObjectRef::create(state);
    globalObject->defineDataProperty(
        state, ValueRef::create(StringRef::fromASCII("webapis")),
        ValueRef::create(webapis), true, true, true);

    webapis->defineDataProperty(
        state, ValueRef::create(StringRef::fromASCII("avplay")),
        ValueRef::create(avplay), true, true, true);
#endif /* defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY) */

    fnEventTarget();
    fnWindow();

    m_ownerWindow->init(this, m_ownerWindow);

    context->setVirtualIdentifierCallback(virtualIdentifierCallback);

#ifdef STARFISH_ENABLE_TEST
    if (ownerWindow()->webView()->testCompatibleMode() ==
        StarfishTestCompatibleMode::Normal) {
        evaluateString(this, String::fromUTF8("delete this.testRunner"));
    }
#endif /* STARFISH_ENABLE_TEST */
}

void ScriptBindingWindowInstance::destroy()
{
    if (m_ownerWindow->browsingContext()->isTopLevelBrowsingContext()) {
        m_scriptContext->vmInstance()->clearCachesRelatedWithContext();
    }

    ScriptBindingInstance::destroy();
}

Window* ScriptBindingWindowInstance::ownerWindow()
{
    return m_ownerWindow;
}

Document* ScriptBindingWindowInstance::ownerDocument()
{
    return m_ownerWindow->document();
}

void ScriptBindingWindowInstance::dispatchErrorEventToGlobalScope(
    ErrorEventInit& errorInfo)
{
    m_ownerWindow->dispatchErrorEvent(errorInfo);
}
}
