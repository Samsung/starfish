/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "binding/ScriptBindingInstance.h"
#include "core/dom/HTMLAllCollection.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/Element.h"

#include <EscargotPublic.h>

using namespace Escargot;

namespace Starfish {

static ValueRef* callCollection(ExecutionStateRef* state, ValueRef*,
                                size_t argc, ValueRef** argv, bool)
{
    if (!argc || argv[0]->isUndefined()) {
        return ValueRef::createNull();
    }
    auto* collection = static_cast<HTMLAllCollection*>(
        state->resolveCallee().value()->extraData());
    auto result = collection->item(toBrowserString(state, argv[0]));
    if (!result) {
        return ValueRef::createNull();
    }
    const auto& value = result.value();
    return value.isElementValue()
               ? value.getElementValue()->scriptValue()
               : value.getHTMLCollectionValue()->scriptValue();
}

void HTMLAllCollection::postInit(ScriptBindingInstance* instance)
{
    Evaluator::execute(
        instance->scriptContext(),
        [](ExecutionStateRef* state, HTMLAllCollection* self) -> ValueRef* {
            auto* caller = FunctionObjectRef::create(
                state, FunctionObjectRef::NativeFunctionInfo(
                           AtomicStringRef::emptyAtomicString(), callCollection,
                           0, true, false));
            caller->deleteOwnProperty(state,
                                      StringRef::createFromASCII("length"));
            caller->deleteOwnProperty(state,
                                      StringRef::createFromASCII("name"));
            // Keep the generated indexed and named getters live on the
            // callable. The collection inherits its IDL methods through them.
            caller->setPrototype(state, self->m_object);
            caller->setExtraData(self);
            caller->setIsHTMLDDA();
            self->overrideScriptObject(caller);
            return caller;
        },
        this);
}
} // namespace Starfish
