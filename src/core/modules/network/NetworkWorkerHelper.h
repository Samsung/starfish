/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#ifndef __StarFishNetworkWorkerHelper__
#define __StarFishNetworkWorkerHelper__

namespace StarFish {

class NetworkRequest;
class NetworkWorkerHelper;

struct NetworkWorkerData {
    NetworkRequest* request;
    NetworkWorkerHelper* networkWorker;
    CURL* curl;
    curl_slist* headerList;
    bool isSync;
    bool isAborted;
    long responseCode;
    int res;
};

class NetworkWorkerHelper : public gc {
public:
    NetworkWorkerHelper()
    {
    }
    virtual ~NetworkWorkerHelper()
    {
    }
    void* networkWorker(void* data);

protected:
    virtual void responseHandlerWrapper(int res, NetworkWorkerData* requestData)
    {
    }
    static void responseHandler(size_t handle, void* requestData);
};

class AsyncNetworkWorkHelper : public NetworkWorkerHelper {
protected:
    virtual void responseHandlerWrapper(int res,
                                        NetworkWorkerData* requestData);
};

class SyncNetworkWorkHelper : public NetworkWorkerHelper {
protected:
    virtual void responseHandlerWrapper(int res,
                                        NetworkWorkerData* requestData);
};
}

#endif
