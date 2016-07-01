/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#ifndef __StarFishMarkupTokenizerInlines__
#define __StarFishMarkupTokenizerInlines__

#include "util/String.h"

namespace StarFish {

inline bool isTokenizerWhitespace(char32_t cc)
{
    return cc == ' ' || cc == '\x0A' || cc == '\x09' || cc == '\x0C';
}

inline void advanceStringAndASSERTIgnoringCase(SegmentedString& source, const char* expectedCharacters)
{
    while (*expectedCharacters)
        source.advanceAndASSERTIgnoringCase(*expectedCharacters++);
}

inline void advanceStringAndASSERT(SegmentedString& source, const char* expectedCharacters)
{
    while (*expectedCharacters)
        source.advanceAndASSERT(*expectedCharacters++);
}

#if COMPILER(MSVC)
// We need to disable the "unreachable code" warning because we want to assert
// that some code points aren't reached in the state machine.
#pragma warning(disable: 4702)
#endif

#define BEGIN_STATE(prefix, stateName) case prefix::stateName: stateName:
#define END_STATE() STARFISH_ASSERT_NOT_REACHED(); break;

// We use this macro when the HTML5 spec says "reconsume the current input
// character in the <mumble> state."
#define RECONSUME_IN(prefix, stateName)                                    \
    do {                                                                   \
        m_state = prefix::stateName;                                       \
        goto stateName;                                                    \
    } while (false)

// We use this macro when the HTML5 spec says "consume the next input
// character ... and switch to the <mumble> state."
#define ADVANCE_TO(prefix, stateName)                                      \
    do {                                                                   \
        m_state = prefix::stateName;                                       \
        if (!m_inputStreamPreprocessor.advance(source))                    \
            return haveBufferedCharacterToken();                           \
        cc = m_inputStreamPreprocessor.nextInputCharacter();               \
        goto stateName;                                                    \
    } while (false)

// Sometimes there's more complicated logic in the spec that separates when
// we consume the next input character and when we switch to a particular
// state. We handle those cases by advancing the source directly and using
// this macro to switch to the indicated state.
#define SWITCH_TO(prefix, stateName)                                       \
    do {                                                                   \
        m_state = prefix::stateName;                                       \
        if (source.isEmpty() || !m_inputStreamPreprocessor.peek(source))   \
            return haveBufferedCharacterToken();                           \
        cc = m_inputStreamPreprocessor.nextInputCharacter();               \
        goto stateName;                                                    \
    } while (false)

}

#endif
