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

#ifndef __StarFishFrameBlockBoxInlineLayout__
#define __StarFishFrameBlockBoxInlineLayout__

#include "FrameText.h"

namespace StarFish {

struct TextToken {
    FrameText* m_frameText;
    size_t m_start;
    size_t m_end;
    LayoutUnit m_width;
    WordType m_type;

    TextToken(FrameText* ft, size_t start, size_t end, WordType type)
        : m_frameText(ft), m_start(start), m_end(end), m_type(type)
    {
        switch (m_type) {
        case CollapsibleWhiteSpace:
            m_width = m_frameText->style()->font()->spaceWidth();
            break;
        case NonCollapsibleWhiteSpace:
            m_width =
                m_frameText->style()->font()->spaceWidth() * (m_end - m_start);
            break;
        case ForcedNewline:
            m_width = 0;
            break;
        case General:
            m_width = m_frameText->style()->font()->measureText(
                StringView(m_frameText->text(), m_start, m_end));
            break;
        }
    }

    bool isWhiteSpace()
    {
        return m_type != General;
    }

    LayoutUnit width()
    {
        return m_width;
    }

#ifndef NDEBUG
    void dump()
    {
        std::string str = m_frameText->text()
                              ->substring(m_start, m_end - m_start)
                              ->utf8Data();
        str = FrameText::replaceAll(str, "\n", "\\n");
        printf("%s (", str.data());
        printf("width:%d, ", width().toInt());
        printf("type:%s)\n", m_type == General ? "GN" : m_type == ForcedNewline
                                                            ? "NL"
                                                            : "WS");
    }
#endif
};

inline bool isSeparator(char32_t c)
{
    // Fixed-width spaces (such as U+3000 and U+2000 through U+200A)
    // are whitespace but are not considered word-separator characters
    // (https://drafts.csswg.org/css-text-3)
    if (c == 0x3000 || (c >= 0x2000 && c <= 0x200A)) {
        return false;
    }
    return String::isSpaceOrNewline(c);
}

template <typename Context>
void tokenizeText(StarFish* sf, FrameText* f, Context* ctx)
{
    // TODO : Consider direction
    String* txt = f->text();
    size_t len = txt->length();

    bool collapseSpace = !f->shouldPreserveWhiteSpaces();
    bool collapseNewline = f->shouldIgnoreNewlineChar();

    unsigned offset = 0;
    while (offset < len) {
        if (!collapseNewline && String::isNewline(txt->charAt(offset))) {
            TextToken token =
                TextToken(f, offset, offset + 1, WordType::ForcedNewline);
            offset++;
            ctx->handleTextToken(token);
            continue;
        }
        bool isWhiteSpace = false;
        if (isSeparator(txt->charAt(offset))) {
            isWhiteSpace = true;
        }

        // find next space
        unsigned nextOffset = offset + 1;
        if (isWhiteSpace) {
            while (nextOffset < txt->length() &&
                   isSeparator((*txt)[nextOffset])) {
                if (!collapseNewline && String::isNewline((*txt)[nextOffset])) {
                    break;
                }
                nextOffset++;
            }

            // Mostly white-spaces in text are collaped.
            // But the text in <pre> or depending on CSS white-space property,
            // user agent should preserve white-spaces in text.
            WordType type = WordType::CollapsibleWhiteSpace;
            if (!collapseSpace) {
                type = WordType::NonCollapsibleWhiteSpace;
            }
            TextToken token = TextToken(f, offset, nextOffset, type);
            ctx->handleTextToken(token);
        } else {
            size_t start = offset;
            while (nextOffset < txt->length() &&
                   !isSeparator((*txt)[nextOffset])) {
                nextOffset++;
            }

            auto breaker = sf->lineBreaker();
            breaker->setText(txt->toUnicodeString(start, nextOffset));
            int32_t c, prev = 0;
            size_t txtLen = txt->length();
            while (((c = breaker->next()) != icu::BreakIterator::DONE) &&
                   (c + start <= txtLen)) {
                TextToken token =
                    TextToken(f, prev + start, c + start, WordType::General);
                ctx->handleTextToken(token);
                prev = c;
            }
        }
        offset = nextOffset;
    }
}
}

#endif
