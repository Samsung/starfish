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
#include "InputDomain.h"
#include "../CDPDispatcher.h"
#include "../CDPCommand.h"
#include "core/page/WebView.h"
#include "core/modules/renderer/Renderer.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/Touch.h"
#include "platform/event/PlatformKeyEventData.h"

#include "rapidjson/document.h"

#include <cmath>
#include <string>
#include <vector>

using namespace LWE;

namespace Starfish {

static std::string paramString(CDPCommand& cmd, const char* name)
{
    if (cmd.params() && cmd.params()->HasMember(name) &&
        (*cmd.params())[name].IsString()) {
        return (*cmd.params())[name].GetString();
    }
    return std::string();
}

static double paramNumber(CDPCommand& cmd, const char* name, double def)
{
    if (cmd.params() && cmd.params()->HasMember(name) &&
        (*cmd.params())[name].IsNumber()) {
        return (*cmd.params())[name].GetDouble();
    }
    return def;
}

// CDP mouse button name -> Starfish button / buttons-mask.
static MouseButtonValue cdpButton(const std::string& b)
{
    if (b == "right") {
        return MouseButtonValue::RightButton;
    }
    if (b == "middle") {
        return MouseButtonValue::MiddleButton;
    }
    // "left", "none", "back", "forward" -> left/none (NoButton == LeftButton ==
    // 0)
    return MouseButtonValue::LeftButton;
}

static MouseButtonsValue cdpButtonsMask(const std::string& b)
{
    if (b == "right") {
        return MouseButtonsValue::RightButtonDown;
    }
    if (b == "middle") {
        return MouseButtonsValue::MiddleButtonDown;
    }
    if (b == "left") {
        return MouseButtonsValue::LeftButtonDown;
    }
    return MouseButtonsValue::NoButtonDown;
}

// Map a CDP key event to a Starfish KeyValue. Printable ASCII is resolved from
// `text` (single char) or single-char `key`; the KeyValue enum is ASCII-aligned
// for 32..126. Named keys (Enter/Backspace/...) map by their `key` name.
static bool cdpKeyToKeyValue(const std::string& key, const std::string& text,
                             KeyValue& out)
{
    // Named keys first.
    if (key == "Enter" || key == "Return") {
        out = KeyValue::EnterKey;
        return true;
    }
    if (key == "Tab") {
        out = KeyValue::TabKey;
        return true;
    }
    if (key == "Backspace") {
        out = KeyValue::BackspaceKey;
        return true;
    }
    if (key == "Delete") {
        out = KeyValue::DeleteKey;
        return true;
    }
    if (key == "ArrowLeft") {
        out = KeyValue::ArrowLeftKey;
        return true;
    }
    if (key == "ArrowRight") {
        out = KeyValue::ArrowRightKey;
        return true;
    }
    if (key == "ArrowUp") {
        out = KeyValue::ArrowUpKey;
        return true;
    }
    if (key == "ArrowDown") {
        out = KeyValue::ArrowDownKey;
        return true;
    }
    if (key == "Home") {
        out = KeyValue::HomeKey;
        return true;
    }
    if (key == "End") {
        out = KeyValue::EndKey;
        return true;
    }
    if (key == "Escape") {
        out = KeyValue::EscapeKey;
        return true;
    }
    if (key == " " || key == "Space") {
        out = KeyValue::SpaceKey;
        return true;
    }

    // Printable single character: prefer text, fall back to key.
    char c = 0;
    if (text.size() == 1) {
        c = text[0];
    } else if (key.size() == 1) {
        c = key[0];
    }
    if (c >= 32 && c <= 126) {
        out = static_cast<KeyValue>((unsigned char)c);
        return true;
    }
    return false;
}

void InputDomain::processMessage(CDPCommand& cmd, const std::string& method)
{
    WebView* wv = m_dispatcher->webView();
    if (!wv) {
        cmd.sendError(-32000, "No WebView");
        return;
    }

    if (method == "dispatchMouseEvent") {
        std::string type = paramString(cmd, "type");
        double x = paramNumber(cmd, "x", 0);
        double y = paramNumber(cmd, "y", 0);
        std::string button = paramString(cmd, "button");
        int clickCount = (int)paramNumber(cmd, "clickCount", 0);

        if (type == "mousePressed") {
            wv->dispatchMouseEvent(MouseEventKind::MouseEventDown,
                                   MouseData(cdpButton(button),
                                             cdpButtonsMask(button), x, y,
                                             clickCount > 0 ? clickCount : 1));
            cmd.sendResultEmpty();
            return;
        }
        if (type == "mouseReleased") {
            wv->dispatchMouseEvent(
                MouseEventKind::MouseEventUp,
                MouseData(cdpButton(button), MouseButtonsValue::NoButtonDown, x,
                          y, clickCount > 0 ? clickCount : 1));
            cmd.sendResultEmpty();
            return;
        }
        if (type == "mouseMoved") {
            wv->dispatchMouseEvent(MouseEventKind::MouseEventMove,
                                   MouseData(MouseButtonValue::NoButton,
                                             cdpButtonsMask(button), x, y, 0));
            cmd.sendResultEmpty();
            return;
        }
        if (type == "mouseWheel") {
            double deltaX = paramNumber(cmd, "deltaX", 0);
            double deltaY = paramNumber(cmd, "deltaY", 0);
            // Starfish takes z=-1/1 and a vertical flag. Pick the dominant
            // axis.
            if (deltaY != 0) {
                wv->dispatchMouseWheelEvent((float)x, (float)y,
                                            deltaY > 0 ? 1 : -1, true);
            } else if (deltaX != 0) {
                wv->dispatchMouseWheelEvent((float)x, (float)y,
                                            deltaX > 0 ? 1 : -1, false);
            }
            cmd.sendResultEmpty();
            return;
        }
        cmd.sendResultEmpty();
        return;
    }

    if (method == "dispatchKeyEvent") {
        std::string type = paramString(cmd, "type");
        std::string key = paramString(cmd, "key");
        std::string text = paramString(cmd, "text");

        KeyValue kv;
        if (!cdpKeyToKeyValue(key, text, kv)) {
            // Unmappable key (e.g. modifier-only); ack without dispatch.
            cmd.sendResultEmpty();
            return;
        }
        PlatformKeyEventData data(kv);

        // keyDown/rawKeyDown -> KeyEventDown (this is where editable text is
        // inserted and the input event fires); keyUp -> KeyEventUp; char ->
        // KeyEventPress.
        if (type == "keyDown" || type == "rawKeyDown") {
            wv->dispatchKeyEvent(KeyEventKind::KeyEventDown, data);
        } else if (type == "keyUp") {
            wv->dispatchKeyEvent(KeyEventKind::KeyEventUp, data);
        } else if (type == "char") {
            wv->dispatchKeyEvent(KeyEventKind::KeyEventPress, data);
        }
        cmd.sendResultEmpty();
        return;
    }

    if (method == "insertText") {
        std::string text = paramString(cmd, "text");
        // Insert each printable ASCII char via a keyDown (the editable-text
        // insertion path). Non-ASCII is dropped in this MVP.
        for (size_t i = 0; i < text.size(); i++) {
            char c = text[i];
            if (c >= 32 && c <= 126) {
                PlatformKeyEventData data(
                    static_cast<KeyValue>((unsigned char)c));
                wv->dispatchKeyEvent(KeyEventKind::KeyEventDown, data);
            }
        }
        cmd.sendResultEmpty();
        return;
    }

    if (method == "imeSetComposition") {
        // Set the (unconfirmed) IME composition / preedit string on the focused
        // editable. Starfish drives composition through a single
        // CompositionEventUpdate: a non-empty string inserts the preedit text
        // and fires `compositionupdate`; an empty string ends the preedit
        // (clearing it). Commit of the composed text is done by the caller via
        // Input.insertText (or a dispatchKeyEvent), matching the CDP contract
        // where imeSetComposition only sets the in-progress composition.
        std::string text = paramString(cmd, "text");
        // selectionStart/End and replacementStart/End are part of the CDP
        // payload but Starfish's composition path has no
        // caret-range/replacement hook, so they are accepted and ignored.
        wv->dispatchCompositionEvent(
            CompositionEventKind::CompositionEventUpdate,
            String::fromUTF8(text.data(), text.length()), nullptr);
        cmd.sendResultEmpty();
        return;
    }

    if (method == "dispatchTouchEvent") {
        std::string type = paramString(cmd, "type");

        TouchEventKind kind;
        if (type == "touchStart") {
            kind = TouchEventKind::TouchEventStart;
        } else if (type == "touchEnd") {
            kind = TouchEventKind::TouchEventEnd;
        } else if (type == "touchMove") {
            kind = TouchEventKind::TouchEventMove;
        } else if (type == "touchCancel") {
            kind = TouchEventKind::TouchEventCancel;
        } else {
            cmd.sendResultEmpty();
            return;
        }

        // touchCancel carries no points; release active/hovered state.
        if (kind == TouchEventKind::TouchEventCancel) {
            TouchData none;
            wv->dispatchTouchEvent(kind, &none, 0);
            cmd.sendResultEmpty();
            return;
        }

        // Build TouchData from CDP touchPoints[]. clientX/Y == screenX/Y here
        // (no separate screen offset in this MVP). touchEnd's touchPoints holds
        // the points that were lifted; if the client sends an empty array on
        // touchEnd (CDP allows it), fall back to the last start position is not
        // tracked here -- require at least one point.
        std::vector<TouchData> points;
        if (cmd.params() && cmd.params()->HasMember("touchPoints") &&
            (*cmd.params())["touchPoints"].IsArray()) {
            const rapidjson::Value& arr = (*cmd.params())["touchPoints"];
            for (rapidjson::SizeType i = 0; i < arr.Size(); i++) {
                const rapidjson::Value& p = arr[i];
                double x = 0, y = 0;
                if (p.HasMember("x") && p["x"].IsNumber()) {
                    x = p["x"].GetDouble();
                }
                if (p.HasMember("y") && p["y"].IsNumber()) {
                    y = p["y"].GetDouble();
                }
                points.push_back(TouchData(x, y, x, y));
            }
        }

        if (points.empty()) {
            // No points to dispatch (e.g. touchEnd with empty array). Ack so
            // puppeteer's tap sequence completes without hanging.
            cmd.sendResultEmpty();
            return;
        }

        wv->dispatchTouchEvent(kind, points.data(), points.size());
        cmd.sendResultEmpty();
        return;
    }

    if (method == "synthesizeScrollGesture") {
        // Scroll by (xDistance, yDistance) starting at (x, y). CDP sign
        // convention: a positive yDistance means the finger/content moves up,
        // i.e. the page scrolls down and window.scrollY increases. Starfish's
        // wheel path moves scrollTop by z*30 per step (z=+1 => scrollTop
        // increases), so positive yDistance maps to z=+1. We emit one wheel
        // step per 30px so the total scroll is proportional to the distance.
        double x = paramNumber(cmd, "x", 0);
        double y = paramNumber(cmd, "y", 0);
        double xDistance = paramNumber(cmd, "xDistance", 0);
        double yDistance = paramNumber(cmd, "yDistance", 0);

        const double kStep = 30.0; // pixels per wheel step (matches engine)
        if (yDistance != 0) {
            int z = yDistance > 0 ? 1 : -1;
            int steps = (int)(std::abs(yDistance) / kStep);
            if (steps < 1) {
                steps = 1;
            }
            for (int i = 0; i < steps; i++) {
                wv->dispatchMouseWheelEvent((float)x, (float)y, z, true);
            }
        }
        if (xDistance != 0) {
            int z = xDistance > 0 ? 1 : -1;
            int steps = (int)(std::abs(xDistance) / kStep);
            if (steps < 1) {
                steps = 1;
            }
            for (int i = 0; i < steps; i++) {
                wv->dispatchMouseWheelEvent((float)x, (float)y, z, false);
            }
        }
        cmd.sendResultEmpty();
        return;
    }

    if (method == "synthesizeTapGesture") {
        // Single tap at (x, y): a touchStart immediately followed by a touchEnd
        // at the same point, reusing the touch dispatch path. This fires
        // touchstart/touchend and the synthesized click. tapCount/duration are
        // accepted; tapCount repeats the start/end pair.
        double x = paramNumber(cmd, "x", 0);
        double y = paramNumber(cmd, "y", 0);
        int tapCount = (int)paramNumber(cmd, "tapCount", 1);
        if (tapCount < 1) {
            tapCount = 1;
        }
        for (int i = 0; i < tapCount; i++) {
            TouchData point(x, y, x, y);
            wv->dispatchTouchEvent(TouchEventKind::TouchEventStart, &point, 1);
            wv->dispatchTouchEvent(TouchEventKind::TouchEventEnd, &point, 1);
        }
        cmd.sendResultEmpty();
        return;
    }

    if (method == "synthesizePinchGesture") {
        // The engine has no pinch-zoom / multi-touch scaling input path
        // reachable from here, so a real pinch cannot be delivered to the page.
        // Accept the command and ack (no-op) so callers that issue a pinch do
        // not error out. x/y/scaleFactor are ignored.
        cmd.sendResultEmpty();
        return;
    }

    if (method == "dispatchDragEvent") {
        // The engine does not implement HTML5 DragEvent / DataTransfer
        // (Document::createEvent rejects "DragEvent" as unsupported), so a real
        // drag/drop sequence cannot be delivered to the page. Best effort: move
        // the synthetic pointer to the target coordinates (so any pointer-based
        // observation tracks the drag position) and ack.
        // drop/dragEnter/dragOver/ dragCancel all collapse to a pointer move;
        // data[] is ignored.
        double x = paramNumber(cmd, "x", 0);
        double y = paramNumber(cmd, "y", 0);
        wv->dispatchMouseEvent(MouseEventKind::MouseEventMove,
                               MouseData(MouseButtonValue::NoButton,
                                         MouseButtonsValue::LeftButtonDown, x,
                                         y, 0));
        cmd.sendResultEmpty();
        return;
    }

    // setIgnoreInputEvents / etc. accepted as no-ops.
    cmd.sendResultEmpty();
}

} // namespace Starfish

#endif
