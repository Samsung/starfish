/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "core/dom/Document.h"
#include "core/dom/MessagePort.h"
#include "core/dom/MessageChannel.h"

namespace StarFish {

MessageChannel::MessageChannel(Document* document)
    : ScriptWrappable(this)
    , m_scriptBindingInstance(document->scriptBindingInstance())
    , m_port1(new MessagePort(document))
    , m_port2(new MessagePort(document))
{
    MessagePort::entangle(m_port1, m_port2);
}

MessagePort* MessageChannel::port1()
{
    return m_port1;
}

MessagePort* MessageChannel::port2()
{
    return m_port2;
}

ScriptBindingInstance* MessageChannel::scriptBindingInstance()
{
    return m_scriptBindingInstance;
}
}
