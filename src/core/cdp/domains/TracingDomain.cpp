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
#include "TracingDomain.h"
#include "../CDPDispatcher.h"
#include "../CDPCommand.h"
#include "../CDPSession.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <unistd.h>

namespace Starfish {

// The engine has no real trace recorder. The categories below mirror the
// default set DevTools/puppeteer expect from Tracing.getCategories so the
// client's category negotiation does not fail.
static const char* kCategories[] = {
    "devtools.timeline",
    "disabled-by-default-devtools.timeline",
    "disabled-by-default-devtools.timeline.frame",
    "disabled-by-default-devtools.timeline.stack",
    "v8",
    "v8.execute",
    "blink",
    "blink.user_timing",
    "loading",
    "latencyInfo",
    "toplevel",
    "disabled-by-default-v8.cpu_profiler",
};

// Append one Chrome trace event object to `arr`. `args` is moved in.
static void addTraceEvent(rapidjson::Value& arr,
                          rapidjson::Document::AllocatorType& alloc,
                          const char* name, const char* cat, const char* ph,
                          double ts, int pid, int tid, rapidjson::Value& args)
{
    rapidjson::Value e(rapidjson::kObjectType);
    e.AddMember("name", rapidjson::Value(name, alloc), alloc);
    e.AddMember("cat", rapidjson::Value(cat, alloc), alloc);
    e.AddMember("ph", rapidjson::Value(ph, alloc), alloc);
    e.AddMember("ts", ts, alloc);
    e.AddMember("pid", pid, alloc);
    e.AddMember("tid", tid, alloc);
    e.AddMember("args", args, alloc);
    arr.PushBack(e, alloc);
}

void TracingDomain::processMessage(CDPCommand& cmd, const std::string& method)
{
    CDPSession* s = m_dispatcher->session();

    if (method == "getCategories") {
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value cats(rapidjson::kArrayType);
        for (const char* c : kCategories) {
            cats.PushBack(rapidjson::Value(c, alloc), alloc);
        }
        result.AddMember("categories", cats, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "start") {
        // transferMode defaults to "ReportEvents" per CDP; puppeteer's
        // page.tracing.start() passes "ReturnAsStream".
        std::string transferMode = "ReportEvents";
        if (cmd.params() && cmd.params()->HasMember("transferMode") &&
            (*cmd.params())["transferMode"].IsString()) {
            transferMode = (*cmd.params())["transferMode"].GetString();
        }
        s->tracingActive = true;
        s->tracingTransferMode = transferMode;
        s->tracingStartTickUs = (uint64_t)longTickCount();
        cmd.sendResultEmpty();
        return;
    }

    if (method == "end") {
        // Ack the command first (CDP returns {} for Tracing.end), then deliver
        // the collected trace via dataCollected/stream and tracingComplete.
        cmd.sendResultEmpty();

        if (!s->tracingActive) {
            // No active session: still emit tracingComplete so a client that
            // called end without start does not hang.
            rapidjson::Document cdoc;
            rapidjson::Value cparams(rapidjson::kObjectType);
            cdoc.SetObject();
            cmd.sendEvent("Tracing.tracingComplete", cparams, cdoc);
            return;
        }
        s->tracingActive = false;

        // Synthesize a minimal but valid set of Chrome trace events. The engine
        // does not record real timeline data, so this is metadata (process/
        // thread names) plus a couple of timeline markers spanning the
        // start..end window. ts/dur are microseconds.
        int pid = (int)getpid();
        const int tid = 1;
        double startTs = (double)s->tracingStartTickUs;
        double endTs = (double)longTickCount();
        if (endTs < startTs) {
            endTs = startTs;
        }

        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value events(rapidjson::kArrayType);

        // process_name / thread_name metadata (ph "M").
        {
            rapidjson::Value args(rapidjson::kObjectType);
            args.AddMember("name", "Starfish", alloc);
            addTraceEvent(events, alloc, "process_name", "__metadata", "M", 0.0,
                          pid, tid, args);
        }
        {
            rapidjson::Value args(rapidjson::kObjectType);
            args.AddMember("name", "CrRendererMain", alloc);
            addTraceEvent(events, alloc, "thread_name", "__metadata", "M", 0.0,
                          pid, tid, args);
        }
        // TracingStartedInBrowser instant marker (devtools.timeline). DevTools
        // anchors the recording window on this event.
        {
            rapidjson::Value args(rapidjson::kObjectType);
            rapidjson::Value data(rapidjson::kObjectType);
            data.AddMember("frameTreeNodeId", 1, alloc);
            args.AddMember("data", data, alloc);
            addTraceEvent(events, alloc, "TracingStartedInBrowser",
                          "disabled-by-default-devtools.timeline", "I", startTs,
                          pid, tid, args);
        }
        // A RunTask duration event (B/E pair) spanning the trace window so the
        // file carries at least one non-instant timeline event.
        {
            rapidjson::Value args(rapidjson::kObjectType);
            addTraceEvent(events, alloc, "RunTask", "devtools.timeline", "B",
                          startTs, pid, tid, args);
        }
        {
            rapidjson::Value args(rapidjson::kObjectType);
            addTraceEvent(events, alloc, "RunTask", "devtools.timeline", "E",
                          endTs, pid, tid, args);
        }

        if (s->tracingTransferMode == "ReturnAsStream") {
            // Serialize {"traceEvents":[...]} into an IO stream and hand its
            // handle to the client via tracingComplete. puppeteer drains it
            // with IO.read/IO.close and JSON.parses the result.
            rapidjson::Value root(rapidjson::kObjectType);
            root.AddMember("traceEvents", events, alloc);
            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            root.Accept(w);

            std::string handle = std::to_string(++s->ioStreamCounter);
            IOStream stream;
            stream.data.assign(sb.GetString(), sb.GetString() + sb.GetSize());
            s->ioStreams[handle] = std::move(stream);

            rapidjson::Document cdoc;
            rapidjson::Document::AllocatorType& calloc = cdoc.GetAllocator();
            rapidjson::Value cparams(rapidjson::kObjectType);
            cparams.AddMember(
                "stream",
                rapidjson::Value(handle.c_str(), handle.size(), calloc),
                calloc);
            // dataFormat/traceFormat: the stream holds JSON.
            cparams.AddMember("dataFormat", "json", calloc);
            cparams.AddMember("traceFormat", "json", calloc);
            cmd.sendEvent("Tracing.tracingComplete", cparams, cdoc);
        } else {
            // ReportEvents: deliver the events inline via dataCollected, then
            // signal completion.
            rapidjson::Document ddoc;
            rapidjson::Document::AllocatorType& dalloc = ddoc.GetAllocator();
            rapidjson::Value dparams(rapidjson::kObjectType);
            dparams.AddMember("value", events, dalloc); // moves `events`
            cmd.sendEvent("Tracing.dataCollected", dparams, ddoc);

            rapidjson::Document cdoc;
            rapidjson::Value cparams(rapidjson::kObjectType);
            cdoc.SetObject();
            cmd.sendEvent("Tracing.tracingComplete", cparams, cdoc);
        }
        return;
    }

    if (method == "requestMemoryDump") {
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember("dumpGuid", "0000000000000000", alloc);
        result.AddMember("success", true, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    // getBufferUsage / recordClock / etc. are acked so client handshakes do not
    // throw.
    cmd.sendResultEmpty();
}

} // namespace Starfish

#endif
