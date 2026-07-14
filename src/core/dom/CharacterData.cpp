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
#include "core/dom/CharacterData.h"
#include "core/dom/DOMException.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameText.h"
#include "core/dom/MutationObservationScope.h"
#if defined(STARFISH_ENABLE_TTS) && \
    defined(STARFISH_ENABLE_A11Y_TOUCH_EXPLORATION)
#include "core/modules/tts/A11yLiveRegion.h"
#endif

namespace Starfish {

void* CharacterData::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(CharacterData));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(CharacterData)] = { 0 };
        CharacterData::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(CharacterData));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

uint32_t CharacterData::length() const
{
    // https://www.w3.org/TR/DOM-Level-3-Core/core.html
    // according spec, we should treat length as utf-16 unit offset.
    const auto& s = data()->bufferAccessData();
    if (s.bufferDataKind != StringBufferAccessData::UTF32Data) {
        size_t len = 0;
        for (size_t i = 0; i < s.length; i++) {
            char32_t ch = s.charAt(i);
            char16_t buf[2];
            len += utf32ToUtf16(ch, buf);
        }
        return len;
    } else {
        return s.length;
    }
}

String* CharacterData::substringData(unsigned long offset, unsigned long count)
{
    // https://dom.spec.whatwg.org/#concept-cd-substring
    // Let length be node’s length.
    size_t length = CharacterData::length();
    // If offset is greater than length, then throw an IndexSizeError.
    if (offset > length) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INDEX_SIZE_ERR);
    }
    // If offset plus count is greater than length, return a string whose value
    // is the code units from the offsetth code unit to the end of node’s data,
    // and then return.
    // Return a string whose value is the code units from the offsetth code unit
    // to the offset+countth code unit in node’s data.
    size_t subLength = count;
    if (offset + count > length) {
        subLength = length - offset;
    }

    // https://www.w3.org/TR/DOM-Level-3-Core/core.html
    // according spec, we should treat offset, length as utf-16 unit offset.
    const auto& s = data()->bufferAccessData();
    if (s.bufferDataKind != StringBufferAccessData::UTF32Data) {
        return data()->substring(offset, subLength);
    } else {
        auto str = data()->toUTF16NonGCString().substr(offset, subLength);
        return String::fromUTF16(str.data(), str.length());
    }
}

void CharacterData::setData(String* data)
{
    STARFISH_ASSERT(data);
    String* oldData = m_data;
    m_data = data;

    MutationObservationScope scop;
    scop.startCharacterDataMutationScope(this, oldData);

    if (frame() && frame()->isFrameText() &&
        style()->textTransform() != NoneTextTransformValue) {
        frame()->asFrameText()->transformText(m_data);
    }

    setNeedsFrameTreeBuild();

    notifyDOMEventToParentTree(parentNode(), [oldData, data](Node* parent) {
        parent->didCharacterDataModified(oldData, data);
    });

#if defined(STARFISH_ENABLE_TTS) && \
    defined(STARFISH_ENABLE_A11Y_TOUCH_EXPLORATION)
    // Text swaps inside aria-live regions (e.g. node.data = "...") do not
    // go through node insertion; announce them here.
    A11yLiveRegion::characterDataChanged(this);
#endif
}

void CharacterData::appendData(String* d)
{
    // TODO Queue a mutation record of "characterData" for node with oldValue
    // node’s data.
    setData(data()->concat(d));
}

void CharacterData::insertData(unsigned long offset, String* newData)
{
    size_t length = CharacterData::length();
    if (offset > length) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INDEX_SIZE_ERR);
    }

    // TODO Queue a mutation record of "characterData" for node with oldValue

    // https://www.w3.org/TR/DOM-Level-3-Core/core.html
    // according spec, we should treat offset as utf-16 unit offset.

    String* oldValue = data();
    const auto& s = oldValue->bufferAccessData();

    if (s.bufferDataKind != StringBufferAccessData::UTF32Data) {
        StringBuilder sb;
        sb.appendSubString(oldValue, 0, offset);
        sb.appendString(newData);
        sb.appendSubString(oldValue, offset, length);
        setData(sb.finalize());
    } else {
        auto u16String = oldValue->toUTF16NonGCString();
        auto u16NewString = newData->toUTF16NonGCString();
        auto newString = u16String.substr(0, offset) + u16NewString +
                         u16String.substr(offset);
        setData(String::fromUTF16(newString.data(), newString.length()));
    }
}

void CharacterData::deleteData(unsigned long offset, unsigned long count)
{
    // TODO Queue a mutation record of "characterData" for node with oldValue
    // node’s data.
    size_t length = CharacterData::length();
    if (offset > length) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INDEX_SIZE_ERR);
    }

    // If offset plus count is greater than length, then set count to length
    // minus offset.
    if ((offset + count) > length) {
        count = length - offset;
    }

    // https://www.w3.org/TR/DOM-Level-3-Core/core.html
    // according spec, we should treat offset, length as utf-16 unit offset.
    const auto& s = data()->bufferAccessData();
    if (s.bufferDataKind != StringBufferAccessData::UTF32Data) {
        String* oldValue = data();
        StringBuilder sb;
        sb.appendSubString(oldValue, 0, offset);
        sb.appendSubString(oldValue, offset + count, length);
        setData(sb.finalize());
    } else {
        auto u16String = data()->toUTF16NonGCString();
        auto newString =
            u16String.substr(0, offset) + u16String.substr(offset + count);
        setData(String::fromUTF16(newString.data(), newString.length()));
    }
}

void CharacterData::replaceData(unsigned long offset, unsigned long count,
                                String* newData)
{
    // TODO Queue a mutation record of "characterData" for node with oldValue
    // node’s data.
    size_t length = CharacterData::length();
    if (offset > length) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INDEX_SIZE_ERR);
    }

    // If offset plus count is greater than length, then set count to length
    // minus offset.
    if ((offset + count) > length) {
        count = length - offset;
    }

    // TODO Queue a mutation record of "characterData" for node with oldValue
    // node’s data.

    // https://www.w3.org/TR/DOM-Level-3-Core/core.html
    // according spec, we should treat offset, length as utf-16 unit offset.
    const auto& s = data()->bufferAccessData();
    if (s.bufferDataKind != StringBufferAccessData::UTF32Data) {
        String* oldValue = data();
        StringBuilder sb;
        sb.appendSubString(oldValue, 0, offset);
        sb.appendString(newData);
        sb.appendSubString(oldValue, offset + count, length);
        setData(sb.finalize());
    } else {
        auto u16String = data()->toUTF16NonGCString();
        auto newString = u16String.substr(0, offset) +
                         newData->toUTF16NonGCString() +
                         u16String.substr(offset + count);
        setData(String::fromUTF16(newString.data(), newString.length()));
    }
}
} // namespace Starfish
