/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2006, 2007 Nicholas Shanks (webkit@nickshanks.com)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights
 * reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 * Copyright (C) 2007, 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved.
 * (http://www.torchmobile.com/)
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "core/dom/Element.h"
#include "AncestorSelectorFilter.h"
#include "core/style/Style.h"
#include "core/style/StyleRule.h"

namespace StarFish {

enum { TagNameSalt = 13, IdSalt = 17, ClassNameSalt = 19 };

static void collectElementIdentifierHashes(
    Element* element, std::vector<unsigned>& identifierHashes)
{
    identifierHashes.push_back(
        element->name().localNameAtomic().string()->hashValue() * TagNameSalt);
    if (element->hasId()) {
        identifierHashes.push_back(element->atomicId().string()->hashValue() *
                                   IdSalt);
    }
    const auto& classes = element->classNames();
    size_t classNameSize = classes.size();
    for (size_t i = 0; i < classNameSize; i++) {
        identifierHashes.push_back(classes[i].string()->hashValue() *
                                   ClassNameSalt);
    }
}

void AncestorSelectorFilter::pushElement(Element* e)
{
    AncestorStackFrame frame(e);
    collectElementIdentifierHashes(e, frame.m_identifierHashes);
    for (size_t i = 0; i < frame.m_identifierHashes.size(); i++) {
        m_bloomFilter.add(frame.m_identifierHashes[i]);
    }
    m_parentStack.push_back(std::move(frame));
}

void AncestorSelectorFilter::popElement()
{
    AncestorStackFrame& frame = m_parentStack.back();
    for (size_t i = 0; i < frame.m_identifierHashes.size(); i++) {
        m_bloomFilter.remove(frame.m_identifierHashes[i]);
    }
    m_parentStack.pop_back();
}

static inline void collectDescendantSelectorIdentifierHashes(
    CSSSelector* selector, unsigned*& hash)
{
    switch (selector->type()) {
    case CSSSelector::Id:
        (*hash++) = selector->selectorText().string()->hashValue() * IdSalt;
        break;
    case CSSSelector::Class:
        (*hash++) =
            selector->selectorText().string()->hashValue() * ClassNameSalt;
        break;
    case CSSSelector::Tag:
        (*hash++) =
            selector->selectorText().string()->hashValue() * TagNameSalt;
        break;
    default:
        break;
    }
}

void AncestorSelectorFilter::computeIdentifierHash(StyleRule* rule)
{
    const unsigned maximumIdentifierCount = 10;

    unsigned* identifierHashes = rule->m_identifierHashes;

    unsigned* hash = identifierHashes;
    unsigned* end = identifierHashes + StyleRule::maximumIdentifierCount;
    auto& selectorList = rule->selectorList();
    size_t selectorListSize = selectorList.size();
    CSSSelector* selector = selectorList[0];
    CSSSelector::RelationType relation = selector->relation();
    bool relationIsAffectedByPseudoContent =
        selector->relationIsAffectedByPseudoContent();

    // Skip the topmost selector. It is handled quickly by the rule hashes.
    bool skipOverSubselectors = true;
    for (size_t i = 1; i < selectorListSize; i++) {
        selector = selectorList[i];

        // Only collect identifiers that match ancestors.
        switch (relation) {
        case CSSSelector::SubSelector:
            if (!skipOverSubselectors)
                collectDescendantSelectorIdentifierHashes(selector, hash);
            break;
        case CSSSelector::AdjacentSibling:
        case CSSSelector::GeneralSibling:
            skipOverSubselectors = true;
            break;
        case CSSSelector::Descendant:
        case CSSSelector::Child:
            if (relationIsAffectedByPseudoContent) {
                // Disable fastRejectSelector.
                *identifierHashes = 0;
                break;
            }
            skipOverSubselectors = false;
            collectDescendantSelectorIdentifierHashes(selector, hash);
            break;
        default:
            STARFISH_ASSERT_NOT_REACHED();
        }
        if (hash == end)
            break;
        relation = selector->relation();
        relationIsAffectedByPseudoContent =
            selector->relationIsAffectedByPseudoContent();
    }
}
}
