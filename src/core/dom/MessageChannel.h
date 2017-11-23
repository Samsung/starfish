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

#ifndef __StarFishMessageChannel__
#define __StarFishMessageChannel__

#include "binding/ScriptWrappable.h"

namespace StarFish {

class MessagePort;

class MessageChannel : public ScriptWrappable {
public:
    MessageChannel(Document* document);
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
}

#endif
