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

#ifndef __StarFishResourceRequestJobInterface__
#define __StarFishResourceRequestJobInterface__

namespace StarFish {

class ResourceRequest;

static String* decodeURL(String* src, size_t idx);

class ResourceRequestJobInterface {
public:
    // Currently, only 'send' is chosen as a common interface, but more
    // interfaces can be added later.
    virtual void send(String* body = String::emptyString) = 0;
};

class ResourceRequestJobDelegateFactory {
public:
    static ResourceRequestJobInterface* createJob(ResourceRequest* proxy);

private:
    ResourceRequestJobDelegateFactory(){};
    ~ResourceRequestJobDelegateFactory(){};
};

class FileURLResourceRequestJobDelegate : public gc,
                                          public ResourceRequestJobInterface {
public:
    static void worker(ResourceRequest* res, String* filePath);
    FileURLResourceRequestJobDelegate(ResourceRequest* proxy);
    virtual void send(String* body = String::emptyString);

private:
    ResourceRequest* m_orgProxy;
};

class DataURLResourceRequestJobDelegate : public gc,
                                          public ResourceRequestJobInterface {
public:
    static void worker(ResourceRequest* res, String* filePath);
    DataURLResourceRequestJobDelegate(ResourceRequest* proxy);
    virtual void send(String* body = String::emptyString);

private:
    ResourceRequest* m_orgProxy;
};

class BlobURLResourceRequestJobDelegate : public gc,
                                          public ResourceRequestJobInterface {
public:
    static void worker(ResourceRequest* res, String* filePath);
    BlobURLResourceRequestJobDelegate(ResourceRequest* proxy);
    virtual void send(String* body = String::emptyString);

private:
    ResourceRequest* m_orgProxy;
};
}

#endif
