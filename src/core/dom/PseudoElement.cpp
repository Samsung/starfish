/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/PseudoElement.h"
#include "core/layout/FrameText.h"
#include "core/page/Window.h"

#define NonBreakingSpace 0x00A0

namespace Starfish {

static inline bool isSpaceForFirstLetter(char32_t c)
{
    return String::isSpaceOrNewline(c) || c == NonBreakingSpace;
}

void* PseudoElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(PseudoElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLElement)] = { 0 };
        PseudoElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

QualifiedName PseudoElement::pseudoElementTagName(Document* document,
                                                  PseudoElementType pseudoId)
{
    Starfish* starfish = document->starfish();
    switch (pseudoId) {
    case PseudoElementType::PseudoElementAfter:
        return QualifiedName(AtomicString(), AtomicString::createAtomicString(
                                                 starfish, "pseudo:after"));
    case PseudoElementType::PseudoElementBefore:
        return QualifiedName(AtomicString(), AtomicString::createAtomicString(
                                                 starfish, "pseudo:before"));
    case PseudoElementType::PseudoElementFirstLetter:
        return QualifiedName(
            AtomicString(),
            AtomicString::createAtomicString(starfish, "pseudo:first-letter"));
    default:
        break;
    }
    return QualifiedName(AtomicString(),
                         AtomicString::createAtomicString(starfish, "pseudo"));
}

size_t FirstLetterPseudoElement::firstLetterLength(String* text)
{
    size_t length = 0;
    size_t textLength = text->length();

    if (textLength == 0) {
        return length;
    }

    // ex. [\s]*[\punctuation]*([!\s])[\punctuation]*
    // Account for leading spaces first.
    while (length < textLength && isSpaceForFirstLetter(text->charAt(length))) {
        length++;
    }

    while (length < textLength && String::isPunctuation(text->charAt(length))) {
        length++;
    }

    // Bail if we didn't find a letter before the end of the text or before a
    // space.
    if (length == textLength || isSpaceForFirstLetter(text->charAt(length))) {
        return 0;
    }

    // Account the next character for first letter.
    length++;

    // Keep looking for allowed punctuation for the :first-letter.
    for (; length < textLength; ++length) {
        char32_t c = text->charAt(length);
        if (!String::isPunctuation(c)) {
            break;
        }
    }

    return length;
}

// Once we see any of these frames we can stop looking for first-letter as
// they signal the end of the first line of text.
static bool isInvalidFirstLetterFrame(Frame* obj)
{
    STARFISH_ASSERT(obj);
    return obj->isFrameLineBreak();
}

Frame* FirstLetterPseudoElement::firstLetterFrameText(Node* n)
{
    Frame* parentFrame = nullptr;

    if (n->isFirstLetterPseudoElement()) {
        parentFrame = n->parentElement()->frame();
    } else {
        parentFrame = n->frame();
    }

    if (!parentFrame || parentFrame->isAnonymous() ||
        !(parentFrame->node()->isElement() &&
          parentFrame->node()->style()->seenPseudoElement(
              PseudoElementType::PseudoElementFirstLetter)) ||
        !parentFrame->canHaveFirstLineOrFirstLetterStyle()) {
        return nullptr;
    }

    Frame* firstLetterFrame = parentFrame->firstChild();

    // TODO: find proper position to insert frame of first-letter pseudo element
    while (firstLetterFrame) {
        if (firstLetterFrame->isAnonymous()) {
            if (Frame* c = firstLetterFrame->firstChild()) {
                if (c->isFrameText() &&
                    c->asFrameText()->text()->containsOnlyWhitespace()) {
                    firstLetterFrame = firstLetterFrame->next();
                    continue;
                }
            }
            firstLetterFrame = firstLetterFrame->firstChild();
        } else if (firstLetterFrame->node()->isCounterPseudoElement()) {
            // Skip counter frames
            firstLetterFrame = firstLetterFrame->next();
        } else if (firstLetterFrame->isFrameText()) {
            String* str = firstLetterFrame->asFrameText()->text();
            if (firstLetterLength(str) ||
                isInvalidFirstLetterFrame(firstLetterFrame)) {
                break;
            }
            firstLetterFrame = firstLetterFrame->next();
        } else if (!firstLetterFrame->isNormalFlow()) { // float or out-of-flow
            if (firstLetterFrame->node()->isElement() &&
                firstLetterFrame->node()->style()->seenPseudoElement(
                    PseudoElementType::PseudoElementFirstLetter)) {
                firstLetterFrame = firstLetterFrame->firstChild();
                break;
            }
            firstLetterFrame = firstLetterFrame->next();
        } else if (firstLetterFrame->isAtomicInlineLevel()) {
            return nullptr;
        } else {
            firstLetterFrame = firstLetterFrame->firstChild();
        }
    }

    if (!firstLetterFrame || !firstLetterFrame->asFrameText()->text() ||
        isInvalidFirstLetterFrame(firstLetterFrame)) {
        return nullptr;
    }

    return firstLetterFrame;
}
} // namespace Starfish
