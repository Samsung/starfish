/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "StorageManager.h"
#include "core/dom/WebOrigin.h"
#include "platform/file/File.h"

#include "rapidjson/writer.h"
#include "rapidjson/encodings.h"

typedef rapidjson::GenericStringBuffer<rapidjson::UTF8<>> JosnStringBuffer;
typedef rapidjson::GenericValue<rapidjson::UTF8<>> JsonValue;

namespace Starfish {

StorageManager::StorageManager(String* localStoragePath)
    : m_localStoragePath(localStoragePath)
    , m_jsonDocument(new JsonDocument())
{
    m_jsonDocument->SetObject();
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            StorageManager* mgr = (StorageManager*)obj;
            mgr->writeJsonDocumentAsFile();
            delete mgr->m_jsonDocument;
        },
        NULL, NULL, NULL);
}

Nullable<String*> StorageManager::getItem(WebOrigin* webOrigin, String* key)
{
    auto serializedOrigin = webOrigin->serialize()->toUTF8NonGCString();
    auto& allocator = m_jsonDocument->GetAllocator();

    JsonValue::MemberIterator it1 =
        m_jsonDocument->FindMember(serializedOrigin.data());
    if (it1 != m_jsonDocument->MemberEnd()) {
        auto& o = it1->value;
        auto utf8Key = key->toUTF8NonGCString();
        JsonValue k(utf8Key.data(), utf8Key.length(), allocator);
        JsonValue::MemberIterator it2 = o.FindMember(k);
        if (it2 != o.MemberEnd()) {
            return String::fromUTF8(it2->value.GetString(),
                                    it2->value.GetStringLength());
        }
    }
    return nullptr;
}

void StorageManager::load(GCUnorderedMap<String*, String*>& out,
                          WebOrigin* webOrigin)
{
    loadFromFileToJsonDocument();

    auto serializedOrigin = webOrigin->serialize()->toUTF8NonGCString();

    JsonValue::MemberIterator it1 =
        m_jsonDocument->FindMember(serializedOrigin.data());
    if (it1 != m_jsonDocument->MemberEnd()) {
        auto& o = it1->value;
        for (JsonValue::MemberIterator it2 = o.MemberBegin();
             it2 != o.MemberEnd(); ++it2) {
            out.insert(std::pair<String*, String*>(
                String::fromUTF8(it2->name.GetString(),
                                 it2->name.GetStringLength()),
                String::fromUTF8(it2->value.GetString(),
                                 it2->value.GetStringLength())));
        }
    }
}

void StorageManager::setItem(WebOrigin* webOrigin, String* key, String* value)
{
    auto serializedOrigin = webOrigin->serialize()->toUTF8NonGCString();
    auto& allocator = m_jsonDocument->GetAllocator();
    auto utf8Key = key->toUTF8NonGCString();
    auto utf8Value = value->toUTF8NonGCString();

    JsonValue k(utf8Key.data(), utf8Key.length(), allocator);
    JsonValue v(utf8Value.data(), utf8Value.length(), allocator);

    JsonValue::MemberIterator it1 =
        m_jsonDocument->FindMember(serializedOrigin.data());
    if (it1 != m_jsonDocument->MemberEnd()) {
        auto& o = it1->value;
        JsonValue::MemberIterator it2 = o.FindMember(k);
        if (it2 != o.MemberEnd()) {
            it2->value = v;
        } else {
            o.AddMember(k, v, allocator);
        }
    } else {
        JsonValue s(serializedOrigin.data(), allocator);
        JsonValue o(rapidjson::kObjectType);
        o.AddMember(k, v, allocator);
        m_jsonDocument->AddMember(s, o, allocator);
    }
    writeJsonDocumentAsFile();
}

void StorageManager::removeItem(WebOrigin* webOrigin, String* key)
{
    auto serializedOrigin = webOrigin->serialize()->toUTF8NonGCString();
    auto& allocator = m_jsonDocument->GetAllocator();
    auto utf8Key = key->toUTF8NonGCString();
    JsonValue k(utf8Key.data(), utf8Key.length(), allocator);

    JsonValue::MemberIterator it =
        m_jsonDocument->FindMember(serializedOrigin.data());
    if (it != m_jsonDocument->MemberEnd()) {
        it->value.RemoveMember(k);
    }
    writeJsonDocumentAsFile();
}

void StorageManager::clear(WebOrigin* webOrigin)
{
    auto serializedOrigin = webOrigin->serialize()->toUTF8NonGCString();

    JsonValue::MemberIterator it =
        m_jsonDocument->FindMember(serializedOrigin.data());
    if (it != m_jsonDocument->MemberEnd()) {
        it->value.RemoveAllMembers();
    }
    writeJsonDocumentAsFile();
}

unsigned long StorageManager::size(WebOrigin* webOrigin)
{
    auto serializedOrigin = webOrigin->serialize()->toUTF8NonGCString();

    JsonValue::MemberIterator it =
        m_jsonDocument->FindMember(serializedOrigin.data());
    if (it != m_jsonDocument->MemberEnd()) {
        return it->value.MemberCount();
    }
    return 0;
}

void StorageManager::loadFromFileToJsonDocument()
{
    auto fileIO = File::open(m_localStoragePath, File::Read);
    if (fileIO) {
        Nullable<String*> filedata = fileIO->readAll();
        if (filedata.hasValue()) {
            auto s = filedata.getValue()->toUTF8NonGCString();
            m_jsonDocument->Parse(s.data());
        }
    }
    if (!m_jsonDocument->IsObject()) {
        JsonDocument().Swap(*m_jsonDocument);
        m_jsonDocument->SetObject();
    }
}

void StorageManager::writeJsonDocumentAsFile()
{
    JosnStringBuffer buffer;
    buffer.Clear();
    rapidjson::Writer<JosnStringBuffer> writer(buffer);
    m_jsonDocument->Accept(writer);

    auto fileIO = File::open(m_localStoragePath, File::Write);
    if (fileIO) {
        fileIO->write((void*)buffer.GetString(), 1, buffer.GetSize());
    }
}
} // namespace Starfish
