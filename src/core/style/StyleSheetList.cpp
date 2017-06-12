/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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
