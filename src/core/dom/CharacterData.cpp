/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "StarFish.h"
#include "core/dom/CharacterData.h"
#include "core/dom/DOMException.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameText.h"

namespace StarFish {

void* CharacterData::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(CharacterData)] = { 0 };
        Node::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(CharacterData, m_data));
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

    setNeedsFrameTreeBuild();

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
