/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishFetchBody__
#define __StarFishFetchBody__

#include "binding/ScriptWrappable.h"
#include "core/fetch/GetSet.h"
#include "binding/BlobOrBufferSourceOrUSVStringUnion.h"

namespace StarFish {

typedef BlobOrBufferSourceOrUSVString BodyInit;

class Body : public gc {
public:
    Promise* arrayBuffer();
    Promise* blob();
    Promise* json();
    Promise* text();

    bool bodyUsed() const
    {
        return m_bodyUsed;
    };

    Nullable<BodyInit> body() const
    {
        return m_body;
    }

protected:
    Body(ScriptBindingInstance* instance)
        : m_scriptBindingInstance(instance)
    {
    }

    bool m_bodyUsed;
    Nullable<BodyInit> m_body;
    ScriptBindingInstance* m_scriptBindingInstance;
};
}

#endif
