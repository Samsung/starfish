/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_CDP)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "PerformanceDomain.h"
#include "MemoryDomain.h"
#include "../CDPDispatcher.h"
#include "../CDPCommand.h"
#include "../CDPSession.h"
#include "core/page/WebView.h"
#include "core/page/BrowsingContext.h"
#include "core/dom/Document.h"
#include "core/dom/Node.h"

#include "rapidjson/document.h"
#include <gc.h>

namespace Starfish {

void PerformanceDomain::processMessage(CDPCommand& cmd,
                                       const std::string& method)
{
    CDPSession* s = m_dispatcher->session();

    if (method == "enable") {
        s->performanceEnabled = true;
        cmd.sendResultEmpty();
        return;
    }
    if (method == "disable") {
        s->performanceEnabled = false;
        cmd.sendResultEmpty();
        return;
    }
    if (method == "setTimeDomain") {
        // Only "timeTicks"/"threadTicks" enums exist; we always report
        // monotonic ticks. Ack so the client's choice does not error.
        cmd.sendResultEmpty();
        return;
    }
    if (method == "getMetrics") {
        WebView* wv = m_dispatcher->webView();

        // Frame/document count: the main frame plus its direct child
        // BrowsingContexts that have loaded a document.
        size_t documentCount = 0;
        size_t nodeCount = 0;
        sampleDOMCounters(wv, documentCount, nodeCount);
        double documents = (double)documentCount;
        double frames = (double)documentCount;

        // GC heap: total committed heap vs. used (committed minus free).
        size_t heapTotal = GC_get_heap_size();
        size_t heapFree = GC_get_free_bytes();
        size_t heapUsed = heapTotal > heapFree ? heapTotal - heapFree : 0;

        // Monotonic timestamp in seconds (longTickCount is microseconds).
        double timestamp = (double)longTickCount() / 1e6;

        struct Metric {
            const char* name;
            double value;
        };
        // The full key set puppeteer's page.metrics() consumes. Metrics this
        // engine does not instrument (durations, listener/layout counters) are
        // reported as 0 so the returned object always carries every key.
        const Metric metrics[] = {
            { "Timestamp", timestamp },
            { "Documents", documents },
            { "Frames", frames },
            { "JSEventListeners", 0.0 },
            { "Nodes", (double)nodeCount },
            { "LayoutCount", 0.0 },
            { "RecalcStyleCount", 0.0 },
            { "LayoutDuration", 0.0 },
            { "RecalcStyleDuration", 0.0 },
            { "ScriptDuration", 0.0 },
            { "TaskDuration", 0.0 },
            { "JSHeapUsedSize", (double)heapUsed },
            { "JSHeapTotalSize", (double)heapTotal },
        };

        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value arr(rapidjson::kArrayType);
        for (const Metric& m : metrics) {
            rapidjson::Value entry(rapidjson::kObjectType);
            entry.AddMember("name", rapidjson::Value(m.name, alloc), alloc);
            entry.AddMember("value", m.value, alloc);
            arr.PushBack(entry, alloc);
        }
        result.AddMember("metrics", arr, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    cmd.sendError(-32601, "'method' wasn't found");
}

} // namespace Starfish

#endif
