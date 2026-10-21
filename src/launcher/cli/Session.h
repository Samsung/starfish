/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishCLISession__
#define __StarfishCLISession__

#include <string>
#include <vector>

namespace StarfishCLI {

class CDPClient;

class Session {
public:
    explicit Session(CDPClient& client);

    bool setup(std::string& error);
    // output gets the text form.
    // data gets the JSON object for --json.
    // The same holds for the other commands that take these arguments.
    bool open(const std::string& url, std::string& output, std::string& data,
              std::string& error);
    bool snapshotInteractive(std::string& output, std::string& data,
                             std::string& error);
    bool click(int elementRef, std::string& error);
    bool fill(int elementRef, const std::string& text, std::string& error);
    bool evaluate(const std::string& script, std::string& output,
                  std::string& data, std::string& error);

    // True when the last failure was a ref whose node no longer exists.
    bool isStaleRef() const
    {
        return m_isStaleRef;
    }

private:
    bool command(const std::string& method, const std::string& parameters,
                 std::string& result, std::string& error);
    // Finds the node of a ref. A ref is stale when its node was removed, or
    // when the document was replaced after the snapshot. This never looks the
    // element up again by role or name.
    bool resolveElement(int elementRef, std::string& objectId,
                        std::string& error);
    bool failStaleRef(int elementRef, std::string& error);
    bool failRefLookup(int elementRef, const std::string& lookupError,
                       std::string& error);
    bool pageInfo(std::string& url, std::string& title, std::string& error);

    CDPClient& m_client;
    std::string m_sessionId;
    std::string m_targetId;
    std::vector<int> m_snapshotNodeIds;
    // Set by open. The refs of the last snapshot are then stale until the
    // next snapshot.
    bool m_areRefsStale{ false };
    bool m_isStaleRef{ false };
};

} // namespace StarfishCLI

#endif
