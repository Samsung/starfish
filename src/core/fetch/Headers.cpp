/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#include <EscargotPublic.h>
using namespace Escargot;

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/DOMException.h"
#include "platform/network/http/HTTPUtil.h"
#include "core/fetch/Headers.h"
#include "core/fetch/HeadersData.h"
#include "core/dom/ExecutionContext.h"

#define THROW_SCRIPT_TYPE_ERROR_IF_NEEDS()                               \
    do {                                                                 \
        if (error) {                                                     \
            throw new DOMException(executionContext(),                   \
                                   DOMException::Code::SCRIPT_TYPE_ERR); \
        }                                                                \
    } while (0)

namespace Starfish {

class HeadersIterationSource final
    : public IterationSource<Nullable<String*>, Nullable<String*>> {
public:
    HeadersIterationSource(HTTPHeaderMap* headerMap)
    {
        cloneHeaderMap(headerMap);
        m_iterator = m_headerMap.begin();
    }

    bool next(ExecutionStateRef* state, Nullable<String*>& key,
              Nullable<String*>& value)
    {
        if (m_iterator == m_headerMap.end()) {
            return false;
        }

        key = String::fromUTF8(m_iterator->first.data());
        value = String::fromUTF8(m_iterator->second.data());
        m_iterator++;
        return true;
    }

    void cloneHeaderMap(HTTPHeaderMap* headerMap)
    {
        auto srcHeaderMap = headerMap->headerMap();
        for (auto it = srcHeaderMap.begin(); it != srcHeaderMap.end(); ++it) {
            m_headerMap.insert(
                std::pair<std::string, std::string>(it->first, it->second));
        }
    }

private:
    std::map<std::string, std::string> m_headerMap; // Should be sorted.
    std::map<std::string, std::string>::iterator m_iterator;
};

Headers::Headers(ExecutionContext* executionContext)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_headersData(new HeadersData())
{
}

Headers::Headers(ExecutionContext* executionContext, HeadersInit headersInit)
    : Headers(executionContext)
{
    fill(headersInit);
}

ScriptBindingInstance* Headers::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

void Headers::fill(HeadersInit headersInit)
{
    // TODO: Move to binding dir.
    if (_CHECK_TYPEOF(headersInit, Headers)) {
        Headers* headers = (Headers*)(headersInit->asObject()->extraData());
        initHeadersFromHeaders(headers);
    } else if (headersInit->isObject()) {
        auto object = headersInit->asObject();
        if (object->isArrayObject()) {
            // [["key1", "value1"], ["key2", "value2"]]
            initHeadersFromArrayObject(object);
        } else {
            // {"key1": "value1", "key2": "value2"}
            initHeadersFromObject(object);
        }
    }
}

void Headers::initHeadersFromHeaders(Headers* headers)
{
    auto srcHeaderMap = headers->m_headersData->httpHeaderMap()->headerMap();
    for (auto it = srcHeaderMap.begin(); it != srcHeaderMap.end(); ++it) {
        m_headersData->httpHeaderMap()->append(it->first, it->second);
    }
}

void Headers::initHeadersFromArrayObject(ScriptObject object)
{
    ContextRef* ctx = scriptBindingInstance()->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(ctx);
    ValueVectorRef* values = object->getOwnPropertyKeys(state);

    for (size_t i = 0; i < values->size(); i++) {
        ValueRef* key = values->at(i);
        if (key->isNumber() && object->hasOwnProperty(state, key)) {
            ScriptValue subObject = object->get(state, key);
            if (subObject->isObject()) {
                ScriptObject element = subObject->asObject();
                if (element->isArrayObject()) {
                    ScriptValue length = element->get(
                        state,
                        ValueRef::create(StringRef::fromASCII("length")));
                    if (length->asNumber() != 2.0) {
                        throw new DOMException(
                            executionContext(),
                            DOMException::Code::SCRIPT_TYPE_ERR);
                    }
                    auto elementKey = element->get(state, ValueRef::create(0));
                    setHeader(elementKey,
                              element->get(state, ValueRef::create(1)), state);
                }
            }
        }
    }
}

void Headers::initHeadersFromObject(ScriptObject object)
{
    ContextRef* ctx = scriptBindingInstance()->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(ctx);
    ValueVectorRef* values = object->getOwnPropertyKeys(state);

    for (size_t i = 0; i < values->size(); i++) {
        auto key = values->at(i);
        if (key->isString() && object->hasOwnProperty(state, key)) {
            ScriptValue value = object->get(state, key);
            setHeader(key, value, state);
        }
    }
}

void Headers::setHeader(ScriptValue keyValue, ScriptValue nameValue,
                        ExecutionStateRef* state)
{
    String* name = toBrowserString(state, keyValue->toString(state));
    String* value = toBrowserString(state, nameValue->toString(state));

    bool error = false;
    m_headersData->set(name, value, &error);
    THROW_SCRIPT_TYPE_ERROR_IF_NEEDS();
}

Nullable<String*> Headers::get(String* name)
{
    bool error = false;
    auto nullable = m_headersData->get(name, &error);
    THROW_SCRIPT_TYPE_ERROR_IF_NEEDS();
    return nullable;
}

void Headers::append(String* name, String* value)
{
    bool error = false;
    m_headersData->append(name, value, &error);
    THROW_SCRIPT_TYPE_ERROR_IF_NEEDS();
}

void Headers::set(String* name, String* value)
{
    bool error = false;
    m_headersData->set(name, value, &error);
    THROW_SCRIPT_TYPE_ERROR_IF_NEEDS();
}

void Headers::noCheckValidSet(const std::string& lowerCaseName,
                              const std::string& value)
{
    m_headersData->noCheckValidSet(lowerCaseName, value);
}

bool Headers::has(String* name)
{
    bool error = false;
    bool ret = m_headersData->has(name, &error);
    THROW_SCRIPT_TYPE_ERROR_IF_NEEDS();
    return ret;
}

bool Headers::noCheckValidHas(const std::string& lowerCaseName)
{
    return m_headersData->noCheckValidHas(lowerCaseName);
}

void Headers::deleteHeader(String* name)
{
    bool error = false;
    m_headersData->deleteHeader(name, &error);
    THROW_SCRIPT_TYPE_ERROR_IF_NEEDS();
}

void Headers::setGuard(Guard guard)
{
    m_headersData->setGuard(guard);
}

Guard Headers::guard()
{
    return m_headersData->guard();
}

void Headers::copyHeaders(Headers* src)
{
    initHeadersFromHeaders(src);
    setGuard(src->guard());
}

String* Headers::extractMIMEType()
{
    return m_headersData->extractMIMEType();
}

IterationSource<Nullable<String*>, Nullable<String*>>* Headers::startIteration(
    ExecutionStateRef* state)
{
    return new HeadersIterationSource(m_headersData->httpHeaderMap());
}
}
