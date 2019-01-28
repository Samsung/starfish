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

#include <EscargotPublic.h>

namespace Starfish {

ScriptEngineInstance::ScriptEngineInstance(const char* locale, const char* timezone, PromiseJobListener listener)
{
    // Set this flag to process const keyword temporary
    setenv("ESCARGOT_TREAT_CONST_AS_VAR", "1", 1);

    Escargot::Globals::initialize();

    m_engineInstance = Escargot::VMInstanceRef::create(locale, timezone);
    m_engineInstance->setNewPromiseJobListener(listener);
}

void ScriptEngineInstance::dispose()
{
    m_engineInstance->destroy();
}
}
