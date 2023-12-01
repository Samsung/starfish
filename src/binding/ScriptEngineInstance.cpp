/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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
#include "binding/ScriptEngineInstance.h"
#include "binding/ScriptBindingInstance.h"


#include "core/modules/message_loop/MessageLoop.h"

#ifdef STARFISH_TIZEN_PROD_TV
#include <app_common.h>
#endif
#include <EscargotPublic.h>

namespace Starfish {

ScriptEngineInstance::ScriptEngineInstance(const char* locale,
                                           const char* timezone)
{
#ifdef STARFISH_TIZEN_PROD_TV
    // add argument for CodeCache directory
    m_engineInstance = Escargot::VMInstanceRef::create(
        locale, timezone, app_get_data_path());
#else
    m_engineInstance = Escargot::VMInstanceRef::create(
        locale, timezone);
#endif
    if (m_engineInstance->isCodeCacheEnabled()) {
        m_engineInstance->setMaxCompiledByteCodeSize(1024 * 1024 * 8);
        m_engineInstance->setCodeCacheMinSourceLength(1024 * 2);
        m_engineInstance->setCodeCacheMaxCacheCount(16);
        m_engineInstance->setCodeCacheShouldLoadFunctionOnScriptLoading(true);
    } else {
        m_engineInstance->setMaxCompiledByteCodeSize(1024 * 1024 * 4);
    }
}

void ScriptEngineInstance::dispose()
{
    m_engineInstance = nullptr;
}
} // namespace Starfish
