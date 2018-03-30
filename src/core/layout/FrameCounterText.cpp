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
#include "core/layout/FrameCounterText.h"
#include "core/style/ContentData.h"
#include "core/style/CounterStyle.h"
#include "core/style/ContentData.h"

namespace StarFish {

FrameCounterText::FrameCounterText(Node* node, CounterType type)
    : FrameText(node, nullptr)
    , m_type(type)
{
    m_node = (Node*)new FrameTextRareData(m_node);
}

static CounterContentData* pseudoCounterContentData(ComputedStyle* style)
{
    ContentDataGroup* content = style->content();
    STARFISH_ASSERT(content->size() == 1);
    STARFISH_ASSERT(content->back().isCounter());
    return content->back().counter();
}

const AtomicString& FrameCounterText::counterId() const
{
    STARFISH_ASSERT(m_type == CounterTypePseudoContent);
    return pseudoCounterContentData(node()->style())->id();
}

const CounterContentData* FrameCounterText::counterContentData() const
{
    STARFISH_ASSERT(m_type == CounterTypePseudoContent);
    return pseudoCounterContentData(node()->style());
}

const CounterStyle* FrameCounterText::counterStyle() const
{
    if (m_type == CounterTypePseudoContent) {
        return counterContentData()->counterStyle();
    }
    STARFISH_ASSERT(isListType());
    STARFISH_ASSERT(node()->style()->listStyleData().typeData());
    STARFISH_ASSERT(node()->style()->listStyleData().typeData()->system() !=
                    CounterStyle::NoneSystem);
    return node()->style()->listStyleData().typeData();
}

static String* generateCounterText(const CounterContentData* counterData,
                                   std::vector<int32_t>& indice)
{
    STARFISH_ASSERT(counterData);
    STARFISH_ASSERT(indice.size());

    const CounterStyle* counter = counterData->counterStyle();
    Nullable<String*> sp = counterData->separator();
    if (sp.hasValue()) {
        StringBuilder sb;
        size_t indiceSize = indice.size();
        for (size_t i = 0; i < indiceSize; i++) {
            sb.appendString(
                counter->generateLabelForCSSContentProperty(indice[i]));
            if (i + 1 != indiceSize) {
                sb.appendString(sp.getValue());
            }
        }
        return sb.finalize();
    }
    return counter->generateLabelForCSSContentProperty(indice.back());
}

void FrameCounterText::updateCounterText(int32_t index)
{
    if (m_type == CounterTypePseudoContent) {
        setText(counterStyle()->generateLabelForCSSContentProperty(index));
    } else {
        setText(counterStyle()->generateLabel(index));
    }
}

void FrameCounterText::updateCounterText(std::vector<int32_t>& indice)
{
    if (m_type == CounterTypePseudoContent) {
        setText(generateCounterText(counterContentData(), indice));
    } else {
        setText(counterStyle()->generateLabel(indice.back()));
    }
}
} /* namespace StarFish */
