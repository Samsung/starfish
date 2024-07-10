/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_IDB)

#include "StarfishConfig.h"
#include "core/dom/WebOrigin.h"
#include "core/modules/indexeddb/IDBDatabaseIdentifier.h"

namespace Starfish {

IDBDatabaseIdentifier::IDBDatabaseIdentifier()
    : hash(0)
{
}

IDBDatabaseIdentifier::IDBDatabaseIdentifier(WebOrigin* origin, String* name)
    : hash(0)
{
    hash_combine(hash, origin->serialize()->hashValue());
    hash_combine(hash, name->hashValue());
}

bool IDBDatabaseIdentifier::operator==(const IDBDatabaseIdentifier& other) const
{
    return hash == other.hash;
}

} // namespace Starfish

#endif
