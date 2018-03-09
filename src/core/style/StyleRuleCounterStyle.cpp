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

#include "StarFishConfig.h"
#include "core/modules/threading/Thread.h"
#include "core/style/StyleRuleCounterStyle.h"

namespace StarFish {

const StyleRuleCounterStyle* StyleRuleCounterStyle::getDiscCounter()
{
    STARFISH_ASSERT(isMainThread());
    static StyleRuleCounterStyle* counter;
    if (counter == nullptr) {
        counter = new (NoGC)
            StyleRuleCounterStyle(String::createASCIIString("disc"),
                                  StyleRuleCounterStyle::CyclicSystem);
        counter->setSuffix(String::spaceString);
        counter->addSymbol(String::createUTF32String(0x2022));
    }
    return counter;
}

bool StyleRuleCounterStyle::equals(const StyleRuleCounterStyle* b) const
{
    if (this == b) {
        return true;
    }
    if (!b) {
        return false;
    }
    if (!m_name->equals(b->m_name)) {
        return false;
    }
    if (m_system != b->m_system) {
        return false;
    }
    if (m_symbols.size() != b->m_symbols.size()) {
        return false;
    }
    if (m_additiveWeight.size() != b->m_additiveWeight.size()) {
        return false;
    }
    for (size_t i = 0; i < m_symbols.size(); i++) {
        if (!m_symbols[i]->equals(b->m_symbols[i])) {
            return false;
        }
    }
    for (size_t i = 0; i < m_additiveWeight.size(); i++) {
        if (m_additiveWeight[i] != b->m_additiveWeight[i]) {
            return false;
        }
    }
    if (!m_prefix->equals(b->m_prefix)) {
        return false;
    }
    if (!m_suffix->equals(b->m_suffix)) {
        return false;
    }
    if (!m_negativePrefix->equals(b->m_negativePrefix)) {
        return false;
    }
    if (!m_negativeSuffix->equals(b->m_negativeSuffix)) {
        return false;
    }
    if (m_lowerBound != b->m_lowerBound) {
        return false;
    }
    if (m_upperBound != b->m_upperBound) {
        return false;
    }
    if (m_padFixedWidth != b->m_padFixedWidth) {
        return false;
    }
    if (!m_padSymbol->equals(b->m_padSymbol)) {
        return false;
    }
    if (m_system == ExtendsSystem) {
        STARFISH_ASSERT(m_systemExtra.m_extends);
        STARFISH_ASSERT(b->m_systemExtra.m_extends);
        if (!m_systemExtra.m_extends->equals(b->m_systemExtra.m_extends)) {
            return false;
        }
    }
    if (m_system == FixedSystem &&
        m_systemExtra.m_firstSymbolValue !=
            b->m_systemExtra.m_firstSymbolValue) {
        return false;
    }
    if (m_fallback) {
        return m_fallback->equals(b->m_fallback);
    }
    return !b->m_fallback;
}
} /* namespace StarFish */
