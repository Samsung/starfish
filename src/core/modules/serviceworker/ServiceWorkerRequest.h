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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && \
    !defined(__StarfishServiceWorkerRequest__)
#define __StarfishServiceWorkerRequest__

namespace Starfish {

class Promise;
class RequestTask;

class ServiceWorkerRequest : public Archivable {
public:
    // payload
    RequestId id;
    ServiceWorkerContextId contextId;
    String* name;
    String* scope{ nullptr };

    // serialize/deserialize
    const char* archiveId() const override;
    void archive(Archiver& ar) override;

    DEFINE_GETTER_SETTER(Promise*, promise, Promise);
    DEFINE_GETTER_SETTER(RequestTask*, postTask, PostTask);

private:
    Promise* m_promise{ nullptr };
    RequestTask* m_postTask{ nullptr };
};

// RequestTask

using Handler_t = void (*)(ServiceWorkerRequest&, TaskResult&, TaskParam&);

class RequestTask : public Task<Handler_t> {
public:
    RequestTask(Handler_t handler, std::initializer_list<void*> params = {})
        : Task(handler, std::move(params))
    {
    }

    void run(ServiceWorkerRequest* request,
             std::initializer_list<void*> results = {})
    {
        STARFISH_ASSERT(request != nullptr);
        TaskResult wrapper;
        wrapper.assign(results.begin(), results.end());
        m_handler(*request, wrapper, m_params);
    }
};

} // namespace Starfish

#endif
