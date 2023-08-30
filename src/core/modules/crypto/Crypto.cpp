/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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
#include "EscargotPublic.h"
#include "binding/ScriptBindingInstance.h"
#include "core/util/RandomEngine.h"
#include "core/dom/DOMException.h"
#include "core/modules/crypto/Crypto.h"

namespace Starfish {
Crypto::Crypto(ExecutionContext* executionContext)
    : ScriptWrappable(this)
    , m_executionContext(executionContext){};

ScriptArrayBufferView Crypto::getRandomValues(ScriptArrayBufferView array)
{
    // https://w3c.github.io/webcrypto/#dfn-Crypto-method-getRandomValues
    uint8_t* buffer = arrayBufferViewRawData(array);
    unsigned byteSize = arrayBufferViewByteSize(array);

    // 1. If array is not an Int8Array, Uint8Array, Uint8ClampedArray,
    // Int16Array, Uint16Array, Int32Array, Uint32Array, BigInt64Array, or
    // BigUint64Array, then throw a TypeMismatchError and terminate the
    // algorithm.
    if (!isIntTypeArray(array)) {
        throw new DOMException(
            executionContext(), DOMException::Code::TYPE_MISMATCH_ERR,
            "The provided ArrayBufferView is not an integer array type.");
    }

    // 2. If the byteLength of array is greater than 65536, throw a
    // QuotaExceededError and terminate the algorithm.
    if (byteSize > 65536) {
        throw new DOMException(
            executionContext(), DOMException::Code::QUOTA_EXCEEDED_ERR,
            "The ArrayBufferView's byte length exceeds the number of bytes of "
            "entropy available via this API");
    }
    // 3. Overwrite all elements of array with cryptographically strong random
    // values of the appropriate type.
    makeRandomByte(buffer, byteSize);

    return array;
}

bool Crypto::isIntTypeArray(ScriptArrayBufferView array)
{
    if (array->asObject()->isInt8ArrayObject()) {
        return true;
    }
    if (array->asObject()->isInt8ArrayObject()) {
        return true;
    }
    if (array->asObject()->isUint8ArrayObject()) {
        return true;
    }
    if (array->asObject()->isUint8ClampedArrayObject()) {
        return true;
    }
    if (array->asObject()->isInt16ArrayObject()) {
        return true;
    }
    if (array->asObject()->isUint16ArrayObject()) {
        return true;
    }
    if (array->asObject()->isInt32ArrayObject()) {
        return true;
    }
    if (array->asObject()->isUint32ArrayObject()) {
        return true;
    }
    if (array->asObject()->isBigInt64ArrayObject()) {
        return true;
    }
    if (array->asObject()->isBigUint64ArrayObject()) {
        return true;
    }
    return false;
}

void Crypto::makeRandomByte(uint8_t* buffer, unsigned byteSize)
{
    std::uniform_int_distribution<uint32_t> distribution(0, 0xFFFFFFFFu);
    int offset = 0;
    uint32_t bits = 0;
    for (unsigned i = 0; i < byteSize; i++) {
        if (offset == 0) {
            bits = distribution(RandomEngine::instance().mt19937());
        }
        *(buffer++) = static_cast<char>(bits & 0xFF);
        bits >>= 8;
        if (++offset >= 4) {
            offset = 0;
        }
    }
}
} // namespace Starfish
