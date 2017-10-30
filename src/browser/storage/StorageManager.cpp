/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "StorageManager.h"
#include "core/page/SecurityOriginData.h"
#include "platform/file/FileIO.h"

#include "../third_party/rapidjson/include/rapidjson/document.h"
#include "../third_party/rapidjson/include/rapidjson/stringbuffer.h"
#include "../third_party/rapidjson/include/rapidjson/writer.h"
#include "../third_party/rapidjson/include/rapidjson/encodings.h"

#define PROTOCOL "protocol"
#define HOST "host"
#define PORT "port"
#define KEY "key"
#define VALUE "value"

typedef rapidjson::GenericStringBuffer<rapidjson::UTF8<>> JosnStringBuffer;
typedef rapidjson::GenericDocument<rapidjson::UTF8<>> JsonDocument;
typedef rapidjson::GenericValue<rapidjson::UTF8<>> JsonValue;

namespace StarFish {

StorageManager::StorageManager(String* localStoragePath)
    : m_localStoragePath(localStoragePath)
    , m_jsonHolder(new JsonDocument())
{
    JsonDocument* root = (JsonDocument*)m_jsonHolder;
    root->SetNull();

    jsonDocumentRead();
    GC_REGISTER_FINALIZER_NO_ORDER(this,
                                   [](void* obj, void* cd) {
                                       // STARFISH_LOG_INFO("StorageManager::~StorageManager\n");
                                       StorageManager* mgr =
                                           (StorageManager*)obj;
                                       mgr->jsonDocumentWrite();
                                       mgr = nullptr;
                                       JsonDocument* root = (JsonDocument*)cd;
                                       if (root) {
                                           delete root;
                                           root = nullptr;
                                       }
                                   },
                                   m_jsonHolder, NULL, NULL);
}

Nullable<String*> StorageManager::key(SecurityOriginData* securityOriginData,
                                      unsigned long index)
{
    JsonDocument* root = (JsonDocument*)m_jsonHolder;
    STARFISH_ASSERT(root->IsArray());

    unsigned long securityOriginCount = 0;
    for (auto itr = root->Begin(); itr != root->End(); ++itr) {
        STARFISH_ASSERT(itr->IsObject());
        auto v1 = securityOriginData->protocol()->toUTF8NonGCString();
        auto v2 = securityOriginData->host()->toUTF8NonGCString();
        auto v3 = securityOriginData->port();
        if ((*itr)[PROTOCOL] == v1.data() && (*itr)[HOST] == v2.data() &&
            (*itr)[PORT] == v3) {
            if (securityOriginCount == index) {
                return String::fromUTF8((*itr)[KEY].GetString(),
                                        (*itr)[KEY].GetStringLength());
            }
            securityOriginCount++;
        }
    }
    return nullptr;
}

Nullable<String*> StorageManager::getItem(
    SecurityOriginData* securityOriginData, String* key)
{
    JsonDocument* root = (JsonDocument*)m_jsonHolder;
    STARFISH_ASSERT(root->IsArray());

    for (auto itr = root->Begin(); itr != root->End(); ++itr) {
        STARFISH_ASSERT(itr->IsObject());
        auto v1 = securityOriginData->protocol()->toUTF8NonGCString();
        auto v2 = securityOriginData->host()->toUTF8NonGCString();
        auto v3 = securityOriginData->port();
        auto v4 = key->toUTF8NonGCString();
        if ((*itr)[PROTOCOL] == v1.data() && (*itr)[HOST] == v2.data() &&
            (*itr)[PORT] == v3 && (*itr)[KEY] == v4.data()) {
            return String::fromUTF8((*itr)[VALUE].GetString(),
                                    (*itr)[VALUE].GetStringLength());
        }
    }
    return nullptr;
}

GCUnorderedMap<String*, String*>* StorageManager::getItems(
    SecurityOriginData* securityOriginData)
{
    GCUnorderedMap<String*, String*>* ret =
        new (GC) GCUnorderedMap<String*, String*>();
    JsonDocument* root = (JsonDocument*)m_jsonHolder;
    STARFISH_ASSERT(root->IsArray());

    for (auto itr = root->Begin(); itr != root->End(); ++itr) {
        STARFISH_ASSERT(itr->IsObject());
        auto v1 = securityOriginData->protocol()->toUTF8NonGCString();
        auto v2 = securityOriginData->host()->toUTF8NonGCString();
        auto v3 = securityOriginData->port();
        if ((*itr)[PROTOCOL] == v1.data() && (*itr)[HOST] == v2.data() &&
            (*itr)[PORT] == v3) {
            ret->insert(std::pair<String*, String*>(
                String::fromUTF8((*itr)[KEY].GetString(),
                                 (*itr)[KEY].GetStringLength()),
                String::fromUTF8((*itr)[VALUE].GetString(),
                                 (*itr)[VALUE].GetStringLength())));
        }
    }
    return ret;
}

void StorageManager::setItem(SecurityOriginData* securityOriginData,
                             String* key, String* value)
{
    JsonDocument* root = (JsonDocument*)m_jsonHolder;
    STARFISH_ASSERT(root->IsArray());
    JsonDocument::AllocatorType& alloactor = root->GetAllocator();

    for (auto itr = root->Begin(); itr != root->End(); ++itr) {
        STARFISH_ASSERT(itr->IsObject());
        auto v1 = securityOriginData->protocol()->toUTF8NonGCString();
        auto v2 = securityOriginData->host()->toUTF8NonGCString();
        auto v3 = securityOriginData->port();
        auto v4 = key->toUTF8NonGCString();
        if ((*itr)[PROTOCOL] == v1.data() && (*itr)[HOST] == v2.data() &&
            (*itr)[PORT] == v3 && (*itr)[KEY] == v4.data()) {
            auto s = value->toUTF8NonGCString();
            (*itr)[VALUE].SetString(s.data(), s.length(), alloactor);
            jsonDocumentWrite();
            return;
        }
    }

    JsonValue makeSetItem(rapidjson::kObjectType);
    JsonValue jsonValue1, jsonValue2, jsonValue3, jsonValue4;
    auto v = securityOriginData->protocol()->toUTF8NonGCString();
    jsonValue1.SetString(v.data(), v.length(), alloactor);
    v = securityOriginData->host()->toUTF8NonGCString();
    jsonValue2.SetString(v.data(), v.length(), alloactor);
    v = key->toUTF8NonGCString();
    jsonValue3.SetString(v.data(), v.length(), alloactor);
    v = value->toUTF8NonGCString();
    jsonValue4.SetString(v.data(), v.length(), alloactor);

    makeSetItem.AddMember(PROTOCOL, jsonValue1, alloactor);
    makeSetItem.AddMember(HOST, jsonValue2, alloactor);
    makeSetItem.AddMember(PORT, securityOriginData->port(), alloactor);
    makeSetItem.AddMember(KEY, jsonValue3, alloactor);
    makeSetItem.AddMember(VALUE, jsonValue4, alloactor);
    root->PushBack(makeSetItem, alloactor);
    jsonDocumentWrite();
}

void StorageManager::removeItem(SecurityOriginData* securityOriginData,
                                String* key)
{
    JsonDocument* root = (JsonDocument*)m_jsonHolder;
    STARFISH_ASSERT(root->IsArray());

    for (auto itr = root->Begin(); itr != root->End(); ++itr) {
        STARFISH_ASSERT(itr->IsObject());
        auto v1 = securityOriginData->protocol()->toUTF8NonGCString();
        auto v2 = securityOriginData->host()->toUTF8NonGCString();
        auto v3 = securityOriginData->port();
        auto v4 = key->toUTF8NonGCString();
        if ((*itr)[PROTOCOL] == v1.data() && (*itr)[HOST] == v2.data() &&
            (*itr)[PORT] == v3 && (*itr)[KEY] == v4.data()) {
            root->Erase(itr);
            jsonDocumentWrite();
            return;
        }
    }
}

void StorageManager::clear(SecurityOriginData* securityOriginData)
{
    JsonDocument* root = (JsonDocument*)m_jsonHolder;
    STARFISH_ASSERT(root->IsArray());

    for (auto itr = root->Begin(); itr != root->End();) {
        STARFISH_ASSERT(itr->IsObject());
        auto v1 = securityOriginData->protocol()->toUTF8NonGCString();
        auto v2 = securityOriginData->host()->toUTF8NonGCString();
        auto v3 = securityOriginData->port();
        if ((*itr)[PROTOCOL] == v1.data() && (*itr)[HOST] == v2.data() &&
            (*itr)[PORT] == v3) {
            itr = root->Erase(itr);
        } else {
            ++itr;
        }
    }
    jsonDocumentWrite();
}

unsigned long StorageManager::length(SecurityOriginData* securityOriginData)
{
    JsonDocument* root = (JsonDocument*)m_jsonHolder;
    STARFISH_ASSERT(root->IsArray());

    unsigned long securityOriginCount = 0;
    for (auto itr = root->Begin(); itr != root->End(); ++itr) {
        STARFISH_ASSERT(itr->IsObject());
        auto v1 = securityOriginData->protocol()->toUTF8NonGCString();
        auto v2 = securityOriginData->host()->toUTF8NonGCString();
        auto v3 = securityOriginData->port();
        if ((*itr)[PROTOCOL] == v1.data() && (*itr)[HOST] == v2.data() &&
            (*itr)[PORT] == v3) {
            securityOriginCount++;
        }
    }
    return securityOriginCount;
}

void StorageManager::jsonDocumentRead()
{
    JsonDocument* root = (JsonDocument*)m_jsonHolder;
    FileIO* m_fileIO = FileIO::create();
    bool canLoad = m_fileIO->open(m_localStoragePath, Read);
    if (canLoad == true) {
        String* filedata = m_fileIO->readAll();
        auto s = filedata->toUTF8NonGCString();
        root->Parse(s.data());
    }
    m_fileIO->close();

    if (root->GetType() != rapidjson::kArrayType) {
        root->SetArray();
    }
}

void StorageManager::jsonDocumentWrite()
{
    JosnStringBuffer buffer;
    buffer.Clear();
    rapidjson::Writer<JosnStringBuffer> writer(buffer);
    JsonDocument* root = (JsonDocument*)m_jsonHolder;
    STARFISH_ASSERT(root->IsArray());
    root->Accept(writer);

    FileIO* m_fileIO = FileIO::create();
    bool canLoad = m_fileIO->open(m_localStoragePath, Write);
    if (canLoad == true) {
        m_fileIO->write((void*)buffer.GetString(), 1, buffer.GetSize());
    }
    m_fileIO->close();
}
}
