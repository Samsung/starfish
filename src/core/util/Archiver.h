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
#ifndef __StarfishArchiver__
#define __StarfishArchiver__

namespace Starfish {

class String;

template <typename T>
constexpr typename std::underlying_type<T>::type toUnderlyingType(T value)
{
    return static_cast<typename std::underlying_type<T>::type>(value);
}

class Archiver {
public:
    virtual ~Archiver(){};
    virtual operator bool() const = 0;
    virtual Archiver& StartObject() = 0;
    virtual Archiver& Member(const char* name) = 0;
    virtual bool HasMember(const char* name) const = 0;
    virtual Archiver& EndObject() = 0;
    virtual Archiver& StartArray(size_t* size = 0) = 0;
    virtual Archiver& EndArray() = 0;
    virtual Archiver& operator&(bool& b) = 0;
    virtual Archiver& operator&(unsigned& u) = 0;
    virtual Archiver& operator&(int& i) = 0;
    virtual Archiver& operator&(double& d) = 0;
    virtual Archiver& operator&(std::string& s) = 0;
    virtual Archiver& operator&(String*& s) = 0;
    virtual Archiver& SetNull() = 0;
    virtual bool IsReader() = 0;

    // archivers for template types
    template <typename T>
    void MemberId(const char* name, Id<T>& id)
    {
        Member(name) & id.m_id;
    }

    template <typename T>
    void MemberEnum(const char* name, T& enumValue)
    {
        unsigned enumNumber = 0;
        if (IsReader()) {
            Member(name) & enumNumber;
            enumValue = static_cast<T>(enumNumber);
        } else {
            enumNumber = toUnderlyingType(enumValue);
            Member(name) & enumNumber;
        }
    }
};

class JsonReader : public Archiver {
public:
    JsonReader(const char* json);
    virtual ~JsonReader();

    operator bool() const override
    {
        return !mError;
    }

    JsonReader& StartObject() override;
    JsonReader& Member(const char* name) override;
    bool HasMember(const char* name) const override;
    JsonReader& EndObject() override;
    JsonReader& StartArray(size_t* size = 0) override;
    JsonReader& EndArray() override;
    JsonReader& operator&(bool& b)override;
    JsonReader& operator&(unsigned& u)override;
    JsonReader& operator&(int& i)override;
    JsonReader& operator&(double& d)override;
    JsonReader& operator&(std::string& s)override;
    JsonReader& operator&(String*& s)override;
    JsonReader& SetNull() override;

    bool IsReader() override
    {
        return true;
    };

private:
    JsonReader(const JsonReader&);
    JsonReader& operator=(const JsonReader&);

    void Next();

    void* mDocument;
    void* mStack;
    bool mError;
};

class JsonWriter : public Archiver {
public:
    JsonWriter();
    virtual ~JsonWriter();

    const char* GetString() const;
    size_t GetSize() const;

    operator bool() const override
    {
        return true;
    }

    JsonWriter& StartObject() override;
    JsonWriter& Member(const char* name) override;
    bool HasMember(const char* name) const override;
    JsonWriter& EndObject() override;
    JsonWriter& StartArray(size_t* size = 0) override;
    JsonWriter& EndArray() override;
    JsonWriter& operator&(bool& b)override;
    JsonWriter& operator&(unsigned& u)override;
    JsonWriter& operator&(int& i)override;
    JsonWriter& operator&(double& d)override;
    JsonWriter& operator&(std::string& s)override;
    JsonWriter& operator&(String*& s)override;
    JsonWriter& SetNull() override;

    bool IsReader() override
    {
        return false;
    };

private:
    JsonWriter(const JsonWriter&);
    JsonWriter& operator=(const JsonWriter&);

    void* mWriter;
    void* mStream;
};

} // namespace Starfish

#endif
#endif
