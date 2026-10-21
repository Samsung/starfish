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

#include <vector>

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
    std::vector<int> nodeIds;

    EXPECT_TRUE(formatInteractiveSnapshot(axTree, output, error, nodeIds));
    EXPECT_EQ("@e1 [link] \"Help\"\n", output);
    ASSERT_EQ(1u, nodeIds.size());
    EXPECT_EQ(10, nodeIds[0]);
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
    std::vector<int> nodeIds;

    EXPECT_TRUE(formatInteractiveSnapshot(axTree, output, error, nodeIds));
    EXPECT_EQ(
        "@e1 [button] \"Unsafe\\u001b]0;owned\\u0007\\u009b"
        "31mred\\u009d0;title\\u009c\\\"\\\\\"\n",
        output);
    ASSERT_EQ(1u, nodeIds.size());
    EXPECT_EQ(10, nodeIds[0]);
}

namespace {

    std::string formatOne(const std::string& extra)
    {
        const std::string axTree =
            R"({"nodes":[{"ignored":false,"role":{"value":"button"},)"
            R"("name":{"value":"Name"},"backendDOMNodeId":10)" +
            extra + "}]}";
        std::string output;
        std::string error;
        std::vector<int> nodeIds;
        EXPECT_TRUE(formatInteractiveSnapshot(axTree, output, error, nodeIds));
        return output;
    }

} // namespace

TEST(SnapshotTest, PrintsStatesInFixedOrder)
{
    EXPECT_EQ(
        "@e1 [button] \"Name\" [level=3, checked=mixed, expanded=false, "
        "selected, disabled, required]\n",
        formatOne(R"(,"properties":[
            {"name":"required","value":{"type":"boolean","value":true}},
            {"name":"disabled","value":{"type":"boolean","value":true}},
            {"name":"selected","value":{"type":"booleanOrUndefined","value":true}},
            {"name":"expanded","value":{"type":"booleanOrUndefined","value":false}},
            {"name":"checked","value":{"type":"tristate","value":"mixed"}},
            {"name":"level","value":{"type":"integer","value":3}}])"));
}

TEST(SnapshotTest, OmitsFalseStatesAndOtherProperties)
{
    EXPECT_EQ("@e1 [button] \"Name\" [checked=false, expanded=true]\n",
              formatOne(R"(,"properties":[
            {"name":"checked","value":{"type":"tristate","value":"false"}},
            {"name":"expanded","value":{"type":"booleanOrUndefined","value":true}},
            {"name":"selected","value":{"type":"booleanOrUndefined","value":false}},
            {"name":"disabled","value":{"type":"boolean","value":false}},
            {"name":"required","value":{"type":"boolean","value":false}},
            {"name":"readonly","value":{"type":"boolean","value":true}},
            {"name":"focusable","value":{"type":"boolean","value":true}}])"));
    EXPECT_EQ("@e1 [button] \"Name\"\n",
              formatOne(R"(,"properties":[
            {"name":"readonly","value":{"type":"boolean","value":true}}])"));
}

TEST(SnapshotTest, AppendsValueOnlyWhenUseful)
{
    EXPECT_EQ("@e1 [button] \"Name\": old\n",
              formatOne(R"(,"value":{"type":"string","value":"old"})"));
    EXPECT_EQ("@e1 [button] \"Name\"\n",
              formatOne(R"(,"value":{"type":"string","value":"Name"})"));
    EXPECT_EQ("@e1 [button] \"Name\"\n",
              formatOne(R"(,"value":{"type":"string","value":""})"));
    EXPECT_EQ("@e1 [button] \"Name\"\n",
              formatOne(R"(,"value":{"type":"boolean","value":true})"));
    EXPECT_EQ("@e1 [button] \"Name\": 5\n",
              formatOne(R"(,"value":{"type":"number","value":5.0})"));
    EXPECT_EQ("@e1 [button] \"Name\": 2.5\n",
              formatOne(R"(,"value":{"type":"number","value":2.5})"));
    EXPECT_EQ("@e1 [button] \"Name\" [disabled]: 7\n",
              formatOne(R"(,"properties":[
            {"name":"disabled","value":{"type":"boolean","value":true}}],
            "value":{"type":"number","value":7})"));
}

TEST(SnapshotTest, RejectsResponseWithoutNodes)
{
    std::string output;
    std::string error;
    std::vector<int> nodeIds;

    EXPECT_FALSE(formatInteractiveSnapshot("{}", output, error, nodeIds));
    EXPECT_EQ("unexpected Accessibility.getFullAXTree response", error);
}

} // namespace StarfishCLI
