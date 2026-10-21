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

#include "Options.h"
#include "Protocol.h"
#include "gtest/gtest.h"

namespace StarfishCLI {

namespace {

    bool scan(std::vector<std::string> arguments, ClientOptions& options,
              ParseFailure& failure)
    {
        return parseClientOptions(arguments, options, failure);
    }

} // namespace

TEST(OptionsTest, AcceptsGlobalOptionsBeforeAndAfterCommand)
{
    ClientOptions before;
    ClientOptions after;
    ParseFailure failure;

    EXPECT_TRUE(scan(
        { "--session", "s1", "--json", "--timeout", "250", "open", "http://x" },
        before, failure));
    EXPECT_TRUE(scan(
        { "open", "http://x", "--timeout", "250", "--json", "--session", "s1" },
        after, failure));
    for (const ClientOptions* options : { &before, &after }) {
        EXPECT_EQ("s1", options->session);
        EXPECT_TRUE(options->isJson);
        EXPECT_TRUE(options->hasTimeout);
        EXPECT_EQ(250, options->timeoutMs);
        EXPECT_EQ((std::vector<std::string>{ "open", "http://x" }),
                  options->arguments);
    }
}

TEST(OptionsTest, UsesDefaultSessionAndNoTimeout)
{
    ClientOptions options;
    ParseFailure failure;

    EXPECT_TRUE(scan({ "close" }, options, failure));
    EXPECT_EQ("default", options.session);
    EXPECT_FALSE(options.isJson);
    EXPECT_FALSE(options.hasTimeout);
}

TEST(OptionsTest, RejectsInvalidSessionNames)
{
    for (const char* name :
         { "", "a b", "a/b", "a.b", "../x", "caf\xc3\xa9" }) {
        ClientOptions options;
        ParseFailure failure;

        EXPECT_FALSE(scan({ "--session", name, "close" }, options, failure))
            << name;
        EXPECT_EQ("invalid_session_name", failure.type);
        EXPECT_EQ(std::string("Invalid session name '") + name +
                      "'. Only alphanumeric characters, hyphens, and "
                      "underscores are allowed.",
                  failure.message);
    }
    EXPECT_TRUE(isValidSessionName("a-B_9"));
}

TEST(OptionsTest, ReportsMissingValueInsteadOfTakingTheCommand)
{
    for (const char* option : { "--session", "--timeout" }) {
        for (const std::vector<std::string>& arguments :
             { std::vector<std::string>{ option },
               std::vector<std::string>{ option, "open", "http://x" },
               std::vector<std::string>{ "open", "http://x", option,
                                         "--json" } }) {
            ClientOptions options;
            ParseFailure failure;

            EXPECT_FALSE(scan(arguments, options, failure));
            EXPECT_EQ("missing_arguments", failure.type);
            EXPECT_EQ(std::string("Missing arguments for: ") + option,
                      failure.message);
        }
    }
}

TEST(OptionsTest, ParsesTimeoutAsPositiveDecimalMilliseconds)
{
    long long milliseconds = 0;

    EXPECT_TRUE(parseTimeout("1", milliseconds));
    EXPECT_EQ(1, milliseconds);
    EXPECT_TRUE(parseTimeout("120000", milliseconds));
    EXPECT_EQ(120000, milliseconds);
    for (const char* text : { "0", "", "-5", "+5", "1.5", "5s", " 5", "0x10",
                              "99999999999999999999" }) {
        EXPECT_FALSE(parseTimeout(text, milliseconds)) << text;
    }

    ClientOptions options;
    ParseFailure failure;
    EXPECT_FALSE(scan({ "--timeout", "5s", "close" }, options, failure));
    EXPECT_EQ("invalid_value", failure.type);
    EXPECT_EQ("--timeout expects a number in ms, got '5s'", failure.message);
}

TEST(OptionsTest, RejectsUnknownOptionBeforeCommandOnly)
{
    ClientOptions options;
    ParseFailure failure;

    EXPECT_FALSE(scan({ "--bogus", "close" }, options, failure));
    EXPECT_EQ("unknown_command", failure.type);
    EXPECT_EQ("Unknown option: --bogus", failure.message);

    // After the command, option-like words belong to the command.
    EXPECT_TRUE(scan({ "eval", "--bogus" }, options, failure));
    EXPECT_EQ((std::vector<std::string>{ "eval", "--bogus" }),
              options.arguments);

    // A bare "--" is an ordinary word. Global options stay global.
    EXPECT_TRUE(scan({ "eval", "--", "1+1" }, options, failure));
    EXPECT_EQ((std::vector<std::string>{ "eval", "--", "1+1" }),
              options.arguments);
    EXPECT_TRUE(scan({ "fill", "@e1", "--json" }, options, failure));
    EXPECT_TRUE(options.isJson);
    EXPECT_EQ((std::vector<std::string>{ "fill", "@e1" }), options.arguments);
    EXPECT_TRUE(scan({ "--", "open", "u" }, options, failure));
    EXPECT_EQ((std::vector<std::string>{ "--", "open", "u" }),
              options.arguments);
}

TEST(OptionsTest, JsonEnvelopesKeepTheirKeyOrder)
{
    EXPECT_EQ(R"({"success":true,"data":{"clicked":"@e1"},"error":null})"
              "\n",
              makeJsonSuccess(R"({"clicked":"@e1"})"));
    EXPECT_EQ(R"({"success":false,"data":null,"error":"boom"})"
              "\n",
              makeJsonFailure("boom", ""));
    EXPECT_EQ(
        R"({"success":false,"data":null,"error":"gone","code":"stale_ref"})"
        "\n",
        makeJsonFailure("gone", "stale_ref"));
    EXPECT_EQ(R"({"error":"bad \"x\"","success":false,"type":"invalid_value"})"
              "\n",
              makeJsonParseFailure("bad \"x\"", "invalid_value"));
}

} // namespace StarfishCLI
