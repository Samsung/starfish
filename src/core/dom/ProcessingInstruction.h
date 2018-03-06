/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
