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
#include "core/storage/StoragePersistent.h"

#include "core/dom/WebOrigin.h"
#include "platform/file/PlatformFile.h"

#include "rapidjson/writer.h"
#include "rapidjson/encodings.h"

typedef rapidjson::GenericStringBuffer<rapidjson::UTF8<>> JosnStringBuffer;
typedef rapidjson::GenericValue<rapidjson::UTF8<>> JsonValue;

namespace Starfish {

StoragePersistent::StoragePersistent(StorageType storageType,
                                     WebOrigin* webOrigin,
                                     String* localStoragePath)
    : StorageInternal(storageType, webOrigin)
{
    m_diskWriter = new StorageDiskWriter(localStoragePath);
    m_diskWriter->load(m_cache, m_webOrigin);
}

unsigned long StoragePersistent::length()
{
    return m_cache.size();
}

Nullable<String*> StoragePersistent::key(unsigned long index)
{
    if (index >= m_cache.size()) {
        return nullptr;
    }
    auto itr = std::next(m_cache.begin(), index);
    return itr->first;
}

Nullable<String*> StoragePersistent::getItem(String* key)
{
    auto itr = m_cache.find(key);
    if (itr == m_cache.end()) {
        return m_diskWriter->getItem(m_webOrigin, key);
    }

    return itr->second;
}

GCVector<String*> StoragePersistent::getKeyNames()
{
    GCVector<String*> ret;

    auto iter = m_cache.begin();
    while (iter != m_cache.end()) {
        ret.push_back(iter->first);
        iter++;
    }

    return ret;
}

bool StoragePersistent::setItem(String* key, String* value)
{
    auto iter = m_cache.find(key);
    if (iter == m_cache.end()) {
        m_cache.insert(std::make_pair(key, value));
    } else {
        iter.value() = value;
    }

    m_diskWriter->setItem(m_webOrigin, key, value);

    return true;
}

bool StoragePersistent::removeItem(String* key)
{
    m_cache.erase(key);
    m_diskWriter->removeItem(m_webOrigin, key);

    return true;
}

void StoragePersistent::clear()
{
    m_cache.clear();
    m_diskWriter->clear(m_webOrigin);
}

StorageDiskWriter::StorageDiskWriter(String* localStoragePath)
    : m_localStoragePath(localStoragePath)
    , m_jsonDocument(new JsonDocument())
{
    m_jsonDocument->SetObject();
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            StorageDiskWriter* mgr = (StorageDiskWriter*)obj;
            mgr->writeJsonDocumentAsFile();
            delete mgr->m_jsonDocument;
        },
        NULL, NULL, NULL);
}

Nullable<String*> StorageDiskWriter::getItem(WebOrigin* webOrigin, String* key)
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

void StorageDiskWriter::load(GCUnorderedMap<String*, String*>& out,
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

void StorageDiskWriter::setItem(WebOrigin* webOrigin, String* key,
                                String* value)
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

void StorageDiskWriter::removeItem(WebOrigin* webOrigin, String* key)
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

void StorageDiskWriter::clear(WebOrigin* webOrigin)
{
    auto serializedOrigin = webOrigin->serialize()->toUTF8NonGCString();

    JsonValue::MemberIterator it =
        m_jsonDocument->FindMember(serializedOrigin.data());
    if (it != m_jsonDocument->MemberEnd()) {
        it->value.RemoveAllMembers();
    }
    writeJsonDocumentAsFile();
}

unsigned long StorageDiskWriter::size(WebOrigin* webOrigin)
{
    auto serializedOrigin = webOrigin->serialize()->toUTF8NonGCString();

    JsonValue::MemberIterator it =
        m_jsonDocument->FindMember(serializedOrigin.data());
    if (it != m_jsonDocument->MemberEnd()) {
        return it->value.MemberCount();
    }
    return 0;
}

void StorageDiskWriter::loadFromFileToJsonDocument()
{
    auto fileIO = PlatformFile::open(m_localStoragePath, PlatformFile::Read);
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

void StorageDiskWriter::writeJsonDocumentAsFile()
{
    JosnStringBuffer buffer;
    buffer.Clear();
    rapidjson::Writer<JosnStringBuffer> writer(buffer);
    m_jsonDocument->Accept(writer);

    auto fileIO = PlatformFile::open(m_localStoragePath, PlatformFile::Write);
    if (fileIO) {
        fileIO->write((void*)buffer.GetString(), 1, buffer.GetSize());
    }
}
} // namespace Starfish
