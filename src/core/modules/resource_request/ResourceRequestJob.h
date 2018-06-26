/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishResourceRequestJobInterface__
#define __StarFishResourceRequestJobInterface__

namespace StarFish {

class ResourceRequest;

String* decodeURL(String* src, size_t idx);

class ResourceRequestJobInterface {
public:
    virtual ~ResourceRequestJobInterface()
    {
    }
    // Currently, only 'send' is chosen as a common interface, but more
    // interfaces can be added later.
    virtual void send(String* body = String::emptyString,
                      bool allowCache = false) = 0;
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
    virtual void send(String* body = String::emptyString,
                      bool allowCache = false);

private:
    ResourceRequest* m_orgProxy;
};

class DataURLResourceRequestJobDelegate : public gc,
                                          public ResourceRequestJobInterface {
public:
    static void worker(ResourceRequest* res, String* filePath);
    DataURLResourceRequestJobDelegate(ResourceRequest* proxy);
    virtual void send(String* body = String::emptyString,
                      bool allowCache = false);

private:
    ResourceRequest* m_orgProxy;
};

class BlobURLResourceRequestJobDelegate : public gc,
                                          public ResourceRequestJobInterface {
public:
    static void worker(ResourceRequest* res, String* filePath);
    BlobURLResourceRequestJobDelegate(ResourceRequest* proxy);
    virtual void send(String* body = String::emptyString,
                      bool allowCache = false);

private:
    ResourceRequest* m_orgProxy;
};

class AboutURLResourceRequestJobDelegate : public gc,
                                           public ResourceRequestJobInterface {
public:
    static void worker(ResourceRequest* res, String* filePath);
    AboutURLResourceRequestJobDelegate(ResourceRequest* proxy);
    virtual void send(String* body = String::emptyString,
                      bool allowCache = false);

private:
    ResourceRequest* m_orgProxy;
};

class JavaScriptURLResourceRequestJobDelegate
    : public gc,
      public ResourceRequestJobInterface {
public:
    static void worker(ResourceRequest* res, String* filePath);
    JavaScriptURLResourceRequestJobDelegate(ResourceRequest* proxy);
    virtual void send(String* body = String::emptyString,
                      bool allowCache = false);

private:
    ResourceRequest* m_orgProxy;
};

class UnknownURLResourceRequestJobDelegate
    : public gc,
      public ResourceRequestJobInterface {
public:
    static void worker(ResourceRequest* res, String* filePath);
    UnknownURLResourceRequestJobDelegate(ResourceRequest* proxy);
    virtual void send(String* body = String::emptyString,
                      bool allowCache = false);

private:
    ResourceRequest* m_orgProxy;
};
}

#endif
