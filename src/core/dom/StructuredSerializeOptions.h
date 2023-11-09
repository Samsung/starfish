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
#ifndef __StarfishStructuredSerializeOptions__
#define __StarfishStructuredSerializeOptions__

#include "binding/ScriptWrappable.h"

namespace Starfish {

struct StructuredSerializeOptions : public gc {
    StructuredSerializeOptions() = default;
    StructuredSerializeOptions(GCAtomicVector<ScriptObject>& transfer)
        : m_transfer(transfer)
    {
    }

    const GCAtomicVector<ScriptObject>& transfer() const
    {
        return m_transfer;
    }

    void setTransfer(GCAtomicVector<ScriptObject>& transfer)
    {
        m_transfer = transfer;
    }

    GCAtomicVector<ScriptObject> m_transfer;
};

} // namespace Starfish

#endif
