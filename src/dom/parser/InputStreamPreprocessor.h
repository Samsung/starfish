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

#ifndef __StarFishInputStreamPreprocessor__
#define __StarFishInputStreamPreprocessor__

#include "util/String.h"

namespace StarFish {

const char32_t kEndOfFileMarker = 0;

// http://www.whatwg.org/specs/web-apps/current-work/#preprocessing-the-input-stream
template <typename Tokenizer>
class InputStreamPreprocessor : public gc {
public:
    InputStreamPreprocessor(Tokenizer* tokenizer)
        : m_tokenizer(tokenizer)
    {
        reset();
    }

    ALWAYS_INLINE char32_t nextInputCharacter() const { return m_nextInputCharacter; }

    // Returns whether we succeeded in peeking at the next character.
    // The only way we can fail to peek is if there are no more
    // characters in |source| (after collapsing \r\n, etc).
    ALWAYS_INLINE bool peek(SegmentedString& source)
    {
        m_nextInputCharacter = source.currentChar();

        // Every branch in this function is expensive, so we have a
        // fast-reject branch for characters that don't require special
        // handling. Please run the parser benchmark whenever you touch
        // this function. It's very hot.
        static const UChar specialCharacterMask = '\n' | '\r' | '\0';
        if (m_nextInputCharacter & ~specialCharacterMask) {
            m_skipNextNewLine = false;
            return true;
        }
        return processNextInputCharacter(source);
    }

    // Returns whether there are more characters in |source| after advancing.
    ALWAYS_INLINE bool advance(SegmentedString& source)
    {
        source.advanceAndUpdateLineNumber();
        if (source.isEmpty())
            return false;
        return peek(source);
    }

    bool skipNextNewLine() const { return m_skipNextNewLine; }

    void reset(bool skipNextNewLine = false)
    {
        m_nextInputCharacter = '\0';
        m_skipNextNewLine = skipNextNewLine;
    }

private:
    bool processNextInputCharacter(SegmentedString& source)
    {
    ProcessAgain:
        STARFISH_ASSERT(m_nextInputCharacter == source.currentChar());

        if (m_nextInputCharacter == '\n' && m_skipNextNewLine) {
            m_skipNextNewLine = false;
            source.advancePastNewlineAndUpdateLineNumber();
            if (source.isEmpty())
                return false;
            m_nextInputCharacter = source.currentChar();
        }
        if (m_nextInputCharacter == '\r') {
            m_nextInputCharacter = '\n';
            m_skipNextNewLine = true;
        } else {
            m_skipNextNewLine = false;
            // FIXME: The spec indicates that the surrogate pair range as well as
            // a number of specific character values are parse errors and should be replaced
            // by the replacement character. We suspect this is a problem with the spec as doing
            // that filtering breaks surrogate pair handling and causes us not to match Minefield.
            if (m_nextInputCharacter == '\0' && !shouldTreatNullAsEndOfFileMarker(source)) {
                if (m_tokenizer->shouldSkipNullCharacters()) {
                    source.advancePastNonNewline();
                    if (source.isEmpty())
                        return false;
                    m_nextInputCharacter = source.currentChar();
                    goto ProcessAgain;
                }
                m_nextInputCharacter = 0xFFFD;
            }
        }
        return true;
    }

    bool shouldTreatNullAsEndOfFileMarker(SegmentedString& source) const
    {
        return source.isClosed() && source.length() == 1;
    }

    Tokenizer* m_tokenizer;

    // http://www.whatwg.org/specs/web-apps/current-work/#next-input-character
    char32_t m_nextInputCharacter;
    bool m_skipNextNewLine;
};

}

#endif
