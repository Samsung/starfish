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
#include "AnimationDomain.h"
#include "../CDPDispatcher.h"
#include "../CDPCommand.h"
#include "../CDPSession.h"
#include "../TargetContext.h"

#include "rapidjson/document.h"

namespace Starfish {

void AnimationDomain::processMessage(CDPCommand& cmd, const std::string& method)
{
    CDPSession* session = m_dispatcher->session();

    if (method == "enable") {
        session->animationEnabled = true;
        cmd.sendResultEmpty();
        return;
    }

    if (method == "disable") {
        session->animationEnabled = false;
        cmd.sendResultEmpty();
        return;
    }

    if (method == "getPlaybackRate") {
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember("playbackRate", session->animationPlaybackRate, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "setPlaybackRate") {
        // Store the global playback rate (set->get round-trips). The engine
        // drives animations off the real wall clock with no global time-scale
        // hook, so this is not applied to running animations.
        if (cmd.params() && cmd.params()->HasMember("playbackRate") &&
            (*cmd.params())["playbackRate"].IsNumber()) {
            session->animationPlaybackRate =
                (*cmd.params())["playbackRate"].GetDouble();
        }
        cmd.sendResultEmpty();
        return;
    }

    if (method == "getCurrentTime") {
        // No per-instance animation tracking is exposed to CDP, so the current
        // time of a given animation id is reported as 0 (acked, not throwing)
        // so clients that probe it proceed.
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember("currentTime", 0.0, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    // setPaused / seekAnimations / releaseAnimations / setTiming: no global
    // pause/seek/timing hook in the engine, so these are acked (no throw) so
    // DevTools clients that drive them do not error.
    if (method == "setPaused" || method == "seekAnimations" ||
        method == "releaseAnimations" || method == "setTiming") {
        cmd.sendResultEmpty();
        return;
    }

    if (method == "resolveAnimation") {
        // Would return a Runtime.RemoteObject for the animation's WebAnimation
        // wrapper. No such wrapper is tracked; return an empty RemoteObject of
        // type "object" so the shape is valid and clients do not error.
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value remoteObject(rapidjson::kObjectType);
        remoteObject.AddMember("type", "object", alloc);
        result.AddMember("remoteObject", remoteObject, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    // Any other Animation.* method is acked so handshakes proceed.
    cmd.sendResultEmpty();
}

void AnimationDomain::emitAnimationStarted(WebView* webView,
                                           const std::string& animationName,
                                           double durationMs)
{
    // Route to the TargetContext (session) owning the WebView the animation
    // belongs to, mirroring CDPDispatcher::emitBindingCalled.
    TargetContext* ctx = nullptr;
    for (TargetContext* c : m_dispatcher->contexts()) {
        if (c->webView == webView) {
            ctx = c;
            break;
        }
    }
    if (!ctx) {
        return;
    }
    CDPSession* session = ctx->session;
    if (!session->animationEnabled) {
        return;
    }

    std::string id = "ANIM-" + std::to_string(++session->animationCounter);

    // Build an Animation object (CDP Animation.Animation type). type is
    // "CSSAnimation"; source is an AnimationEffect carrying the @keyframes name
    // and duration.
    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

    rapidjson::Value source(rapidjson::kObjectType);
    source.AddMember("delay", 0.0, alloc);
    source.AddMember("endDelay", 0.0, alloc);
    source.AddMember("iterationStart", 0.0, alloc);
    source.AddMember("iterations", 1.0, alloc);
    source.AddMember("duration", durationMs, alloc);
    source.AddMember("direction", "normal", alloc);
    source.AddMember("fill", "none", alloc);
    source.AddMember("easing", "linear", alloc);

    rapidjson::Value animation(rapidjson::kObjectType);
    animation.AddMember("id", rapidjson::Value(id.c_str(), id.size(), alloc),
                        alloc);
    animation.AddMember(
        "name",
        rapidjson::Value(animationName.c_str(), animationName.size(), alloc),
        alloc);
    animation.AddMember("pausedState", false, alloc);
    animation.AddMember("playState", "running", alloc);
    animation.AddMember("playbackRate", session->animationPlaybackRate, alloc);
    animation.AddMember("startTime", 0.0, alloc);
    animation.AddMember("currentTime", 0.0, alloc);
    animation.AddMember("type", "CSSAnimation", alloc);
    // source is consumed by the AddMember below (moved); rapidjson copies into
    // the animation object's allocator (same doc), so it stays valid.
    animation.AddMember("source", source, alloc);

    // Animation.animationCreated carries only the id.
    {
        rapidjson::Value created(rapidjson::kObjectType);
        created.AddMember("id", rapidjson::Value(id.c_str(), id.size(), alloc),
                          alloc);
        CDPCommand evt(m_dispatcher, Optional<int64_t>(), session->sessionId,
                       nullptr);
        evt.sendEvent("Animation.animationCreated", created, doc);
    }

    // Animation.animationStarted carries the full Animation object.
    {
        CDPCommand evt(m_dispatcher, Optional<int64_t>(), session->sessionId,
                       nullptr);
        evt.sendEvent("Animation.animationStarted", animation, doc);
    }
}

} // namespace Starfish

#endif
