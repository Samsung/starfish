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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPInputDomain__)
#define __StarfishCDPInputDomain__

#include "../CDPDomain.h"
#include <string>

namespace Starfish {

class CDPDispatcher;
class CDPCommand;

// Input domain (MVP). dispatchMouseEvent / dispatchKeyEvent / insertText.
// Handlers run on the main thread and call WebView input dispatch directly.
class InputDomain : public CDPDomain {
public:
    InputDomain(CDPDispatcher* d)
        : m_dispatcher(d)
    {
    }
    void processMessage(CDPCommand& cmd, const std::string& method) override;

private:
    void handleDispatchMouseEvent(CDPCommand& cmd, const std::string& method);
    void handleDispatchKeyEvent(CDPCommand& cmd, const std::string& method);
    void handleInsertText(CDPCommand& cmd, const std::string& method);
    void handleImeSetComposition(CDPCommand& cmd, const std::string& method);
    void handleDispatchTouchEvent(CDPCommand& cmd, const std::string& method);
    void handleSynthesizeScrollGesture(CDPCommand& cmd,
                                       const std::string& method);
    void handleSynthesizeTapGesture(CDPCommand& cmd, const std::string& method);
    void handleSynthesizePinchGesture(CDPCommand& cmd,
                                      const std::string& method);
    void handleDispatchDragEvent(CDPCommand& cmd, const std::string& method);

    CDPDispatcher* m_dispatcher;
};

} // namespace Starfish

#endif
