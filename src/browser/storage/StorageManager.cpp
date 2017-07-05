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

#define LOCALSTORAGE "localstorage"
#define PROTOCOL "protocol"
#define HOST "host"
#define PORT "port"
#define ITEMS "items"
#define KEY "key"
#define VALUE "value"

namespace StarFish {

rapidjson::Value::ValueIterator jsonGetSecurity(
    rapidjson::Value& root, SecurityOriginData* securityOriginData);
rapidjson::Value::ValueIterator jsonGetItems(
    rapidjson::Value::ValueIterator& root, String* key);
rapidjson::Value jsonMakeItem(rapidjson::Document::AllocatorType& alloactor,
                              String* key, String* value);
rapidjson::Value jsonMakeSecurity(rapidjson::Document::AllocatorType& alloactor,
                                  SecurityOriginData* securityOriginData);

StorageManager::StorageManager(String* localStoragePath)
    : m_localStoragePath(localStoragePath)
{
    m_jsonHolder.m_ptr = nullptr;
    m_jsonHolder.m_ptr = new rapidjson::Document();
    jsonDocumentRead();
    GC_REGISTER_FINALIZER_NO_ORDER(this,
                                   [](void* obj, void* cd) {
                                       // STARFISH_LOG_INFO("StorageManager::~StorageManager\n");
                                       StorageManager* mgr =
                                           (StorageManager*)obj;
                                       mgr->jsonDocumentWrite();
                                       mgr = nullptr;

                                       rapidjson::Document* document =
                                           ((rapidjson::Document*)cd);
                                       document->RemoveAllMembers();
                                       free(document);
                                   },
                                   m_jsonHolder.m_ptr, NULL, NULL);
}

Nullable<String*> StorageManager::key(SecurityOriginData* securityOriginData,
                                      unsigned long index)
{
    rapidjson::Document* document = ((rapidjson::Document*)m_jsonHolder.m_ptr);
    if (!(document->IsObject() && document->HasMember(LOCALSTORAGE))) {
        return nullptr;
    }
    rapidjson::Value& root = (*document)[LOCALSTORAGE];
    rapidjson::Value::ValueIterator itrSecurity =
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
    rapidjson::Document* document = ((rapidjson::Document*)m_jsonHolder.m_ptr);
    if (!(document->IsObject() && document->HasMember(LOCALSTORAGE))) {
        return nullptr;
    }
    rapidjson::Value& root = (*document)[LOCALSTORAGE];
    rapidjson::Value::ValueIterator itrSecurity =
        jsonGetSecurity(root, securityOriginData);
    if (itrSecurity == root.End()) {
        return nullptr;
    }
    rapidjson::Value::ValueIterator itrItem = jsonGetItems(itrSecurity, key);
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
    rapidjson::Document* document = ((rapidjson::Document*)m_jsonHolder.m_ptr);
    if (!(document->IsObject() && document->HasMember(LOCALSTORAGE))) {
        return ret;
    }
    rapidjson::Value& root = (*document)[LOCALSTORAGE];
    rapidjson::Value::ValueIterator itrSecurity =
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
    rapidjson::Document* document = ((rapidjson::Document*)m_jsonHolder.m_ptr);
    rapidjson::Document::AllocatorType& alloactor = document->GetAllocator();
    if (!(document->IsObject() && document->HasMember(LOCALSTORAGE))) {
        document->SetObject();
        rapidjson::Value items = jsonMakeItem(alloactor, key, value);
        rapidjson::Value items_ary(rapidjson::kArrayType);
        items_ary.PushBack(items, alloactor);
        rapidjson::Value security =
            jsonMakeSecurity(alloactor, securityOriginData);
        security.AddMember(ITEMS, items_ary, alloactor);
        rapidjson::Value securtiy_ary(rapidjson::kArrayType);
        securtiy_ary.PushBack(security, alloactor);
        (*document).AddMember(LOCALSTORAGE, securtiy_ary, alloactor);
        jsonDocumentWrite();
        return;
    }
    rapidjson::Value& root = (*document)[LOCALSTORAGE];
    rapidjson::Value::ValueIterator itrSecurity =
        jsonGetSecurity(root, securityOriginData);
    if (itrSecurity == root.End()) {
        rapidjson::Value items = jsonMakeItem(alloactor, key, value);
        rapidjson::Value items_ary(rapidjson::kArrayType);
        items_ary.PushBack(items, alloactor);
        rapidjson::Value security =
            jsonMakeSecurity(alloactor, securityOriginData);
        security.AddMember(ITEMS, items_ary, alloactor);
        root.PushBack(security, alloactor);
        jsonDocumentWrite();
        return;
    }
    rapidjson::Value::ValueIterator itrItem = jsonGetItems(itrSecurity, key);
    if (itrItem == (*itrSecurity)[ITEMS].End()) {
        rapidjson::Value json_item = jsonMakeItem(alloactor, key, value);
        (*itrSecurity)[ITEMS].PushBack(json_item, alloactor);
        jsonDocumentWrite();
        return;
    }
    (*itrItem)[VALUE].SetString(value->toUTF8NonGCString().data(),
                                value->length());
    jsonDocumentWrite();
}

void StorageManager::removeItem(SecurityOriginData* securityOriginData,
                                String* key)
{
    rapidjson::Document* document = ((rapidjson::Document*)m_jsonHolder.m_ptr);
    if (!(document->IsObject() && document->HasMember(LOCALSTORAGE))) {
        return;
    }
    rapidjson::Value& root = (*document)[LOCALSTORAGE];
    rapidjson::Value::ValueIterator itrSecurity =
        jsonGetSecurity(root, securityOriginData);
    if (itrSecurity == root.End()) {
        return;
    }
    rapidjson::Value::ValueIterator itrItem = jsonGetItems(itrSecurity, key);
    if (itrItem == (*itrSecurity)[ITEMS].End()) {
        return;
    }
    (*itrSecurity)[ITEMS].Erase(itrItem);
}

void StorageManager::clear(SecurityOriginData* securityOriginData)
{
    rapidjson::Document* document = ((rapidjson::Document*)m_jsonHolder.m_ptr);
    if (!(document->IsObject() && document->HasMember(LOCALSTORAGE))) {
        return;
    }
    rapidjson::Value& root = (*document)[LOCALSTORAGE];
    rapidjson::Value::ValueIterator itrSecurity =
        jsonGetSecurity(root, securityOriginData);
    if (itrSecurity == root.End()) {
        return;
    }
    root.Erase(itrSecurity);
}

unsigned long StorageManager::length(SecurityOriginData* securityOriginData)
{
    rapidjson::Document* document = (rapidjson::Document*)m_jsonHolder.m_ptr;
    if (!(document->IsObject() && document->HasMember(LOCALSTORAGE))) {
        return 0;
    }
    rapidjson::Value& root = (*document)[LOCALSTORAGE];
    rapidjson::Value::ValueIterator itrSecurity =
        jsonGetSecurity(root, securityOriginData);
    if (itrSecurity == root.End()) {
        return 0;
    }
    return (*itrSecurity)[ITEMS].Size();
}

void StorageManager::jsonDocumentRead()
{
    rapidjson::Document* document = ((rapidjson::Document*)m_jsonHolder.m_ptr);
    document->SetObject();
    FileIO* m_fileIO = FileIO::create();
    bool canLoad = m_fileIO->open(m_localStoragePath, Read);
    if (canLoad == true) {
        String* filedata = m_fileIO->readAll();
        m_fileIO->close();
        document->Parse(filedata->toUTF8NonGCString().data());
    } else {
        m_fileIO->open(m_localStoragePath, ReadWrite);
        m_fileIO->close();
    }
}

void StorageManager::jsonDocumentWrite()
{
    rapidjson::StringBuffer buffer;
    buffer.Clear();
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    rapidjson::Document* document = ((rapidjson::Document*)m_jsonHolder.m_ptr);
    document->Accept(writer);

    FileIO* m_fileIO = FileIO::create();
    m_fileIO->open(m_localStoragePath, Write);
    m_fileIO->write((void*)buffer.GetString(), 1, buffer.GetSize());
    m_fileIO->close();
}

rapidjson::Value jsonMakeItem(rapidjson::Document::AllocatorType& alloactor,
                              String* key, String* value)
{
    rapidjson::Value ret(rapidjson::kObjectType);
    ret.AddMember(
        KEY, rapidjson::Value(key->toUTF8NonGCString().data(), key->length()),
        alloactor);
    ret.AddMember(VALUE, rapidjson::Value(value->toUTF8NonGCString().data(),
                                          value->length()),
                  alloactor);
    return ret;
}

rapidjson::Value jsonMakeSecurity(rapidjson::Document::AllocatorType& alloactor,
                                  SecurityOriginData* securityOriginData)
{
    rapidjson::Value ret(rapidjson::kObjectType);
    ret.AddMember(
        PROTOCOL,
        rapidjson::Value(
            securityOriginData->protocol()->toUTF8NonGCString().data(),
            securityOriginData->protocol()->length()),
        alloactor);
    ret.AddMember(
        HOST,
        rapidjson::Value(securityOriginData->host()->toUTF8NonGCString().data(),
                         securityOriginData->host()->length()),
        alloactor);
    ret.AddMember(PORT, securityOriginData->port(), alloactor);
    return ret;
}

rapidjson::Value::ValueIterator jsonGetSecurity(
    rapidjson::Value& root, SecurityOriginData* securityOriginData)
{
    for (auto itr = root.Begin(); itr != root.End(); ++itr) {
        if ((*itr)[PROTOCOL] ==
                securityOriginData->protocol()->toUTF8NonGCString().data() &&
            (*itr)[HOST] ==
                securityOriginData->host()->toUTF8NonGCString().data() &&
            (*itr)[PORT] == securityOriginData->port()) {
            return itr;
        }
    }
    return root.End();
}

rapidjson::Value::ValueIterator jsonGetItems(
    rapidjson::Value::ValueIterator& root, String* key)
{
    for (auto itr = (*root)[ITEMS].Begin(); itr != (*root)[ITEMS].End();
         ++itr) {
        if ((*itr)[KEY] == key->toUTF8NonGCString().data()) {
            return itr;
        }
    }
    return (*root)[ITEMS].End();
}
}
