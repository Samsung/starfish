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

#define LOCALSTORAGE "localstorage"
#define PROTOCOL "protocol"
#define HOST "host"
#define PORT "port"
#define ITEMS "items"
#define KEY "key"
#define VALUE "value"

typedef rapidjson::GenericStringBuffer<rapidjson::UTF8<>> JosnStringBuffer;
typedef rapidjson::GenericDocument<rapidjson::UTF8<>> JsonDocument;
typedef rapidjson::GenericValue<rapidjson::UTF8<>> JsonValue;

namespace StarFish {

JsonValue::ValueIterator jsonGetSecurity(
    JsonValue& root, SecurityOriginData* securityOriginData);
JsonValue::ValueIterator jsonGetItem(JsonValue::ValueIterator& root,
                                     String* key);

JsonValue jsonMakeItem(JsonDocument::AllocatorType& alloactor, String* key,
                       String* value);
JsonValue jsonMakeSecurity(JsonDocument::AllocatorType& alloactor,
                           SecurityOriginData* securityOriginData);

StorageManager::StorageManager(String* localStoragePath)
    : m_localStoragePath(localStoragePath)
{
    m_jsonHolder.m_ptr = nullptr;
    m_jsonHolder.m_ptr = new JsonDocument();
    jsonDocumentRead();
    GC_REGISTER_FINALIZER_NO_ORDER(this,
                                   [](void* obj, void* cd) {
                                       // STARFISH_LOG_INFO("StorageManager::~StorageManager\n");
                                       StorageManager* mgr =
                                           (StorageManager*)obj;
                                       mgr->jsonDocumentWrite();
                                       mgr = nullptr;

                                       JsonDocument* document =
                                           ((JsonDocument*)cd);
                                       delete (document);
                                   },
                                   m_jsonHolder.m_ptr, NULL, NULL);
}

Nullable<String*> StorageManager::key(SecurityOriginData* securityOriginData,
                                      unsigned long index)
{
    JsonDocument* document = ((JsonDocument*)m_jsonHolder.m_ptr);
    if (!(document->IsObject() && document->HasMember(LOCALSTORAGE))) {
        return nullptr;
    }
    JsonValue& root = (*document)[LOCALSTORAGE];
    JsonValue::ValueIterator itrSecurity =
        jsonGetSecurity(root, securityOriginData);
    if (itrSecurity == root.End()) {
        return nullptr;
    }
    if (index >= (*itrSecurity)[ITEMS].Size()) {
        return nullptr;
    }
    return String::fromUTF8((*itrSecurity)[ITEMS][index][KEY].GetString());
}

Nullable<String*> StorageManager::getItem(
    SecurityOriginData* securityOriginData, String* key)
{
    JsonDocument* document = ((JsonDocument*)m_jsonHolder.m_ptr);
    if (!(document->IsObject() && document->HasMember(LOCALSTORAGE))) {
        return nullptr;
    }
    JsonValue& root = (*document)[LOCALSTORAGE];
    JsonValue::ValueIterator itrSecurity =
        jsonGetSecurity(root, securityOriginData);
    if (itrSecurity == root.End()) {
        return nullptr;
    }
    JsonValue::ValueIterator itrItem = jsonGetItem(itrSecurity, key);
    if (itrItem == (*itrSecurity)[ITEMS].End()) {
        return nullptr;
    }
    return String::fromUTF8((*itrItem)[VALUE].GetString());
}

GCUnorderedMap<String*, String*>* StorageManager::getItems(
    SecurityOriginData* securityOriginData)
{
    GCUnorderedMap<String*, String*>* ret =
        new (GC) GCUnorderedMap<String*, String*>();
    JsonDocument* document = ((JsonDocument*)m_jsonHolder.m_ptr);
    if (!(document->IsObject() && document->HasMember(LOCALSTORAGE))) {
        return ret;
    }
    JsonValue& root = (*document)[LOCALSTORAGE];
    JsonValue::ValueIterator itrSecurity =
        jsonGetSecurity(root, securityOriginData);
    if (itrSecurity == root.End()) {
        return ret;
    }
    for (auto itr = (*itrSecurity)[ITEMS].Begin();
         itr != (*itrSecurity)[ITEMS].End(); ++itr) {
        ret->insert(std::pair<String*, String*>(
            String::fromUTF8((*itr)[KEY].GetString()),
            String::fromUTF8((*itr)[VALUE].GetString())));
    }
    return ret;
}

void StorageManager::setItem(SecurityOriginData* securityOriginData,
                             String* key, String* value)
{
    JsonDocument* document = ((JsonDocument*)m_jsonHolder.m_ptr);
    JsonDocument::AllocatorType& alloactor = document->GetAllocator();
    if (!(document->IsObject() && document->HasMember(LOCALSTORAGE))) {
        document->SetObject();
        JsonValue item = jsonMakeItem(alloactor, key, value);
        JsonValue itemArray(rapidjson::kArrayType);
        itemArray.PushBack(item, alloactor);
        JsonValue security = jsonMakeSecurity(alloactor, securityOriginData);
        security.AddMember(ITEMS, itemArray, alloactor);
        JsonValue securtiyArray(rapidjson::kArrayType);
        securtiyArray.PushBack(security, alloactor);
        (*document).AddMember(LOCALSTORAGE, securtiyArray, alloactor);
        jsonDocumentWrite();
        return;
    }
    JsonValue& root = (*document)[LOCALSTORAGE];
    JsonValue::ValueIterator itrSecurity =
        jsonGetSecurity(root, securityOriginData);
    if (itrSecurity == root.End()) {
        JsonValue item = jsonMakeItem(alloactor, key, value);
        JsonValue itemArray(rapidjson::kArrayType);
        itemArray.PushBack(item, alloactor);
        JsonValue security = jsonMakeSecurity(alloactor, securityOriginData);
        security.AddMember(ITEMS, itemArray, alloactor);
        root.PushBack(security, alloactor);
        jsonDocumentWrite();
        return;
    }
    JsonValue::ValueIterator itrItem = jsonGetItem(itrSecurity, key);
    if (itrItem == (*itrSecurity)[ITEMS].End()) {
        JsonValue json_item = jsonMakeItem(alloactor, key, value);
        (*itrSecurity)[ITEMS].PushBack(json_item, alloactor);
        jsonDocumentWrite();
        return;
    }
    auto s = value->toUTF8NonGCString();
    (*itrItem)[VALUE].SetString(s.data(), s.length());
    jsonDocumentWrite();
}

void StorageManager::removeItem(SecurityOriginData* securityOriginData,
                                String* key)
{
    JsonDocument* document = ((JsonDocument*)m_jsonHolder.m_ptr);
    if (!(document->IsObject() && document->HasMember(LOCALSTORAGE))) {
        return;
    }
    JsonValue& root = (*document)[LOCALSTORAGE];
    JsonValue::ValueIterator itrSecurity =
        jsonGetSecurity(root, securityOriginData);
    if (itrSecurity == root.End()) {
        return;
    }
    JsonValue::ValueIterator itrItem = jsonGetItem(itrSecurity, key);
    if (itrItem == (*itrSecurity)[ITEMS].End()) {
        return;
    }
    (*itrSecurity)[ITEMS].Erase(itrItem);
}

void StorageManager::clear(SecurityOriginData* securityOriginData)
{
    JsonDocument* document = ((JsonDocument*)m_jsonHolder.m_ptr);
    if (!(document->IsObject() && document->HasMember(LOCALSTORAGE))) {
        return;
    }
    JsonValue& root = (*document)[LOCALSTORAGE];
    JsonValue::ValueIterator itrSecurity =
        jsonGetSecurity(root, securityOriginData);
    if (itrSecurity == root.End()) {
        return;
    }
    root.Erase(itrSecurity);
}

unsigned long StorageManager::length(SecurityOriginData* securityOriginData)
{
    JsonDocument* document = (JsonDocument*)m_jsonHolder.m_ptr;
    if (!(document->IsObject() && document->HasMember(LOCALSTORAGE))) {
        return 0;
    }
    JsonValue& root = (*document)[LOCALSTORAGE];
    JsonValue::ValueIterator itrSecurity =
        jsonGetSecurity(root, securityOriginData);
    if (itrSecurity == root.End()) {
        return 0;
    }
    return (*itrSecurity)[ITEMS].Size();
}

void StorageManager::jsonDocumentRead()
{
    JsonDocument* document = ((JsonDocument*)m_jsonHolder.m_ptr);
    document->SetObject();
    FileIO* m_fileIO = FileIO::create();
    bool canLoad = m_fileIO->open(m_localStoragePath, ReadWrite);
    if (canLoad == true) {
        String* filedata = m_fileIO->readAll();
        m_fileIO->close();
        auto s = filedata->toUTF8NonGCString();
        document->Parse(s.data());
    } else {
        m_fileIO->open(m_localStoragePath, ReadWrite);
        m_fileIO->close();
    }
}

void StorageManager::jsonDocumentWrite()
{
    JosnStringBuffer buffer;
    buffer.Clear();
    rapidjson::Writer<JosnStringBuffer> writer(buffer);
    JsonDocument* document = ((JsonDocument*)m_jsonHolder.m_ptr);
    document->Accept(writer);

    FileIO* m_fileIO = FileIO::create();
    bool canLoad = m_fileIO->open(m_localStoragePath, Write);
    if (canLoad == true) {
        m_fileIO->write((void*)buffer.GetString(), 1, buffer.GetSize());
        m_fileIO->close();
    }
}

JsonValue jsonMakeItem(JsonDocument::AllocatorType& alloactor, String* key,
                       String* value)
{
    JsonValue ret(rapidjson::kObjectType);
    JsonValue v1, v2;
    auto v = key->toUTF8NonGCString();
    v1.SetString(v.data(), v.length(), alloactor);
    ret.AddMember(KEY, v1, alloactor);
    v = value->toUTF8NonGCString();
    v2.SetString(v.data(), v.length(), alloactor);
    ret.AddMember(VALUE, v2, alloactor);
    return ret;
}

JsonValue jsonMakeSecurity(JsonDocument::AllocatorType& alloactor,
                           SecurityOriginData* securityOriginData)
{
    JsonValue ret(rapidjson::kObjectType);
    JsonValue v1, v2;
    auto v = securityOriginData->protocol()->toUTF8NonGCString();
    v1.SetString(v.data(), v.length(), alloactor);
    ret.AddMember(PROTOCOL, v1, alloactor);
    v = securityOriginData->host()->toUTF8NonGCString();
    v2.SetString(v.data(), v.length(), alloactor);
    ret.AddMember(HOST, v2, alloactor);
    ret.AddMember(PORT, securityOriginData->port(), alloactor);
    return ret;
}

JsonValue::ValueIterator jsonGetSecurity(JsonValue& root,
                                         SecurityOriginData* securityOriginData)
{
    for (auto itr = root.Begin(); itr != root.End(); ++itr) {
        auto v1 = securityOriginData->protocol()->toUTF8NonGCString();
        auto v2 = securityOriginData->host()->toUTF8NonGCString();
        if ((*itr)[PROTOCOL] == v1.data() && (*itr)[HOST] == v2.data() &&
            (*itr)[PORT] == securityOriginData->port()) {
            return itr;
        }
    }
    return root.End();
}

JsonValue::ValueIterator jsonGetItem(JsonValue::ValueIterator& root,
                                     String* key)
{
    for (auto itr = (*root)[ITEMS].Begin(); itr != (*root)[ITEMS].End();
         ++itr) {
        auto v = key->toUTF8NonGCString();
        if ((*itr)[KEY] == v.data()) {
            return itr;
        }
    }
    return (*root)[ITEMS].End();
}
}
