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
#include "core/fileapi/Blob.h"

#include <EscargotPublic.h>
using namespace Escargot;

namespace StarFish {

ValueRef* blobConstructor(ExecutionStateRef* state, ValueRef* thisValue,
                          size_t argCount, ValueRef** argv,
                          bool isNewExpression)
{
    // https://www.w3.org/TR/FileAPI/#blob-constructor-steps
    if (argCount == 0) {
        Document* document = fetchDocument(state->context());
        Blob* b =
            new Blob(document, 0, String::emptyString, nullptr, false, false);
        return b->scriptValue();
    }
    ValueRef* firstArg = argv[0];

    void* bytes = 0;
    ValueRef* lengthString = ValueRef::create(
        AtomicStringRef::create(state->context(), "length")->string());
    ObjectRef* obj = nullptr;
    ValueRef* lengthValue = ValueRef::createUndefined();

    if (!firstArg->isObject() ||
        (lengthValue = (obj = firstArg->asObject())->get(state, lengthString))
            ->isUndefinedOrNull()) {
        COMPOSE_MESSAGE(reason, ARG_TYPE_MISMATCH_WITH_INDEXABLE_TYPE, "0",
                        "blobParts");
        COMPOSE_MESSAGE(msg, FAILED_TO_CONSTRUCT, "Blob", reason);
        THROW_EXCEPTION(msg);
    }

    size_t length = (size_t)lengthValue->toNumber(state);

    GCVector<std::pair<void*, size_t>> bufferInfo;
    size_t totalByteLength = 0;
    for (size_t i = 0; i < length; i++) {
        ValueRef* element = obj->get(state, ValueRef::create(i));

        // ArrayBufferView
        if (element->isObject() && element->asObject()->isArrayBufferView()) {
            ArrayBufferViewRef* v = element->asObject()->asArrayBufferView();
            const char* p = (const char*)v->buffer()->rawBuffer();
            bufferInfo.push_back(std::make_pair((void*)p, v->bytelength()));
            totalByteLength += v->bytelength();
            continue;
        } else if (element->isObject() &&
                   element->asObject()->isArrayBufferObject()) {
            ArrayBufferObjectRef* v =
                element->asObject()->asArrayBufferObject();
            bufferInfo.push_back(
                std::make_pair((void*)v->rawBuffer(), v->bytelength()));
            totalByteLength += v->bytelength();
            continue;
        } else if (element->isObject() && element->asObject()->extraData()) {
            void* extraData = element->asObject()->extraData();
            if (((ScriptWrappable*)extraData)->isBlob()) {
                Blob* bb = (Blob*)extraData;
                bufferInfo.push_back(std::make_pair(bb->data(), bb->size()));
                totalByteLength += bb->size();
                continue;
            }
        }

        // otherwise, toString()
        NullableUTF8String s =
            toBrowserString(state, element)->toNullableUTF8String();
        bufferInfo.push_back(std::make_pair((void*)s.m_buffer, s.m_bufferSize));
        totalByteLength += s.m_bufferSize;
    }

    size_t offset = 0;
    char* buffer = (char*)GC_MALLOC_ATOMIC_IGNORE_OFF_PAGE(totalByteLength);
    for (size_t i = 0; i < bufferInfo.size(); i++) {
        memcpy(buffer + offset, bufferInfo[i].first, bufferInfo[i].second);
        offset += bufferInfo[i].second;
    }

    STARFISH_ASSERT(offset == totalByteLength);

    ValueRef* secondArg = argCount >= 2 ? argv[1] : scriptUndefined();
    String* type = String::emptyString;
    if (secondArg->isUndefinedOrNull()) {
        type = toBrowserString(state, secondArg)->toLower();
    }

    Document* document = fetchDocument(state->context());
    Blob* newBlob =
        new Blob(document, totalByteLength, type, buffer, false, false);
    return newBlob->scriptValue();
}
}
