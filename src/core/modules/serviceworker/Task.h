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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && !defined(__StarfishTask__)
#define __StarfishTask__

namespace Starfish {

using TaskParam = GCVector<void*>;
using TaskResult = GCVector<void*>;

template <typename HandlerType>
class Task : public gc {
public:
    Task(HandlerType handler, TaskParam params)
        : m_handler(handler)
        , m_params(std::move(params))
    {
        STARFISH_ASSERT(m_handler);
    }

    Task(HandlerType handler, std::initializer_list<void*> params)
        : m_handler(handler)
    {
        STARFISH_ASSERT(m_handler);
        m_params.assign(params.begin(), params.end());
    }

    void destroy()
    {
        for (auto&& param : m_params) {
            GC_FREE(param);
        }
    }

    void run(std::vector<void*> results = {})
    {
        m_handler(m_params, results);
    }

protected:
    HandlerType m_handler;
    TaskParam m_params;
};

template <typename IdType, typename ValueType>
class TaskMap : public gc {
public:
    void destroy()
    {
        for (auto const& it : m_taskMap) {
            (it.second)->destroy();
        }
    }

    void add(IdType id, ValueType task)
    {
        m_taskMap.insert(std::make_pair(id, task));
    }

    ValueType find(IdType id)
    {
        auto it = m_taskMap.find(id);
        if (it == m_taskMap.end()) {
            return nullptr;
        }
        return it->second;
    }

    void finish(IdType id)
    {
        m_taskMap.erase(id);
    }

private:
    GCUnorderedMap<IdType, ValueType, IdHash> m_taskMap;
};

} // namespace Starfish

#endif
