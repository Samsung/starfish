/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#include "StarfishPlatform.h"

#if defined(PORT_EVENTLOOP_BACKEND_GLIB)

#include "StarfishConfig.h"
#include "binding/ScriptBindingInstance.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/threading/Mutex.h"
#include "core/modules/profiling/Profiling.h"
#include "core/page/GlobalScope.h"
#include "platform/message_loop/MessageLoopGLib.h"
#include "platform/message_loop/RunLoopGLib.h"

#include <glib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace Starfish {

static bool isSystemMainThread()
{
    return getCurrentThreadID() == getpid();
}

// How long runWithProcessMainThreadPausedSync() waits for a confirmation that
// the process main thread actually paused before giving up and running
// unprotected. Bounds the wait so a wrong assumption about the process main
// thread running a GLib loop on g_main_context_default() (or that loop
// having stopped/not started yet) can't hang JS execution forever.
static constexpr int kProcessMainThreadPauseTimeoutMs = 300;

// Priority for the pause-request idle source in
// runWithProcessMainThreadPausedSync(), set well above G_PRIORITY_HIGH.
// GLib only dispatches the highest-priority tier of ready sources in a given
// main-context iteration, deferring lower-priority ready sources to the next
// one. Recurring high-priority sources on the same context (e.g. EFL's
// ecore animator, once ecore_main_loop_glib_integrate() rides it on
// g_main_context_default()) could otherwise keep winning that tier every
// frame and starve our source indefinitely -- not just delay it for one
// callback's duration. This priority can't preempt a callback that's already
// running (no GLib source priority can: dispatch is cooperative, not
// preemptive), it only affects which source gets picked once the main loop
// is free to choose.
static constexpr int kProcessMainThreadPauseSourcePriority =
    G_PRIORITY_HIGH - 1000;

// Cross-thread rendezvous between the two directions of synchronous hand-off
// between the process main thread and the dedicated LWE thread (isolated
// thread mode only):
//  - runOnMainThreadSync(): the process main thread (or any other thread)
//    posts a functor to the LWE thread's own GMainContext and blocks until
//    it runs -- e.g. a synchronous public API call like EvaluateJavaScript().
//  - runWithProcessMainThreadPausedSync(): the LWE thread posts a pause
//    request to the process main thread's g_main_context_default() and
//    blocks until it's confirmed -- e.g. running an AddJavaScriptInterface
//    callback that host code assumes has the process main thread excluded.
// Both directions mean the same thing from the process main thread's point
// of view: "don't run any of your own code right now." An earlier version of
// this tracked that with separate atomic flags per direction, each checked
// and then separately committed to under a short-lived gate lock. That still
// left a gap: side A takes the lock, finds nothing pending, releases it, and
// only THEN commits to blocking; if side B's own "become pending" step lands
// in that exact gap, A never observes it, and both sides end up genuinely
// waiting on each other (LWE for a GSource main will never pump, main for a
// GSource LWE will never dispatch) until LWE's bounded timeout fires -- the
// wasted wait the timeout log kept reporting even after that first fix.
// Using one mutex + condition variable pair, with wait_for()'s predicate
// form on the waiting side, closes this properly: "check the other side's
// state" and "commit to waiting on it" happen atomically under the same
// lock on both sides, so whichever side commits first is unconditionally
// observed by the other -- there's no window left for the other side's
// commit to land unseen, regardless of which side gets there first.
static std::mutex g_rendezvousMutex;
static std::condition_variable g_rendezvousCv;

enum class RendezvousOwner {
    None,
    // The process main thread is parked inside runOnMainThreadSync(),
    // blocked waiting for the LWE thread to run its functor. It isn't
    // running any of its own code, so it's already "paused" without needing
    // the GLib pause dance below.
    MainBlockedOnLWE,
    // The LWE thread is running a functor with the process main thread's
    // own GLib loop excluded via the pause GSource below.
    LWEPausingMain,
};
// Guarded by g_rendezvousMutex. Also doubles as the reentrancy guard for
// runWithProcessMainThreadPausedSync(): a nested call from within its own
// paused functor sees LWEPausingMain and just runs directly instead of
// trying (and failing) to pause a process main thread that isn't iterating
// its GLib context right now anyway.
static RendezvousOwner g_rendezvousOwner = RendezvousOwner::None;

struct PauseRequest {
    bool mainConfirmed{ false };
    bool resumeMain{ false };
};
// Heap-allocated by runWithProcessMainThreadPausedSync() and freed ONLY by
// the GSource callback it posts (in every one of that callback's return
// paths), never by the function itself: if confirmation doesn't arrive
// before the timeout, the GSource may still be sitting unfired in
// g_main_context_default(), to run whenever (if ever) that context gets
// pumped again, and its callback needs the object to still be valid at that
// point, however much later. Non-null (guarded by g_rendezvousMutex) exactly
// while there's an outstanding pause request the GSource callback hasn't
// resolved yet; both sides compare against this pointer before trusting the
// fields inside, so a GSource that fires after its own attempt already
// resolved another way (confirmed early by runOnMainThreadSync(), or given
// up on via timeout) can tell it's stale and just free itself instead of
// parking the process main thread on a resumeMain signal that will never
// come.
static PauseRequest* g_currentPauseRequest = nullptr;

MessageLoopGLib::MessageLoopGLib()
    : MessageLoop()
{
}

MessageLoopGLib::MessageLoopGLib(RunLoopGLib* loop)
    : MessageLoop()
{
}

struct IdlerData {
    void (*m_fn)(size_t, void*);
    void* m_data;
    void* m_data1;
    void* m_data2;
    guint m_idler;
    MessageLoopGLib* m_ml;
    GlobalScope* m_globalScope;
    volatile bool m_needsRun;
    volatile bool m_isDestroyed;
    bool m_isMainThreadData;
};

void MessageLoopGLib::destroy()
{
    STARFISH_LOG_INFO("MessageLoopGLib::destroy()");
    m_inClosingState = true;
    while (true) {
        {
            Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
            if (m_idlers.size() == 0 && m_idlersFromOtherThread.size() == 0) {
                break;
            }
        }

        g_main_context_iteration(
            reinterpret_cast<GMainContext*>(glibMainContext()), FALSE);
    }
    {
        Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
        auto iterOther = m_idlersFromOtherThread.begin();
        while (iterOther != m_idlersFromOtherThread.end()) {
            IdlerData* id = (IdlerData*)*iterOther;
            id->m_isDestroyed = true;
            iterOther++;
        }
    }
    std::unordered_set<size_t>().swap(m_idlers);
    std::unordered_set<size_t>().swap(m_idlersFromOtherThread);
}

static bool removeIderFromList(std::unordered_set<size_t>& list, IdlerData* id)
{
    if (list.find((size_t)id) != list.end()) {
        list.erase(list.find((size_t)id));
        return true;
    }
    STARFISH_ASSERT_NOT_REACHED();
    return false;
}

size_t MessageLoopGLib::addIdler(GlobalScope* globalScope,
                                 void (*fn)(size_t, void*), void* data)
{
    STARFISH_ASSERT(isMainThread());
    IdlerData* id = new (NoGC) IdlerData;
    m_idlers.insert((size_t)id);
    id->m_isMainThreadData = true;
    id->m_fn = fn;
    id->m_data = data;
    id->m_ml = this;
    id->m_globalScope = globalScope;

    GSource* source = g_timeout_source_new(0);
    g_source_set_callback(
        source,
        (GSourceFunc)[](gpointer data)->gboolean {
            IdlerData* id = (IdlerData*)data;
            if (removeIderFromList(id->m_ml->m_idlers, id)) {
                id->m_fn((size_t)id, id->m_data);
                GC_FREE(id);
            }
            return G_SOURCE_REMOVE;
        },
        id, nullptr);
    id->m_idler = g_source_attach(
        source, reinterpret_cast<GMainContext*>(glibMainContext()));
    g_source_unref(source);

    return (size_t)id;
}

size_t MessageLoopGLib::addIdler(GlobalScope* globalScope,
                                 void (*fn)(size_t, void*, void*), void* data,
                                 void* data1)
{
    STARFISH_ASSERT(isMainThread());
    IdlerData* id = new (NoGC) IdlerData;
    m_idlers.insert((size_t)id);
    id->m_isMainThreadData = true;
    id->m_fn = (void (*)(size_t, void*))fn;
    id->m_data = data;
    id->m_data1 = data1;
    id->m_ml = this;
    id->m_globalScope = globalScope;

    GSource* source = g_timeout_source_new(0);
    g_source_set_callback(
        source,
        (GSourceFunc)[](gpointer data)->gboolean {
            IdlerData* id = (IdlerData*)data;

            if (removeIderFromList(id->m_ml->m_idlers, id)) {
                ((void (*)(size_t, void*, void*))id->m_fn)(
                    (size_t)id, id->m_data, id->m_data1);
                GC_FREE(id);
            }
            return G_SOURCE_REMOVE;
        },
        id, nullptr);
    id->m_idler = g_source_attach(
        source, reinterpret_cast<GMainContext*>(glibMainContext()));
    g_source_unref(source);

    return (size_t)id;
}

size_t MessageLoopGLib::addIdler(GlobalScope* globalScope,
                                 void (*fn)(size_t, void*, void*, void*),
                                 void* data, void* data1, void* data2)
{
    STARFISH_ASSERT(isMainThread());
    IdlerData* id = new (NoGC) IdlerData;
    m_idlers.insert((size_t)id);
    id->m_isMainThreadData = true;
    id->m_fn = (void (*)(size_t, void*))fn;
    id->m_data = data;
    id->m_data1 = data1;
    id->m_data2 = data2;
    id->m_ml = this;
    id->m_globalScope = globalScope;

    GSource* source = g_timeout_source_new(0);
    g_source_set_callback(
        source,
        (GSourceFunc)[](gpointer data)->gboolean {
            IdlerData* id = (IdlerData*)data;
            if (removeIderFromList(id->m_ml->m_idlers, id)) {
                ((void (*)(size_t, void*, void*, void*))id->m_fn)(
                    (size_t)id, id->m_data, id->m_data1, id->m_data2);
                GC_FREE(id);
            }
            return G_SOURCE_REMOVE;
        },
        id, nullptr);
    id->m_idler = g_source_attach(
        source, reinterpret_cast<GMainContext*>(glibMainContext()));
    g_source_unref(source);

    return (size_t)id;
}

size_t MessageLoopGLib::addIdlerWithNoGCRootingInOtherThread(
    GlobalScope* globalScope, void (*fn)(size_t, void*), void* data)
{
    STARFISH_ASSERT(!isMainThread());
    IdlerData* id = new IdlerData;
    id->m_isMainThreadData = false;
    id->m_needsRun = true;
    id->m_isDestroyed = false;
    id->m_fn = fn;
    id->m_data = data;
    id->m_ml = this;
    id->m_globalScope = globalScope;
    {
        Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
        m_idlersFromOtherThread.insert((size_t)id);
    }

    // Single hop: the callback body used to be deferred once more through
    // g_timeout_add(0) (an EFL elm-shutdown workaround carried through the
    // GLib port), costing an extra main-context iteration per posting. It
    // runs at G_PRIORITY_DEFAULT, same as the old g_timeout_add(0) source.
    GSource* source = g_idle_source_new();
    g_source_set_ready_time(source, -1);
    g_source_set_priority(source, G_PRIORITY_HIGH);
    g_source_set_callback(
        source,
        (GSourceFunc)[](gpointer data)->gboolean {
            IdlerData* id = (IdlerData*)data;
            if (!id->m_isDestroyed) {
                if (id->m_ml && id->m_ml->m_idlersFromOtherThreadMutex) {
                    Locker<Mutex> l(*id->m_ml->m_idlersFromOtherThreadMutex);
                    removeIderFromList(id->m_ml->m_idlersFromOtherThread, id);
                }
                if (id->m_needsRun) {
                    id->m_fn((size_t)id, id->m_data);
                }
            }

            id->m_ml = nullptr;
            delete id;
            return G_SOURCE_REMOVE;
        },
        id, nullptr);
    id->m_idler = g_source_attach(
        source, reinterpret_cast<GMainContext*>(glibMainContext()));
    g_source_set_ready_time(source, 0);
    g_source_unref(source);
    return (size_t)id;
}

size_t MessageLoopGLib::addIdlerWithNoGCRootingInOtherThread(
    GlobalScope* globalScope, void (*fn)(size_t, void*, void*), void* data,
    void* data1)
{
    STARFISH_ASSERT(!isMainThread());
    IdlerData* id = new IdlerData;
    id->m_isMainThreadData = false;
    id->m_needsRun = true;
    id->m_isDestroyed = false;
    id->m_fn = (void (*)(size_t, void*))fn;
    id->m_data = data;
    id->m_data1 = data1;
    id->m_ml = this;
    id->m_globalScope = globalScope;
    {
        Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
        m_idlersFromOtherThread.insert((size_t)id);
    }

    // Single hop: the callback body used to be deferred once more through
    // g_timeout_add(0) (an EFL elm-shutdown workaround carried through the
    // GLib port), costing an extra main-context iteration per posting. It
    // runs at G_PRIORITY_DEFAULT, same as the old g_timeout_add(0) source.
    GSource* source = g_idle_source_new();
    g_source_set_ready_time(source, -1);
    g_source_set_priority(source, G_PRIORITY_HIGH);
    g_source_set_callback(
        source,
        (GSourceFunc)[](gpointer data)->gboolean {
            IdlerData* id = (IdlerData*)data;
            if (!id->m_isDestroyed) {
                id->m_isDestroyed = true;
                if (id->m_ml && id->m_ml->m_idlersFromOtherThreadMutex) {
                    Locker<Mutex> l(*id->m_ml->m_idlersFromOtherThreadMutex);
                    removeIderFromList(id->m_ml->m_idlersFromOtherThread, id);
                }
                if (id->m_needsRun) {
                    ((void (*)(size_t, void*, void*))id->m_fn)(
                        (size_t)id, id->m_data, id->m_data1);
                }
            }

            id->m_ml = nullptr;
            delete id;
            return G_SOURCE_REMOVE;
        },
        id, nullptr);
    id->m_idler = g_source_attach(
        source, reinterpret_cast<GMainContext*>(glibMainContext()));
    g_source_set_ready_time(source, 0);
    g_source_unref(source);
    return (size_t)id;
}

void MessageLoopGLib::removeIdler(size_t handle)
{
    STARFISH_ASSERT(isMainThread());
    if (handle == MessageLoopInvalidID) {
        return;
    }
    IdlerData* id = (IdlerData*)handle;
    if (removeIderFromList(m_idlers, id)) {
        GSource* source = g_main_context_find_source_by_id(
            reinterpret_cast<GMainContext*>(glibMainContext()), id->m_idler);
        if (source) {
            g_source_destroy(source);
        }
        GC_FREE(id);
    }
}

void MessageLoopGLib::removeIdlerWithNoGCRooting(size_t handle)
{
    if (handle == MessageLoopInvalidID) {
        return;
    }
    IdlerData* id = (IdlerData*)handle;
    id->m_needsRun = false;
}

void MessageLoopGLib::clearPendingIdlers(GlobalScope* globalScope)
{
    STARFISH_LOG_INFO("clearPendingIdlers: globalScope[%p]", globalScope);
    STARFISH_ASSERT(isMainThread());

    // Remove idlers
    auto iter = m_idlers.begin();
    while (iter != m_idlers.end()) {
        IdlerData* id = (IdlerData*)*iter;
        if (id->m_globalScope == globalScope || globalScope == nullptr) {
            GSource* source = g_main_context_find_source_by_id(
                reinterpret_cast<GMainContext*>(glibMainContext()),
                id->m_idler);
            if (source) {
                g_source_destroy(source);
            }
            iter = m_idlers.erase(iter);
            GC_FREE(id);
        } else {
            iter++;
        }
    }
    // Remove idlers from other thread
    m_idlersFromOtherThreadMutex->lock();
    auto iterOther = m_idlersFromOtherThread.begin();
    while (iterOther != m_idlersFromOtherThread.end()) {
        IdlerData* id = (IdlerData*)*iterOther;
        if ((id->m_globalScope == globalScope || globalScope == nullptr) &&
            id->m_needsRun) {
            id->m_needsRun = false;
        }
        iterOther++;
    }
    m_idlersFromOtherThreadMutex->unlock();
}

void MessageLoopGLib::runOnMainThreadAsync(const std::function<void()>& functor)
{
    struct Param {
        std::function<void()> functor;
    };

    Param* p = new Param();
    p->functor = functor;

    if (isMainThread()) {
        // Already on this MessageLoop's main thread, add directly
        addIdler(
            nullptr,
            [](size_t, void* data) -> void {
                Param* p = (Param*)data;
                p->functor();
                delete p;
            },
            p);
    } else {
        // Called from another thread: add to this MessageLoop's GMainContext
        GMainContext* context =
            reinterpret_cast<GMainContext*>(glibMainContext());
        GSource* source = g_idle_source_new();
        g_source_set_ready_time(source, -1);
        g_source_set_priority(source, G_PRIORITY_DEFAULT);
        g_source_set_callback(
            source,
            (GSourceFunc)[](gpointer data)->gboolean {
                Param* p = (Param*)data;
                p->functor();
                delete p;
                return G_SOURCE_REMOVE;
            },
            p, nullptr);
        g_source_attach(source, context);
        g_source_set_ready_time(source, 0);
        g_source_unref(source);
    }
}

void MessageLoopGLib::init()
{
    // init our glib message-loop context for thread mode
    if (!isSystemMainThread()) {
        if (!g_threadedMainRunLoop) {
            RunLoopGLib* runLoop = new RunLoopGLib();
            g_threadedMainRunLoop = runLoop;
        }
    }
    // Note: in isolated thread mode, this runs on the freshly created LWE
    // thread, never on the process main thread -- so this is the wrong place
    // to ask "does the process main thread run a GLib loop?" (see the
    // diagnostic in LWE::Initialize() instead, which always runs on whatever
    // thread the embedder actually calls Initialize() from).
}

void MessageLoopGLib::run()
{
    if (!isSystemMainThread()) {
        g_threadedMainRunLoop->run();
    }
}

void MessageLoopGLib::stop()
{
    if (!isSystemMainThread()) {
        g_threadedMainRunLoop->stop();
    }
}

void MessageLoopGLib::runOnMainThreadSync(const std::function<void()>& functor)
{
    if (isMainThread()) {
        // Already on this MessageLoop's main thread, execute directly
        functor();
        return;
    }

    // Only the process main thread's own blocking wait stands in for a
    // pause: some other, unrelated thread blocking here says nothing about
    // whether the process main thread is free to run its own code.
    bool callerIsSystemMainThread = isSystemMainThread();
    if (callerIsSystemMainThread) {
        std::lock_guard<std::mutex> lock(g_rendezvousMutex);
        if (g_currentPauseRequest) {
            // The LWE thread has an outstanding pause request against us.
            // We're about to block waiting on the LWE thread instead of
            // pumping g_main_context_default(), which already satisfies
            // "main isn't running its own code" -- resolve its wait
            // directly rather than leaving it to possibly time out waiting
            // for a GSource we may never get back around to dispatching.
            // The GSource it posted is left attached; when/if it does fire
            // later, it'll find g_currentPauseRequest no longer pointing at
            // it and just free itself without parking us a second time.
            g_currentPauseRequest->mainConfirmed = true;
            g_currentPauseRequest = nullptr;
            g_rendezvousCv.notify_all();
        }
        g_rendezvousOwner = RendezvousOwner::MainBlockedOnLWE;
    }

    struct Param {
        std::function<void()> functor;
        std::mutex mutex;
        std::condition_variable cv;
        bool completed;
    };

    Param* p = new Param();
    p->functor = functor;
    p->completed = false;

    // Add task to this MessageLoop's GMainContext
    GMainContext* context = reinterpret_cast<GMainContext*>(glibMainContext());
    GSource* source = g_idle_source_new();
    g_source_set_ready_time(source, -1);
    g_source_set_priority(source, G_PRIORITY_DEFAULT);
    g_source_set_callback(
        source,
        (GSourceFunc)[](gpointer data)->gboolean {
            Param* p = (Param*)data;
            p->functor();
            {
                std::lock_guard<std::mutex> lock(p->mutex);
                p->completed = true;
            }
            p->cv.notify_one();
            return G_SOURCE_REMOVE;
        },
        p, nullptr);
    g_source_attach(source, context);
    g_source_set_ready_time(source, 0);
    g_source_unref(source);

    // Wait for completion
    {
        std::unique_lock<std::mutex> lock(p->mutex);
        p->cv.wait(lock, [p] { return p->completed; });
    }
    delete p;

    if (callerIsSystemMainThread) {
        std::lock_guard<std::mutex> lock(g_rendezvousMutex);
        if (g_rendezvousOwner == RendezvousOwner::MainBlockedOnLWE) {
            g_rendezvousOwner = RendezvousOwner::None;
        }
    }
}

void MessageLoopGLib::runWithProcessMainThreadPausedSync(
    const std::function<void()>& functor)
{
    if (!g_threadedMainRunLoop) {
        // Not running in isolated thread mode: the LWE thread already is the
        // process main thread (or there's no separate one), so there's
        // nothing to pause. Note glibMainContext() itself is nullptr here
        // too, which g_source_attach() below would already treat as the
        // default context -- this early-out just avoids the pause dance
        // entirely when it can't help.
        functor();
        return;
    }

    std::unique_lock<std::mutex> lock(g_rendezvousMutex);
    if (g_rendezvousOwner != RendezvousOwner::None) {
        // Either a nested call from within our own already-paused functor
        // (LWEPausingMain, set by an outer, still-running call on this same
        // thread), or the process main thread is already blocked waiting on
        // us (MainBlockedOnLWE) and thus already excluded without needing
        // the pause dance below. Either way, exclusivity already holds.
        lock.unlock();
        functor();
        return;
    }
    g_rendezvousOwner = RendezvousOwner::LWEPausingMain;
    // See g_currentPauseRequest's comment: freed only by the GSource
    // callback below, in every one of its return paths.
    PauseRequest* req = new PauseRequest();
    g_currentPauseRequest = req;
    lock.unlock();

    GSource* source = g_idle_source_new();
    g_source_set_ready_time(source, -1);
    g_source_set_priority(source, kProcessMainThreadPauseSourcePriority);
    g_source_set_callback(
        source,
        (GSourceFunc)[](gpointer data)->gboolean {
            PauseRequest* req = (PauseRequest*)data;
            std::unique_lock<std::mutex> lock(g_rendezvousMutex);
            if (g_currentPauseRequest != req) {
                // Stale: this attempt already resolved another way (the
                // process main thread reported itself already-blocked on
                // us via runOnMainThreadSync(), or we gave up waiting on
                // timeout). Nothing left to do.
                lock.unlock();
                delete req;
                return G_SOURCE_REMOVE;
            }
            req->mainConfirmed = true;
            g_currentPauseRequest = nullptr;
            g_rendezvousCv.notify_all();
            g_rendezvousCv.wait(lock, [req] { return req->resumeMain; });
            lock.unlock();
            delete req;
            return G_SOURCE_REMOVE;
        },
        req, nullptr);
    g_source_attach(source, g_main_context_default());
    g_source_set_ready_time(source, 0);
    g_source_unref(source);

    bool paused;
    {
        // Logs if confirming the pause takes suspiciously long, well before
        // the kProcessMainThreadPauseTimeoutMs hard cutoff -- a rising trend
        // here means the process main thread's GLib loop is getting
        // congested (or going away), not that any single call is broken.
        LongTaskFinder longTaskFinder(
            "MessageLoopGLib::runWithProcessMainThreadPausedSync waiting for "
            "process main thread to pause");
        std::unique_lock<std::mutex> waitLock(g_rendezvousMutex);
        paused = g_rendezvousCv.wait_for(
            waitLock,
            std::chrono::milliseconds(kProcessMainThreadPauseTimeoutMs),
            [req] { return req->mainConfirmed; });
        if (!paused && g_currentPauseRequest == req) {
            // Give up: mark it stale so the GSource callback, whenever/if
            // it eventually fires, just frees `req` and bails out instead
            // of parking the process main thread on a resumeMain signal
            // we never send in the timeout path below.
            g_currentPauseRequest = nullptr;
        }
    }

    if (!paused) {
        {
            std::lock_guard<std::mutex> lock(g_rendezvousMutex);
            g_rendezvousOwner = RendezvousOwner::None;
        }
        STARFISH_LOG_ERROR(
            "runWithProcessMainThreadPausedSync: gave up after %dms waiting "
            "for the process main thread to pause; running without "
            "exclusivity",
            kProcessMainThreadPauseTimeoutMs);
        functor();
        return;
    }

    struct ResumeGuard {
        PauseRequest* req;
        ~ResumeGuard()
        {
            std::lock_guard<std::mutex> lock(g_rendezvousMutex);
            g_rendezvousOwner = RendezvousOwner::None;
            req->resumeMain = true;
            g_rendezvousCv.notify_all();
        }
    } resumeGuard{ req };

    functor();
}

bool MessageLoopGLib::isCallerInsideBackendEventLoop()
{
    // g_main_depth() only reports the calling thread's own GLib dispatch
    // depth, so this is only meaningful when the caller is itself the
    // process main thread (e.g. LWE::Initialize(), called from whatever
    // thread the embedder actually invokes it from -- typically the process
    // main thread, alongside setting up its other UI components). Called
    // from any other thread (e.g. the dedicated LWE thread in isolated mode)
    // it can't tell us anything about the process main thread, so it's
    // deliberately not checked here.
    return isSystemMainThread() && g_main_depth() > 0;
}

} // namespace Starfish
#endif
