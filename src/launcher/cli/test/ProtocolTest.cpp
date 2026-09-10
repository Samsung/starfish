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

#include "Protocol.h"
#include "gtest/gtest.h"

namespace StarfishCLI {

TEST(ProtocolTest, MakesAndParsesOpenRequest)
{
    Request request;

    EXPECT_TRUE(parseRequest(makeOpenRequest("https://example.com"), request));
    EXPECT_EQ("open", request.command);
    EXPECT_EQ("https://example.com", request.url);
}

TEST(ProtocolTest, MakesAndParsesSuccessfulSnapshotResponse)
{
    Response response;

    EXPECT_TRUE(
        parseResponse(makeOkResponse("@e1 [button] \"Submit\"\n"), response));
    EXPECT_TRUE(response.isOk);
    EXPECT_EQ("@e1 [button] \"Submit\"\n", response.result);
    EXPECT_TRUE(response.error.empty());
}

} // namespace StarfishCLI
