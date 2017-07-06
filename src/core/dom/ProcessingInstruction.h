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

#ifndef __StarFishProcessingInstruction__
#define __StarFishProcessingInstruction__

#include "core/dom/CharacterData.h"

namespace StarFish {

class ProcessingInstruction : public CharacterData {
public:
    ProcessingInstruction(Document* document, String* data, String* target)
        : CharacterData(document, data)
        , m_target(target)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isProcessingInstruction() const override;

    /* 4.4 Interface Node */
    virtual NodeType nodeType() const override
    {
        return Node::PROCESSING_INSTRUCTION_NODE;
    }

    virtual String* nodeName();

    virtual Node* clone() override
    {
        return new ProcessingInstruction(document(), data(), m_target);
    }

    String* target()
    {
        return m_target;
    }

protected:
    String* m_target;
};
}

#endif
