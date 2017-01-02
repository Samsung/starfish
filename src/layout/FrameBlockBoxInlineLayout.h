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

#ifndef __StarFishFrameBlockBoxInlineLayout__
#define __StarFishFrameBlockBoxInlineLayout__

namespace StarFish {

// Tokenize a text using the ICU divider
template <typename fn>
void textDividerForLayout(StarFish* sf, String* txt, fn f)
{
    // TODO consider white-space
    unsigned offset = 0;
    while (true) {
        if (offset >= txt->length())
            break;
        bool isWhiteSpace = false;
        if (String::isSpaceOrNewline(txt->charAt(offset))) {
            isWhiteSpace = true;
        }

        // find next space
        unsigned nextOffset = offset + 1;
        if (isWhiteSpace) {
            while (nextOffset < txt->length() && String::isSpaceOrNewline((*txt)[nextOffset])) {
                nextOffset++;
            }
            f(txt, offset, nextOffset, isWhiteSpace, true);
        } else {
            size_t start = offset;
            while (nextOffset < txt->length() && !String::isSpaceOrNewline((*txt)[nextOffset])) {
                nextOffset++;
            }

            auto breaker = sf->lineBreaker();
            breaker->setText(txt->toUnicodeString(start, nextOffset));
            int32_t c, prev = 0;
            size_t txtLen = txt->length();
            while (((c = breaker->next()) != icu::BreakIterator::DONE) && (c + start <= txtLen)) {
                f(txt, prev + start, c + start, isWhiteSpace, (prev + start != start));
                prev = c;
            }
        }
        offset = nextOffset;
    }
}

}

#endif
