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
#include "PseudoElement.h"
#include "layout/FrameText.h"

#define NonBreakingSpace 0x00A0

namespace StarFish {

static inline bool isSpaceForFirstLetter(char32_t c)
{
    return String::isSpaceOrNewline(c) || c == NonBreakingSpace;
}

CharCategory category(char32_t c)
{
    return static_cast<CharCategory>(U_GET_GC_MASK(c));
}

static inline bool isPunctuationForFirstLetter(char32_t c)
{
    CharCategory charCategory = category(c);
    return charCategory == Punctuation_Open ||
           charCategory == Punctuation_Close ||
           charCategory == Punctuation_InitialQuote ||
           charCategory == Punctuation_FinalQuote ||
           charCategory == Punctuation_Other;
}

QualifiedName PseudoElement::pseudoElementTagName(
    StyleResolver::PseudoElementType pseudoId)
{
    STARFISH_ASSERT(document());

    StarFish* sf = document()->window()->starFish();
    switch (pseudoId) {
    case StyleResolver::PseudoElementType::PseudoElementAfter:
        return QualifiedName(AtomicString(), AtomicString::createAtomicString(
                                                 sf, "pseudo:after"));
    case StyleResolver::PseudoElementType::PseudoElementBefore:
        return QualifiedName(AtomicString(), AtomicString::createAtomicString(
                                                 sf, "pseudo:before"));
    case StyleResolver::PseudoElementType::PseudoElementFirstLetter:
        return QualifiedName(AtomicString(), AtomicString::createAtomicString(
                                                 sf, "pseudo:first-letter"));
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        break;
    }
    return QualifiedName(AtomicString(),
                         AtomicString::createAtomicString(sf, "pseudo"));
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

    while (length < textLength &&
           isPunctuationForFirstLetter(text->charAt(length))) {
        length++;
    }

    // Bail if we didn't find a letter before the end of the text or before a
    // space.
    if (isSpaceForFirstLetter(text->charAt(length)) || length == textLength) {
        return 0;
    }

    // Account the next character for first letter.
    length++;

    // Keep looking for allowed punctuation for the :first-letter.
    for (; length < textLength; ++length) {
        char32_t c = text->charAt(length);
        if (!isPunctuationForFirstLetter(c)) {
            break;
        }
    }

    return length;
}

// Once we see any of these frames we can stop looking for first-letter as
// they signal the end of the first line of text.
static bool isInvalidFirstLetterLayoutObject(Frame* obj)
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

    if (parentFrame->isAnonymous())
        return nullptr;

    if (!(parentFrame->node()->isElement() &&
          parentFrame->node()->asElement()->hasPseudoElement(
              StyleResolver::PseudoElementType::PseudoElementFirstLetter))) {
        return nullptr;
    }

    Frame* firstLetterFrameText = parentFrame->firstChild();

    // TODO: find proper position to insert frame of first-letter pseudo element
    while (firstLetterFrameText) {
        if (firstLetterFrameText->node()->isElement() &&
            firstLetterFrameText->node()->asElement()->hasPseudoElement(
                StyleResolver::PseudoElementType::PseudoElementFirstLetter)) {
            firstLetterFrameText = firstLetterFrameText->next();
        } else if (firstLetterFrameText->isFrameText()) {
            String* str = firstLetterFrameText->asFrameText()->text();
            if (firstLetterLength(str) ||
                isInvalidFirstLetterLayoutObject(firstLetterFrameText)) {
                break;
            }
            firstLetterFrameText = firstLetterFrameText->next();
        } else if (firstLetterFrameText->isFrameReplaced()) {
            return nullptr;
        } else {
            firstLetterFrameText = firstLetterFrameText->firstChild();
        }
    }

    if (!firstLetterFrameText || !firstLetterFrameText->asFrameText()->text() ||
        isInvalidFirstLetterLayoutObject(firstLetterFrameText)) {
        return nullptr;
    }

    return firstLetterFrameText;
}
}
