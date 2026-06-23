/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_TEST)

#include "APIReplayer.h"

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

static uint64_t replayCurrentMicros()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

namespace StarfishShell {

// ── Lightweight JSON key extractors ──────────────────────────────────────────
// These work on the fixed formats produced by APIRecorder and are not
// general-purpose parsers.

static bool extractString(const char* json, const char* key, std::string& out)
{
    // Search for "key":"
    std::string needle = std::string("\"") + key + "\":\"";
    const char* p = strstr(json, needle.c_str());
    if (!p)
        return false;
    p += needle.size();
    std::string result;
    while (*p && *p != '"') {
        if (*p == '\\' && *(p + 1)) {
            ++p;
            switch (*p) {
            case '"':
                result += '"';
                break;
            case '\\':
                result += '\\';
                break;
            case 'n':
                result += '\n';
                break;
            case 'r':
                result += '\r';
                break;
            case 't':
                result += '\t';
                break;
            default:
                result += *p;
                break;
            }
        } else {
            result += *p;
        }
        ++p;
    }
    out = std::move(result);
    return true;
}

static bool extractUInt64(const char* json, const char* key, uint64_t& out)
{
    std::string needle = std::string("\"") + key + "\":";
    const char* p = strstr(json, needle.c_str());
    if (!p)
        return false;
    p += needle.size();
    char* end;
    out = (uint64_t)strtoull(p, &end, 10);
    return end != p;
}

static bool extractDouble(const char* json, const char* key, double& out)
{
    std::string needle = std::string("\"") + key + "\":";
    const char* p = strstr(json, needle.c_str());
    if (!p)
        return false;
    p += needle.size();
    char* end;
    out = strtod(p, &end);
    return end != p;
}

static bool extractInt(const char* json, const char* key, int& out)
{
    std::string needle = std::string("\"") + key + "\":";
    const char* p = strstr(json, needle.c_str());
    if (!p)
        return false;
    p += needle.size();
    char* end;
    out = (int)strtol(p, &end, 10);
    return end != p;
}

// Reads one JSON string token starting at *p (which must point at the opening
// quote). Advances *p past the closing quote. Returns the unescaped content.
static std::string readJsonStringToken(const char** p)
{
    std::string out;
    if (**p != '"')
        return out;
    ++(*p); // skip opening quote
    while (**p && **p != '"') {
        if (**p == '\\' && *(*p + 1)) {
            ++(*p);
            switch (**p) {
            case 'n':
                out += '\n';
                break;
            case 'r':
                out += '\r';
                break;
            case 't':
                out += '\t';
                break;
            default:
                out += **p;
                break;
            }
        } else {
            out += **p;
        }
        ++(*p);
    }
    if (**p == '"')
        ++(*p); // skip closing quote
    return out;
}

// Parses the "args" object of a SetSettings line into key/value pairs.
static void parseSettingsObject(
    const char* json, std::vector<std::pair<std::string, std::string>>& out)
{
    const char* p = strstr(json, "\"args\":{");
    if (!p)
        return;
    p += strlen("\"args\":{");
    while (*p && *p != '}') {
        while (*p && *p != '"' && *p != '}')
            ++p;
        if (*p != '"')
            break;
        std::string key = readJsonStringToken(&p);
        while (*p && *p != ':')
            ++p;
        if (*p == ':')
            ++p;
        while (*p && *p != '"' && *p != '}')
            ++p;
        if (*p != '"')
            break;
        std::string value = readJsonStringToken(&p);
        out.emplace_back(std::move(key), std::move(value));
        while (*p && *p != ',' && *p != '}')
            ++p;
        if (*p == ',')
            ++p;
    }
}

// ── APIReplayer::load
// ─────────────────────────────────────────────────────────

bool APIReplayer::load(const char* path)
{
    FILE* f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "[APIReplayer] Failed to open: %s\n", path);
        return false;
    }

    char line[131072]; // 128 KB — enough for LoadData with truncation
    size_t eventCount = 0;
    while (fgets(line, (int)sizeof(line), f)) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';
        if (line[0] == '\0')
            continue;
        if (parseLine(line, strlen(line)))
            ++eventCount;
    }
    fclose(f);

    fprintf(stderr, "[APIReplayer] Loaded %zu events from: %s\n", eventCount,
            path);
    return !m_events.empty();
}

bool APIReplayer::parseLine(const char* line, size_t /*len*/)
{
    std::string type;
    if (!extractString(line, "type", type))
        return false;

    if (type == "header") {
        double dpr = 1.0;
        int w = 1920, h = 1080;
        extractInt(line, "w", w);
        extractInt(line, "h", h);
        extractDouble(line, "dpr", dpr);
        m_header.width = (unsigned)w;
        m_header.height = (unsigned)h;
        m_header.devicePixelRatio = (float)dpr;
        return true;
    }

    ReplayEvent ev;
    ev.type = type;

    uint64_t tsUs = 0;
    extractUInt64(line, "ts_us", tsUs);
    ev.tsUs = tsUs;

    if (type == "LoadURL") {
        extractString(line, "url", ev.strArg);
    } else if (type == "LoadData") {
        extractString(line, "data", ev.strArg);
    } else if (type == "EvaluateJavaScript") {
        extractString(line, "script", ev.strArg);
    } else if (type == "SetUserAgentString") {
        extractString(line, "ua", ev.strArg);
    } else if (type == "AddJavaScriptInterface" ||
               type == "RemoveJavascriptInterface") {
        extractString(line, "object", ev.strArg);
        extractString(line, "function", ev.strArg2);
    } else if (type == "SetGCFrequency") {
        extractInt(line, "freq", ev.intArg);
    } else if (type == "SetCacheMode") {
        extractInt(line, "mode", ev.intArg);
    } else if (type == "SetDefaultFontSize") {
        extractInt(line, "size", ev.intArg);
    } else if (type == "SetSettings") {
        parseSettingsObject(line, ev.settings);
    } else if (type == "DispatchCompositionStartEvent" ||
               type == "DispatchCompositionUpdateEvent" ||
               type == "DispatchCompositionEndEvent") {
        extractString(line, "text", ev.strArg);
    } else if (type == "DispatchMouseMoveEvent" ||
               type == "DispatchMouseDownEvent" ||
               type == "DispatchMouseUpEvent") {
        extractInt(line, "button", ev.button);
        extractInt(line, "buttons", ev.buttons);
        extractDouble(line, "x", ev.x);
        extractDouble(line, "y", ev.y);
    } else if (type == "DispatchMouseWheelEvent") {
        extractDouble(line, "x", ev.x);
        extractDouble(line, "y", ev.y);
        extractInt(line, "delta", ev.delta);
    } else if (type == "DispatchKeyDownEvent" ||
               type == "DispatchKeyPressEvent" ||
               type == "DispatchKeyUpEvent") {
        extractInt(line, "key", ev.key);
    } else if (type == "ResizeTo") {
        int w = 0, h = 0;
        extractInt(line, "w", w);
        extractInt(line, "h", h);
        ev.uW = (size_t)w;
        ev.uH = (size_t)h;
    } else if (type == "ScrollTo" || type == "ScrollBy") {
        extractInt(line, "x", ev.iX);
        extractInt(line, "y", ev.iY);
    } else if (type == "SetDevicePixelRatio") {
        double dpr = 0;
        extractDouble(line, "dpr", dpr);
        ev.dpr = (float)dpr;
    }
    // Zero-arg events (Reload, GoBack, etc.) need no field extraction.

    m_events.push_back(std::move(ev));
    return true;
}

// ── Event dispatch
// ────────────────────────────────────────────────────────────

static void dispatchEvent(LWE::WebContainer* wc, const ReplayEvent& ev)
{
    const std::string& t = ev.type;

    if (t == "SetGCFrequency") {
        LWE::LWE::SetGCFrequency((unsigned char)ev.intArg);
    } else if (t == "LoadURL") {
        wc->LoadURL(ev.strArg);
    } else if (t == "LoadData") {
        wc->LoadData(ev.strArg);
    } else if (t == "Reload") {
        wc->Reload();
    } else if (t == "StopLoading") {
        wc->StopLoading();
    } else if (t == "GoBack") {
        wc->GoBack();
    } else if (t == "GoForward") {
        wc->GoForward();
    } else if (t == "ClearHistory") {
        wc->ClearHistory();
    } else if (t == "ClearCache") {
        wc->ClearCache();
    } else if (t == "ClearCookies") {
        LWE::CookieManager* cm = LWE::CookieManager::GetInstance();
        if (cm)
            cm->ClearCookies();
    } else if (t == "Pause") {
        wc->Pause();
    } else if (t == "Resume") {
        wc->Resume();
    } else if (t == "Focus") {
        wc->Focus();
    } else if (t == "Blur") {
        wc->Blur();
    } else if (t == "ResizeTo") {
        wc->ResizeTo(ev.uW, ev.uH);
    } else if (t == "DispatchMouseMoveEvent") {
        wc->DispatchMouseMoveEvent((LWE::MouseButtonValue)ev.button,
                                   (LWE::MouseButtonsValue)ev.buttons, ev.x,
                                   ev.y);
    } else if (t == "DispatchMouseDownEvent") {
        wc->DispatchMouseDownEvent((LWE::MouseButtonValue)ev.button,
                                   (LWE::MouseButtonsValue)ev.buttons, ev.x,
                                   ev.y);
    } else if (t == "DispatchMouseUpEvent") {
        wc->DispatchMouseUpEvent((LWE::MouseButtonValue)ev.button,
                                 (LWE::MouseButtonsValue)ev.buttons, ev.x,
                                 ev.y);
    } else if (t == "DispatchMouseWheelEvent") {
        wc->DispatchMouseWheelEvent(ev.x, ev.y, ev.delta);
    } else if (t == "DispatchKeyDownEvent") {
        wc->DispatchKeyDownEvent((LWE::KeyValue)ev.key);
    } else if (t == "DispatchKeyPressEvent") {
        wc->DispatchKeyPressEvent((LWE::KeyValue)ev.key);
    } else if (t == "DispatchKeyUpEvent") {
        wc->DispatchKeyUpEvent((LWE::KeyValue)ev.key);
    } else if (t == "DispatchCompositionStartEvent") {
        wc->DispatchCompositionStartEvent(ev.strArg);
    } else if (t == "DispatchCompositionUpdateEvent") {
        wc->DispatchCompositionUpdateEvent(ev.strArg);
    } else if (t == "DispatchCompositionEndEvent") {
        wc->DispatchCompositionEndEvent(ev.strArg);
    } else if (t == "EvaluateJavaScript") {
        wc->EvaluateJavaScript(ev.strArg);
    } else if (t == "AddJavaScriptInterface") {
        // The original native callback was not serialized; register an echo
        // stub so the JS object/function exists. Return values won't match.
        wc->AddJavaScriptInterface(
            ev.strArg, ev.strArg2,
            [](const std::string& param) -> std::string { return param; });
    } else if (t == "RemoveJavascriptInterface") {
        wc->RemoveJavascriptInterface(ev.strArg, ev.strArg2);
    } else if (t == "SetUserAgentString") {
        wc->SetUserAgentString(ev.strArg);
    } else if (t == "SetCacheMode") {
        wc->SetCacheMode(ev.intArg);
    } else if (t == "SetDefaultFontSize") {
        wc->SetDefaultFontSize((uint32_t)ev.intArg);
    } else if (t == "SetSettings") {
        LWE::Settings settings;
        for (const auto& kv : ev.settings)
            settings.UpdateSetting(kv.first, kv.second);
        wc->SetSettings(settings);
    } else if (t == "ScrollTo") {
        wc->ScrollTo(ev.iX, ev.iY);
    } else if (t == "ScrollBy") {
        wc->ScrollBy(ev.iX, ev.iY);
    } else if (t == "SetDevicePixelRatio") {
        wc->SetDevicePixelRatio(ev.dpr);
    } else {
        fprintf(stderr, "[APIReplayer] Unknown event type: %s\n", t.c_str());
    }
}

// ── APIReplayer::startReplay
// ────────────────────────────────────────────────── Schedules all events
// upfront using AddTimeout so the event loop is never blocked. Each event's
// delay is calculated relative to the moment this function is called (which
// closely matches when the WebContainer was created).

struct ReplayContext {
    LWE::WebContainer* wc;
    ReplayEvent event;
};

void APIReplayer::startReplay(LWE::WebContainer* wc, float speedFactor)
{
    uint64_t scheduleStartUs = replayCurrentMicros();

    for (const ReplayEvent& ev : m_events) {
        uint64_t targetUs =
            speedFactor > 0.0f
                ? (uint64_t)((double)ev.tsUs / (double)speedFactor)
                : ev.tsUs;

        uint64_t elapsedUs = replayCurrentMicros() - scheduleStartUs;
        size_t delayMs = (targetUs > elapsedUs)
                             ? (size_t)((targetUs - elapsedUs) / 1000)
                             : 0;

        auto* ctx = new ReplayContext{ wc, ev };
        wc->AddTimeout(
            [](void* data) {
                auto* ctx = static_cast<ReplayContext*>(data);
                dispatchEvent(ctx->wc, ctx->event);
                delete ctx;
            },
            ctx, delayMs);
    }

    fprintf(stderr, "[APIReplayer] Scheduled %zu events (speed: %.1fx)\n",
            m_events.size(), (double)speedFactor);
}

} // namespace StarfishShell

#endif // STARFISH_ENABLE_TEST
