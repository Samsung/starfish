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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/CharacterData.h"
#include "core/dom/DOMException.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameText.h"

namespace StarFish {

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

String* CharacterData::substringData(unsigned long offset, unsigned long count)
{
    // https://dom.spec.whatwg.org/#concept-cd-substring
    String* d = data();
    // Let length be node’s length.
    size_t length = d->length();
    // If offset is greater than length, then throw an IndexSizeError.
    if (offset > length) {
        throw new DOMException(document(), DOMException::Code::INDEX_SIZE_ERR);
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
    return d->substring(offset, subLength);
}

void CharacterData::setData(String* data)
{
    STARFISH_ASSERT(data);
    String* oldData = m_data;
    m_data = data;

    if (frame() && frame()->isFrameText() &&
        style()->textTransform() != NoneTextTransformValue) {
        frame()->asFrameText()->transformText(m_data);
    }

    setNeedsLayout();

    notifyDOMEventToParentTree(parentNode(), [oldData, data](Node* parent) {
        parent->didCharacterDataModified(oldData, data);
    });
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
        throw new DOMException(document(), DOMException::Code::INDEX_SIZE_ERR);
    }

    // TODO Queue a mutation record of "characterData" for node with oldValue
    // node’s data.
    String* oldValue = data();
    StringBuilder sb;
    sb.appendSubString(oldValue, 0, offset);
    sb.appendString(newData);
    sb.appendSubString(oldValue, offset, length);
    setData(sb.finalize());
}

void CharacterData::deleteData(unsigned long offset, unsigned long count)
{
    // TODO Queue a mutation record of "characterData" for node with oldValue
    // node’s data.
    size_t length = CharacterData::length();
    if (offset > length) {
        throw new DOMException(document(), DOMException::Code::INDEX_SIZE_ERR);
    }

    // If offset plus count is greater than length, then set count to length
    // minus offset.
    if ((offset + count) > length) {
        count = length - offset;
    }

    // TODO Queue a mutation record of "characterData" for node with oldValue
    // node’s data.
    String* oldValue = data();
    StringBuilder sb;
    sb.appendSubString(oldValue, 0, offset);
    sb.appendSubString(oldValue, offset + count, length);
    setData(sb.finalize());
}

void CharacterData::replaceData(unsigned long offset, unsigned long count,
                                String* newData)
{
    // TODO Queue a mutation record of "characterData" for node with oldValue
    // node’s data.
    size_t length = CharacterData::length();
    if (offset > length) {
        throw new DOMException(document(), DOMException::Code::INDEX_SIZE_ERR);
    }

    // If offset plus count is greater than length, then set count to length
    // minus offset.
    if ((offset + count) > length) {
        count = length - offset;
    }

    // TODO Queue a mutation record of "characterData" for node with oldValue
    // node’s data.
    String* oldValue = data();
    StringBuilder sb;
    sb.appendSubString(oldValue, 0, offset);
    sb.appendString(newData);
    sb.appendSubString(oldValue, offset + count, length);
    setData(sb.finalize());
}
}
