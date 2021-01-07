/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishMessageChannel__
#define __StarfishMessageChannel__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class MessagePort;

class MessageChannel : public ScriptWrappable {
public:
    MessageChannel(ExecutionContext* executionContext);
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;
    virtual bool isMessageChannel() const override;

    MessagePort* port1();
    MessagePort* port2();

protected:
    ScriptBindingInstance* m_scriptBindingInstance;
    MessagePort* m_port1;
    MessagePort* m_port2;
};
} // namespace Starfish

#endif
