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

#ifndef __StarFishBlobCustomBinding__
#define __StarFishBlobCustomBinding__

namespace StarFish {

using namespace escargot;

ESValue blobCustomFunction(ESVMInstance* instance)
{
    // https://www.w3.org/TR/FileAPI/#blob-constructor-steps
    int argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount == 0) {
        Window* w = ((Window*)ESVMInstance::currentInstance()
                         ->globalObject()
                         ->extraPointerData());
        Blob* b = new Blob(w->starFish(), 0, String::emptyString, nullptr,
                           false, false);
        return b->scriptValue();
    }
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);

    void* bytes = 0;
    ESValue lengthString = currentInstance->strings().length.string();
    ESObject* obj = nullptr;
    ESValue lengthValue;

    if (!firstArg.isObject() ||
        (lengthValue = (obj = firstArg.toObject())->get(lengthString))
            .isUndefinedOrNull()) {
        auto msg = ESString::create(
            "Failed to construct 'Blob': The 1st argument is "
            "neither "
            "an array, nor does it have indexed properties.");
        instance->throwError(ESValue(TypeError::create(msg)));
    }

    size_t length = (size_t)lengthValue.toNumber();

    GCVector<std::pair<void*, size_t>> bufferInfo;
    size_t totalByteLength = 0;
    for (size_t i = 0; i < length; i++) {
        ESValue element = obj->get(ESValue(i));

#ifdef USE_ES6_FEATURE
        // ESArrayBufferView
        if (element.isESPointer() &&
            element.asESPointer()->isESArrayBufferView()) {
            ESArrayBufferView* v = element.asESPointer()->asESArrayBufferView();
            const char* p = (const char*)v->buffer()->data();
            p += v->byteoffset();
            bufferInfo.push_back(std::make_pair((void*)p, v->bytelength()));
            totalByteLength += v->bytelength();
        }

        // ESArrayBufferObject
        if (element.isESPointer() &&
            element.asESPointer()->isESArrayBufferObject()) {
            ESArrayBufferObject* v =
                element.asESPointer()->asESArrayBufferObject();
            bufferInfo.push_back(
                std::make_pair((void*)v->data(), v->bytelength()));
            totalByteLength += v->bytelength();
        }
#endif
        // Blob
        if (element.isObject()) {
            ESObject* o = element.toObject();
            if (o->extraData() == kEscargotObjectCheckMagic &&
                ((ScriptWrappable*)o->extraPointerData())->isBlob()) {
                Blob* bb = (Blob*)o->extraPointerData();
                bufferInfo.push_back(std::make_pair(bb->data(), bb->size()));
                totalByteLength += bb->size();
                continue;
            }
        }

        // otherwise, toString()
        NullableUTF8String s =
            toBrowserString(element.toString())->toNullableUTF8String();
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

    ESValue secondArg = instance->currentExecutionContext()->readArgument(1);
    String* type = String::emptyString;
    if (!secondArg.isUndefinedOrNull()) {
        type = toBrowserString(secondArg.toString())->toLower();
    }

    Window* w = ((Window*)ESVMInstance::currentInstance()
                     ->globalObject()
                     ->extraPointerData());
    Blob* newBlob =
        new Blob(w->starFish(), totalByteLength, type, buffer, false, false);
    return newBlob->scriptValue();
}
}

#endif
