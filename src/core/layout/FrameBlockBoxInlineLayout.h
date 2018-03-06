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
    bool m_isFirstLine;

    TextToken(FrameText* ft, size_t start, size_t end, WordType type,
              bool isFirstLine = false)
        : m_frameText(ft)
        , m_start(start)
        , m_end(end)
        , m_type(type)
        , m_isFirstLine(isFirstLine)
    {
        setWidth(type);
    }

    bool isWhiteSpace() const
    {
        return m_type != General;
    }

    ComputedStyle* style()
    {
        return m_frameText->style(m_frameText->parent(),
                                  m_frameText->parent()->style(),
                                  m_isFirstLine);
    }

    void setWidth(WordType type)
    {
        switch (type) {
        case CollapsibleWhiteSpace:
            m_width = style()->font()->spaceWidth();
            break;
        case NonCollapsibleWhiteSpace:
            m_width = style()->font()->spaceWidth() * (m_end - m_start);
            break;
        case ForcedNewline:
            m_width = 0;
            break;
        case General:
            m_width = style()->font()->measureText(
                StringView(m_frameText->text(), m_start, m_end));
            break;
        }
    }

    void setIsFirstLine(bool isFirstLine)
    {
        m_isFirstLine = isFirstLine;
    }

    bool isFirstLine() const
    {
        return m_isFirstLine;
    }

    LayoutUnit width() const
    {
        return m_width;
    }

#ifndef NDEBUG
    void dump()
    {
        UTF8StringDataNonGCStd str = m_frameText->text()
                                         ->substring(m_start, m_end - m_start)
                                         ->toUTF8NonGCString();
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
    if (String::isFixedWidthChar(c)) {
        return false;
    }
    return String::isSpaceOrNewline(c);
}
}

#endif
