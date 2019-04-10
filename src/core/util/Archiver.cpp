/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

/*
 * Tencent is pleased to support the open source community by making RapidJSON
 * available.
 *
 * Copyright (C) 2015 THL A29 Limited, a Tencent company, and Milo Yip. All
 * rights reserved.
 *
 * Licensed under the MIT License (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License
 * at
 *
 * http://opensource.org/licenses/MIT
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

#if defined(STARFISH_ENABLE_SERVICE_WORKER)

#include "StarfishConfig.h"

#include "core/util/Id.h"
#include "core/util/Archiver.h"

#include <cassert>
#include <stack>
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

namespace Starfish {

struct JsonReaderStackItem {
    enum State { BeforeStart, Started, Closed };

    JsonReaderStackItem(const rapidjson::Value* value, State state)
        : value(value)
        , state(state)
        , index()
    {
    }

    const rapidjson::Value* value;
    State state;
    rapidjson::SizeType index;
};

typedef std::stack<JsonReaderStackItem> JsonReaderStack;

#define DOCUMENT reinterpret_cast<rapidjson::Document*>(mDocument)
#define STACK (reinterpret_cast<JsonReaderStack*>(mStack))
#define TOP (STACK->top())
#define CURRENT (*TOP.value)

JsonReader::JsonReader(const char* json)
    : mDocument()
    , mStack()
    , mError(false)
{
    mDocument = new rapidjson::Document;
    DOCUMENT->Parse(json);
    if (DOCUMENT->HasParseError()) {
        mError = true;
    } else {
        mStack = new JsonReaderStack;
        STACK->push(
            JsonReaderStackItem(DOCUMENT, JsonReaderStackItem::BeforeStart));
    }
}

JsonReader::~JsonReader()
{
    delete DOCUMENT;
    delete STACK;
}

// Archive concept
JsonReader& JsonReader::StartObject()
{
    if (!mError) {
        if (CURRENT.IsObject() &&
            TOP.state == JsonReaderStackItem::BeforeStart) {
            TOP.state = JsonReaderStackItem::Started;
        } else {
            mError = true;
        }
    }
    return *this;
}

JsonReader& JsonReader::EndObject()
{
    if (!mError) {
        if (CURRENT.IsObject() && TOP.state == JsonReaderStackItem::Started) {
            Next();
        } else {
            mError = true;
        }
    }
    return *this;
}

JsonReader& JsonReader::Member(const char* name)
{
    if (!mError) {
        if (CURRENT.IsObject() && TOP.state == JsonReaderStackItem::Started) {
            rapidjson::Value::ConstMemberIterator memberItr =
                CURRENT.FindMember(name);
            if (memberItr != CURRENT.MemberEnd()) {
                STACK->push(JsonReaderStackItem(
                    &memberItr->value, JsonReaderStackItem::BeforeStart));
            } else {
                mError = true;
            }
        } else {
            mError = true;
        }
    }
    return *this;
}

bool JsonReader::HasMember(const char* name) const
{
    if (!mError && CURRENT.IsObject() &&
        TOP.state == JsonReaderStackItem::Started) {
        return CURRENT.HasMember(name);
    }

    return false;
}

JsonReader& JsonReader::StartArray(size_t* size)
{
    if (!mError) {
        if (CURRENT.IsArray() &&
            TOP.state == JsonReaderStackItem::BeforeStart) {
            TOP.state = JsonReaderStackItem::Started;
            if (size) {
                *size = CURRENT.Size();
            }

            if (!CURRENT.Empty()) {
                const rapidjson::Value* value = &CURRENT[TOP.index];
                STACK->push(JsonReaderStackItem(
                    value, JsonReaderStackItem::BeforeStart));
            } else {
                TOP.state = JsonReaderStackItem::Closed;
            }
        } else {
            mError = true;
        }
    }
    return *this;
}

JsonReader& JsonReader::EndArray()
{
    if (!mError) {
        if (CURRENT.IsArray() && TOP.state == JsonReaderStackItem::Closed) {
            Next();
        } else {
            mError = true;
        }
    }
    return *this;
}

JsonReader& JsonReader::operator&(bool& b)
{
    if (!mError) {
        if (CURRENT.IsBool()) {
            b = CURRENT.GetBool();
            Next();
        } else {
            mError = true;
        }
    }
    return *this;
}

JsonReader& JsonReader::operator&(unsigned& u)
{
    if (!mError) {
        if (CURRENT.IsUint()) {
            u = CURRENT.GetUint();
            Next();
        } else {
            mError = true;
        }
    }
    return *this;
}

JsonReader& JsonReader::operator&(int& i)
{
    if (!mError) {
        if (CURRENT.IsInt()) {
            i = CURRENT.GetInt();
            Next();
        } else {
            mError = true;
        }
    }
    return *this;
}

JsonReader& JsonReader::operator&(double& d)
{
    if (!mError) {
        if (CURRENT.IsNumber()) {
            d = CURRENT.GetDouble();
            Next();
        } else {
            mError = true;
        }
    }
    return *this;
}

JsonReader& JsonReader::operator&(std::string& s)
{
    if (!mError) {
        if (CURRENT.IsString()) {
            s = CURRENT.GetString();
            Next();
        } else {
            mError = true;
        }
    }
    return *this;
}

JsonReader& JsonReader::operator&(String*& s)
{
    if (!mError) {
        if (CURRENT.IsString()) {
            s = String::createASCIIString(CURRENT.GetString());
            Next();
        } else {
            mError = true;
        }
    }
    return *this;
}

JsonReader& JsonReader::SetNull()
{
    mError = true;
    return *this;
}

void JsonReader::Next()
{
    if (!mError) {
        assert(!STACK->empty());
        STACK->pop();

        if (!STACK->empty() && CURRENT.IsArray()) {
            if (TOP.state ==
                JsonReaderStackItem::Started) { // Otherwise means reading array
                                                // item pass end
                if (TOP.index < CURRENT.Size() - 1) {
                    const rapidjson::Value* value = &CURRENT[++TOP.index];
                    STACK->push(JsonReaderStackItem(
                        value, JsonReaderStackItem::BeforeStart));
                } else {
                    TOP.state = JsonReaderStackItem::Closed;
                }
            } else {
                mError = true;
            }
        }
    }
}

#undef DOCUMENT
#undef STACK
#undef TOP
#undef CURRENT

////////////////////////////////////////////////////////////////////////////////
// JsonWriter

#define WRITER \
    reinterpret_cast<rapidjson::PrettyWriter<rapidjson::StringBuffer>*>(mWriter)
#define STREAM reinterpret_cast<rapidjson::StringBuffer*>(mStream)

JsonWriter::JsonWriter()
    : mWriter()
    , mStream()
{
    mStream = new rapidjson::StringBuffer;
    mWriter = new rapidjson::PrettyWriter<rapidjson::StringBuffer>(*STREAM);
}

JsonWriter::~JsonWriter()
{
    delete WRITER;
    delete STREAM;
}

const char* JsonWriter::GetString() const
{
    return STREAM->GetString();
}

JsonWriter& JsonWriter::StartObject()
{
    WRITER->StartObject();
    return *this;
}

JsonWriter& JsonWriter::EndObject()
{
    WRITER->EndObject();
    return *this;
}

JsonWriter& JsonWriter::Member(const char* name)
{
    WRITER->String(name, static_cast<rapidjson::SizeType>(strlen(name)));
    return *this;
}

bool JsonWriter::HasMember(const char*) const
{
    // This function is for JsonReader only.
    assert(false);
    return false;
}

JsonWriter& JsonWriter::StartArray(size_t*)
{
    WRITER->StartArray();
    return *this;
}

JsonWriter& JsonWriter::EndArray()
{
    WRITER->EndArray();
    return *this;
}

JsonWriter& JsonWriter::operator&(bool& b)
{
    WRITER->Bool(b);
    return *this;
}

JsonWriter& JsonWriter::operator&(unsigned& u)
{
    WRITER->Uint(u);
    return *this;
}

JsonWriter& JsonWriter::operator&(int& i)
{
    WRITER->Int(i);
    return *this;
}

JsonWriter& JsonWriter::operator&(double& d)
{
    WRITER->Double(d);
    return *this;
}

JsonWriter& JsonWriter::operator&(std::string& s)
{
    WRITER->String(s.c_str(), static_cast<rapidjson::SizeType>(s.size()));
    return *this;
}

JsonWriter& JsonWriter::operator&(String*& s)
{
    WRITER->String(CSTR(s), static_cast<rapidjson::SizeType>(s->length()));
    return *this;
}

JsonWriter& JsonWriter::SetNull()
{
    WRITER->Null();
    return *this;
}

#undef STREAM
#undef WRITER

} // namespace Starfish

#endif
