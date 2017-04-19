/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishScriptBindingInstanceDataEscargot__
#define __StarFishScriptBindingInstanceDataEscargot__

#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptWrappable.h"

#include <Escargot.h>

namespace StarFish {

using namespace escargot;

#define FOR_EACH_DECLARE_FN(exportName)    \
    ESFunctionObject* binding##exportName( \
        ScriptBindingInstance* scriptBindingInstance);

STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_DECLARE_FN);
#undef FOR_EACH_DECLARE_FN

class ScriptBindingInstanceDataEscargot : public gc {
    friend void ScriptBindingInstance::initBinding(StarFish* sf);

public:
    ScriptBindingInstance* m_bindingInstance;
    ESVMInstance* m_instance;
    ESFunctionObject* m_orgToString;

    ScriptBindingInstanceDataEscargot(ScriptBindingInstance* bindingInstance)
    {
        memset(this, 0, sizeof(ScriptBindingInstanceDataEscargot));
        m_bindingInstance = bindingInstance;
    }

#define FOR_EACH_GETTER_FN(exportName)                                 \
    ESFunctionObject* fn##exportName()                                 \
    {                                                                  \
        if (UNLIKELY(m_fn##exportName == nullptr)) {                   \
            m_fn##exportName = binding##exportName(m_bindingInstance); \
            m_value##exportName = m_fn##exportName;                    \
        }                                                              \
        return m_fn##exportName;                                       \
    }

    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_GETTER_FN)
#undef FOR_EACH_GETTER_FN

#define FOR_EACH_GETTER_VALUE_FN(exportName)                           \
    ESValue value##exportName()                                        \
    {                                                                  \
        if (UNLIKELY(m_fn##exportName == nullptr)) {                   \
            m_fn##exportName = binding##exportName(m_bindingInstance); \
            m_value##exportName = m_fn##exportName;                    \
        }                                                              \
        return m_value##exportName;                                    \
    }

    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_GETTER_VALUE_FN)
#undef FOR_EACH_GETTER_VALUE_FN

#define FOR_EACH_SCRIPT_FN(exportName) ESFunctionObject* m_fn##exportName;
    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_SCRIPT_FN)
#undef FOR_EACH_SCRIPT_FN

public:
#define FOR_EACH_SCRIPTVALUE_FN(exportName) ESValue m_value##exportName;
    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_SCRIPTVALUE_FN)
#undef FOR_EACH_SCRIPTVALUE_FN
};
}

#endif
