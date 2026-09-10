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

#include "Snapshot.h"
#include "gtest/gtest.h"

namespace StarfishCLI {

TEST(SnapshotTest, IncludesOnlyReferenceableInteractiveNodes)
{
    const std::string axTree = R"({"nodes":[
        {"ignored":false,"role":{"value":"link"},
         "name":{"value":"Help"},"backendDOMNodeId":10},
        {"ignored":true,"role":{"value":"button"},
         "name":{"value":"Hidden"},"backendDOMNodeId":11},
        {"ignored":false,"role":{"value":"paragraph"},
         "name":{"value":"Text"},"backendDOMNodeId":12},
        {"ignored":false,"role":{"value":"button"},
         "name":{"value":"No reference"}}
    ]})";
    std::string output;
    std::string error;

    EXPECT_TRUE(formatInteractiveSnapshot(axTree, output, error));
    EXPECT_EQ("@e1 [link] \"Help\"\n", output);
    EXPECT_TRUE(error.empty());
}

TEST(SnapshotTest, EscapesTerminalControlCharacters)
{
    const std::string axTree =
        R"({"nodes":[{"ignored":false,"role":{"value":"button"},)"
        R"("name":{"value":"Unsafe\u001b]0;owned\u0007\u009b)"
        R"(31mred\u009d0;title\u009c\"\\"},)"
        R"("backendDOMNodeId":10}]})";
    std::string output;
    std::string error;

    EXPECT_TRUE(formatInteractiveSnapshot(axTree, output, error));
    EXPECT_EQ(
        "@e1 [button] \"Unsafe\\u001b]0;owned\\u0007\\u009b"
        "31mred\\u009d0;title\\u009c\\\"\\\\\"\n",
        output);
}

TEST(SnapshotTest, RejectsResponseWithoutNodes)
{
    std::string output;
    std::string error;

    EXPECT_FALSE(formatInteractiveSnapshot("{}", output, error));
    EXPECT_EQ("unexpected Accessibility.getFullAXTree response", error);
}

} // namespace StarfishCLI
