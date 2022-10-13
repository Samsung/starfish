/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

ArchivableHandler_t Archiver::m_fpArchivableHandler = nullptr;

struct JsonReaderStackItem {
    enum State { BeforeStart, Started, Closed };

    JsonReaderStackItem(const rapidjson::Value* value, State state)
        : value(value)
        , state(state)
        , index()
    {
        STARFISH_ASSERT(value != nullptr);
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
    STARFISH_ASSERT(json != nullptr);

    mDocument = new rapidjson::Document;
    DOCUMENT->Parse(json);
    if (DOCUMENT->HasParseError() == true) {
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
    if (mError == false) {
        if ((CURRENT.IsObject() == true) &&
            (TOP.state == JsonReaderStackItem::BeforeStart)) {
            TOP.state = JsonReaderStackItem::Started;
        } else {
            mError = true;
        }
    }
    return *this;
}

JsonReader& JsonReader::EndObject()
{
    if (mError == false) {
        if ((CURRENT.IsObject() == true) &&
            (TOP.state == JsonReaderStackItem::Started)) {
            Next();
        } else {
            mError = true;
        }
    }
    return *this;
}

JsonReader& JsonReader::Member(const char* name)
{
    STARFISH_ASSERT(name != nullptr);
    ExecuteScope scope(this, name);

    if (mError == false) {
        if ((CURRENT.IsObject() == true) &&
            (TOP.state == JsonReaderStackItem::Started)) {
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
    STARFISH_ASSERT(name != nullptr);

    if ((mError == false) && (CURRENT.IsObject() == true) &&
        (TOP.state == JsonReaderStackItem::Started)) {
        return CURRENT.HasMember(name);
    }

    return false;
}

JsonReader& JsonReader::StartArray(size_t* size)
{
    STARFISH_ASSERT(size != nullptr);

    if (mError == false) {
        if ((CURRENT.IsArray() == true) &&
            (TOP.state == JsonReaderStackItem::BeforeStart)) {
            TOP.state = JsonReaderStackItem::Started;
            if (size != nullptr) {
                *size = CURRENT.Size();
            }

            if (CURRENT.Empty() == false) {
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
    if (mError == false) {
        if ((CURRENT.IsArray() == true) &&
            (TOP.state == JsonReaderStackItem::Closed)) {
            Next();
        } else {
            mError = true;
        }
    }
    return *this;
}

JsonReader& JsonReader::operator&(bool& b)
{
    if (mError == false) {
        if (CURRENT.IsBool() == true) {
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
    if (mError == false) {
        if (CURRENT.IsUint() == true) {
            u = CURRENT.GetUint();
            Next();
        } else {
            mError = true;
        }
    }
    return *this;
}

STARFISH_ASSERT_STATIC(sizeof(void*) == 8,
                       "Fixme archiving size_t on 32bit target");

JsonReader& JsonReader::operator&(size_t& u)
{
    if (mError == false) {
        if (CURRENT.IsUint64() == true) {
            u = CURRENT.GetUint64();
            Next();
        } else {
            mError = true;
        }
    }
    return *this;
}

JsonReader& JsonReader::operator&(int& i)
{
    if (mError == false) {
        if (CURRENT.IsInt() == true) {
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
    if (mError == false) {
        if (CURRENT.IsNumber() == true) {
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
    if (mError == false) {
        if (CURRENT.IsString() == true) {
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
    if (mError == false) {
        if (CURRENT.IsString() == true) {
            s = String::fromUTF8(CURRENT.GetString(),
                                 CURRENT.GetStringLength());
            Next();
        } else {
            mError = true;
        }
    }
    return *this;
}

JsonReader& JsonReader::operator&(
    std::unordered_map<std::string, std::string>& m)
{
    if (mError == false) {
        if (CURRENT.IsObject() == true) {
            StartObject();

            for (auto iter = CURRENT.MemberBegin(); iter != CURRENT.MemberEnd();
                 iter++) {
                m.insert(std::make_pair(std::string(iter->name.GetString()),
                                        std::string(iter->value.GetString())));
            }

            EndObject();

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
    if (mError == false) {
        STARFISH_ASSERT(STACK->empty() == false);
        STACK->pop();

        if ((STACK->empty() == false) && (CURRENT.IsArray() == true)) {
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

size_t JsonWriter::GetSize() const
{
    return STREAM->GetSize();
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
    STARFISH_ASSERT(name != nullptr);
    WRITER->String(name, static_cast<rapidjson::SizeType>(strlen(name)));
    return *this;
}

bool JsonWriter::HasMember(const char*) const
{
    // This function is for JsonReader only.
    STARFISH_ASSERT(false);
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

JsonWriter& JsonWriter::operator&(size_t& u)
{
    WRITER->Uint64(u);
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
    if (s != nullptr) {
        WRITER->String(s->toUTF8NonGCString().c_str(),
                       static_cast<rapidjson::SizeType>(s->contentLength()));
    } else {
        WRITER->String("", static_cast<rapidjson::SizeType>(0));
    }

    return *this;
}

JsonWriter& JsonWriter::operator&(
    std::unordered_map<std::string, std::string>& m)
{
    StartObject();

    for (auto value : m) {
        WRITER->Key(value.first.c_str());
        WRITER->String(value.second.c_str(),
                       static_cast<rapidjson::SizeType>(value.second.size()));
    }

    EndObject();

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
