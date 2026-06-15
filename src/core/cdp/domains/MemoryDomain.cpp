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
#include "MemoryDomain.h"
#include "../CDPDispatcher.h"
#include "../CDPCommand.h"
#include "core/page/WebView.h"
#include "core/page/BrowsingContext.h"
#include "core/dom/Document.h"
#include "core/dom/Node.h"

#include "rapidjson/document.h"
#include <gc.h>

namespace Starfish {

// Recursively count nodes in a Node subtree (the node itself plus descendants).
// Mirrors the same counting the Performance domain's getMetrics performs.
static size_t countNodes(Node* node)
{
    if (!node) {
        return 0;
    }
    size_t n = 1;
    for (Node* c = node->firstChild(); c; c = c->nextSibling()) {
        n += countNodes(c);
    }
    return n;
}

// Sample the live DOM: the document count (main frame plus loaded child frames)
// and the total DOM node count across those documents. This is the same live
// sampling Performance.getMetrics reports for "Documents" / "Nodes".
static void sampleDOM(WebView* wv, size_t& documents, size_t& nodes)
{
    size_t childFrames = 0;
    size_t nodeCount = 0;
    BrowsingContext* main = wv ? wv->mainBrowsingContext() : nullptr;
    if (main) {
        if (main->document()) {
            nodeCount += countNodes(main->document());
        }
        main->iterateChildContext([&](BrowsingContext* child) {
            if (child && child->document()) {
                childFrames++;
                nodeCount += countNodes(child->document());
            }
        });
    }
    documents = 1 + childFrames;
    nodes = nodeCount;
}

void MemoryDomain::processMessage(CDPCommand& cmd, const std::string& method)
{
    if (method == "getDOMCounters") {
        WebView* wv = m_dispatcher->webView();
        size_t documents = 0;
        size_t nodes = 0;
        sampleDOM(wv, documents, nodes);

        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember("documents", (int)documents, alloc);
        result.AddMember("nodes", (int)nodes, alloc);
        // jsEventListeners is not instrumented by this engine; reported as 0
        // (mirrors Performance.getMetrics' JSEventListeners).
        result.AddMember("jsEventListeners", 0, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "getDOMCountersForLeakDetection") {
        WebView* wv = m_dispatcher->webView();
        size_t documents = 0;
        size_t nodes = 0;
        sampleDOM(wv, documents, nodes);

        struct Counter {
            const char* name;
            int count;
        };
        const Counter counters[] = {
            { "documents", (int)documents },
            { "nodes", (int)nodes },
            { "jsEventListeners", 0 },
        };

        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value arr(rapidjson::kArrayType);
        for (const Counter& c : counters) {
            rapidjson::Value entry(rapidjson::kObjectType);
            entry.AddMember("name", rapidjson::Value(c.name, alloc), alloc);
            entry.AddMember("count", c.count, alloc);
            arr.PushBack(entry, alloc);
        }
        result.AddMember("counters", arr, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "forciblyPurgeJavaScriptMemory") {
        // Run a GC pass on the engine heap. GC_gcollect is the same collector
        // entry point the engine uses for forced collection elsewhere (WebView
        // teardown, navigation). Twice to also sweep objects freed by the first
        // pass becoming unreachable.
        GC_gcollect();
        GC_gcollect();
        cmd.sendResultEmpty();
        return;
    }

    if (method == "setPressureNotificationsSuppressed" ||
        method == "simulatePressureNotification") {
        // No memory-pressure subsystem is wired to the engine; ack so clients
        // that toggle / simulate pressure do not error.
        cmd.sendResultEmpty();
        return;
    }

    if (method == "getAllTimeSamplingProfile" ||
        method == "getBrowserSamplingProfile") {
        // No native memory sampling profiler; return an empty SamplingProfile
        // (empty samples + modules) so clients receive the expected shape.
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value profile(rapidjson::kObjectType);
        profile.AddMember("samples", rapidjson::Value(rapidjson::kArrayType),
                          alloc);
        profile.AddMember("modules", rapidjson::Value(rapidjson::kArrayType),
                          alloc);
        result.AddMember("profile", profile, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    // enable / disable / startSampling / stopSampling and any other Memory.*
    // are acked so handshakes proceed.
    cmd.sendResultEmpty();
}

} // namespace Starfish

#endif
