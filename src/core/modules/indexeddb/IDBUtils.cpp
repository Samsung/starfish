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
#include "core/modules/indexeddb/IDBTransaction.h"
#include "core/modules/indexeddb/IDBUtils.h"

namespace Starfish {

String* IDBUtils::transactionDurabilityToString(
    IDBTransactionDurability durability)
{
    if (durability == IDBTransactionDurability::Default) {
        return String::createASCIIString("default");
    } else if (durability == IDBTransactionDurability::Strict) {
        return String::createASCIIString("strict");
    } else if (durability == IDBTransactionDurability::Relaxed) {
        return String::createASCIIString("relaxed");
    }

    STARFISH_ASSERT_NOT_REACHED();
    return String::emptyString;
}

IDBTransactionDurability IDBUtils::transactionDurabilityToType(String* string)
{
    if (string->equals("default")) {
        return IDBTransactionDurability::Default;
    } else if (string->equals("strict")) {
        return IDBTransactionDurability::Strict;
    } else if (string->equals("relaxed")) {
        return IDBTransactionDurability::Relaxed;
    }

    STARFISH_ASSERT_NOT_REACHED();
    return IDBTransactionDurability::Default;
}

String* IDBUtils::transactionModeToString(IDBTransactionMode mode)
{
    if (mode == IDBTransactionMode::ReadOnly) {
        return String::createASCIIString("readonly");
    } else if (mode == IDBTransactionMode::ReadWrite) {
        return String::createASCIIString("readwrite");
    } else if (mode == IDBTransactionMode::VersionChange) {
        return String::createASCIIString("versionchange");
    }

    STARFISH_ASSERT_NOT_REACHED();
    return String::emptyString;
}

} // namespace Starfish

#endif
