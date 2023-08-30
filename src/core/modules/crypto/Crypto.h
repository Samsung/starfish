/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishCrypto__
#define __StarfishCrypto__

#include "binding/ScriptWrappable.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

class Crypto : public ScriptWrappable {
public:
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCrypto() const override;
    ScriptBindingInstance* scriptBindingInstance() override
    {
        return m_executionContext->scriptBindingInstance();
    }
    ScriptArrayBufferView getRandomValues(ScriptArrayBufferView array);

    static Crypto* create(ExecutionContext* executionContext)
    {
        return new Crypto(executionContext);
    }
    ExecutionContext* executionContext() const
    {
        return m_executionContext;
    }

private:
    Crypto(ExecutionContext* executionContext);
    void makeRandomByte(uint8_t* buffer, unsigned byteSize);
    bool isIntTypeArray(ScriptArrayBufferView array);

    ExecutionContext* m_executionContext;
};
} // namespace Starfish

#endif
