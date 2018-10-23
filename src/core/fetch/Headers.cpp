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
#include "core/dom/Document.h"

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

Headers::Headers(Document* document)
    : ScriptWrappable(this)
    , DocumentHoldable(document)
    , m_instance(document->scriptBindingInstance())
    , m_guard(Guard::None)
{
}

Headers::Headers(Document* document, HeadersInit headersInit)
    : Headers(document)
{
    fill(headersInit);
}

// https://tools.ietf.org/html/rfc2616#section-2.2
bool Headers::isValidHTTPToken(const String* name)
{
    if (name->isEmpty()) {
        return false;
    }

    for (size_t i = 0; i < name->length(); i++) {
        auto c = name->charAt(i);
        if (c <= 0x20 || c >= 0x7F || c == '(' || c == ')' || c == '<' ||
            c == '>' || c == '@' || c == ',' || c == ';' || c == ':' ||
            c == '\\' || c == '"' || c == '/' || c == '[' || c == ']' ||
            c == '?' || c == '=' || c == '{' || c == '}') {
            return false;
        }
    }
    return true;
}

bool Headers::isValidHTTPHeaderValue(const String* value)
{
    for (size_t i = 0; i < value->length(); i++) {
        auto c = value->charAt(i);
        if (c > 0xFF) {
            return false;
        }
    }
    return true;
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
    auto srcHeaderMap = headers->m_headerMap.headerMap();
    for (auto it = srcHeaderMap.begin(); it != srcHeaderMap.end(); ++it) {
        m_headerMap.setHeader(it->first, it->second);
    }
}

void Headers::initHeadersFromArrayObject(ScriptObject object)
{
    ContextRef* ctx = m_instance->scriptContext();
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
                            document(), DOMException::Code::SCRIPT_TYPE_ERR);
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
    ContextRef* ctx = m_instance->scriptContext();
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
    checkValidHeader(name, value);

    m_headerMap.setHeader(name->toLower()->toUTF8NonGCString(),
                          value->trim()->toUTF8NonGCString());
}

Nullable<String*> Headers::get(String* name)
{
    checkValidHeader(name);
    Nullable<std::string> value =
        noCheckValidGet(name->toLower()->toUTF8NonGCString());
    if (value.hasValue()) {
        return String::fromUTF8(value.getValue().data());
    }
    return nullptr;
}

Nullable<std::string> Headers::noCheckValidGet(const std::string& name)
{
    auto it = m_headerMap.findHeader(name);
    if (it == m_headerMap.headerMap().end()) {
        return nullptr;
    }
    return it->second;
}

void Headers::append(String* name, String* value)
{
    checkValidHeader(name, value);

    m_headerMap.setHeader(name->toLower()->toUTF8NonGCString(),
                          value->trim()->toUTF8NonGCString());
}

void Headers::set(String* name, String* value)
{
    checkValidHeader(name, value);
    noCheckValidSet(name->toLower()->toUTF8NonGCString().data(),
                    value->trim()->toUTF8NonGCString().data());
}

void Headers::noCheckValidSet(const std::string& name, const std::string& value)
{
    m_headerMap.headerMap()[name] = value;
}

bool Headers::has(String* name)
{
    checkValidHeader(name);
    return noCheckValidHas(name->toLower()->toUTF8NonGCString().data());
}

bool Headers::noCheckValidHas(const std::string& name)
{
    auto it = m_headerMap.findHeader(name);
    return !(it == m_headerMap.headerMap().end());
}

void Headers::deleteHeader(String* name)
{
    checkValidHeader(name);

    m_headerMap.removeHeader(name->toLower()->toUTF8NonGCString());
}

void Headers::checkValidHeader(String* name)
{
    if (!isValidHTTPToken(name)) {
        throw new DOMException(document(), DOMException::Code::SCRIPT_TYPE_ERR);
    }
    // TODO: check guard(https://fetch.spec.whatwg.org/#headers-class)
    // TODO: check CORS-safelisted
    // (https://fetch.spec.whatwg.org/#terminology-headers)
}

void Headers::checkValidHeader(String* name, String* value)
{
    checkValidHeader(name);

    if (!isValidHTTPHeaderValue(value)) {
        throw new DOMException(document(), DOMException::Code::SCRIPT_TYPE_ERR);
    }
}

void Headers::copyHeaders(Headers* src)
{
    initHeadersFromHeaders(src);
    m_guard = src->guard();
}

String* Headers::extractMIMEType()
{
    auto mimeType = noCheckValidGet("content-type");
    if (!mimeType.hasValue()) {
        return String::emptyString;
    }
    return String::fromUTF8(mimeType.getValue().data())->toLower();
}

IterationSource<Nullable<String*>, Nullable<String*>>* Headers::startIteration(
    ExecutionStateRef* state)
{
    return new HeadersIterationSource(&m_headerMap);
}
}
