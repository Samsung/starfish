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

#include "StarFishConfig.h"
#include "core/dom/Document.h"
#include "core/style/StyleSheet.h"
#include "core/style/StyleSheetList.h"

namespace StarFish {

ScriptBindingInstance* StyleSheetList::scriptBindingInstance()
{
    return m_document->scriptBindingInstance();
}

StyleSheet* StyleSheetList::item(unsigned long index)
{
    // The first sheet is a UA sheet.
    return index < length()
               ? (StyleSheet*)m_document->styleResolver().sheets()[index + 1]
               : nullptr;
}

size_t StyleSheetList::length() const
{
    // The first sheet is a UA sheet.
    STARFISH_ASSERT(m_document->styleResolver().sheets().size());
    return m_document->styleResolver().sheets().size() - 1;
}
}
