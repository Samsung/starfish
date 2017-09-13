/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __StarFishHTTPStatus__
#define __StarFishHTTPStatus__

namespace StarFish {
// We only define state codes that currently we need or that we expect to need
// to implement soon. If you need it, please refer to the link below and add it.
// * https://www.iana.org/assignments/http-status-codes/http-status-codes.xhtml

// Informational 100~
// Successful 200~
// Redirection 300~
// Client error 400~
// Server error 500~
#define STARFISH_ENUM_HTTP_STATUS(F)                                           \
    F(CONTINUE, 100, "Continue")                                               \
    F(SWITCHING_PROTOCOLS, 101, "Switching Protocols")                         \
    F(PROCESSING, 102, "Processing")                                           \
    F(OK, 200, "OK")                                                           \
    F(CREATED, 201, "Created")                                                 \
    F(ACCEPTED, 202, "Accepted")                                               \
    F(NON_AUTHORITATIVE_INFORMATION, 203, "Non-Authoritative Information")     \
    F(NO_CONTENT, 204, "No Content")                                           \
    F(RESET_CONTENT, 205, "Reset Content")                                     \
    F(PARTIAL_CONTENT, 206, "Partial Content")                                 \
    F(MULTI_STATUS, 207, "Multi-Status")                                       \
    F(ALREADY_REPORTED, 208, "Already Reported")                               \
    F(IM_USED, 226, "IM Used")                                                 \
    F(MULTIPLE_CHOICES, 300, "Multiple Choices")                               \
    F(MOVED_PERMANENTLY, 301, "Moved Permanently")                             \
    F(FOUND, 302, "Found")                                                     \
    F(SEE_OTHER, 303, "See Other")                                             \
    F(NOT_MODIFIED, 304, "Not Modified")                                       \
    F(USE_PROXY, 305, "Use Proxy")                                             \
    F(TEMPORARY_REDIRECT, 307, "Temporary Redirect")                           \
    F(PERMANENT_REDIRECT, 308, "Permanent Redirect")                           \
    F(BAD_REQUEST, 400, "Bad Request")                                         \
    F(UNAUTHORIZED, 401, "Unauthorized")                                       \
    F(PAYMENT_REQUIRED, 402, "Payment Required")                               \
    F(FORBIDDEN, 403, "Forbidden")                                             \
    F(NOT_FOUND, 404, "Not Found")                                             \
    F(METHOD_NOT_ALLOWED, 405, "Method Not Allowed")                           \
    F(NOT_ACCEPTABLE, 406, "Not Acceptable")                                   \
    F(PROXY_AUTHENTICATION_REQUIRED, 407, "Proxy Authentication Required")     \
    F(REQUEST_TIMEOUT, 408, "Request Timeout")                                 \
    F(CONFLICT, 409, "Conflict")                                               \
    F(GONE, 410, "Gone")                                                       \
    F(LENGTH_REQUIRED, 411, "Length Required")                                 \
    F(PRECONDITION_FAILED, 412, "Precondition Failed")                         \
    F(PAYLOAD_TOO_LARGE, 413, "Payload Too Large")                             \
    F(URI_TOO_LONG, 414, "URI Too Long")                                       \
    F(UNSUPPORTED_MEDIA_TYPE, 415, "Unsupported Media Type")                   \
    F(RANGE_NOT_SATISFIABLE, 416, "Range Not Satisfiable")                     \
    F(EXPECTATION_FAILED, 417, "Expectation Failed")                           \
    F(MISDIRECTED_REQUEST, 421, "Misdirected Request")                         \
    F(UNPROCESSABLE_ENTITY, 422, "Unprocessable Entity")                       \
    F(LOCKED, 423, "Locked")                                                   \
    F(FAILED_DEPENDENCY, 424, "Failed Dependency")                             \
    F(UPGRADE_REQUIRED, 426, "Upgrade Required")                               \
    F(PRECONDITION_REQUIRED, 428, "Precondition Required")                     \
    F(TOO_MANY_REQUESTS, 429, "Too Many Requests")                             \
    F(REQUEST_HEADER_FIELDS_TOO_LARGE, 431, "Request Header Fields Too Large") \
    F(UNAVAILABLE_FOR_LEGAL_REASONS, 451, "Unavailable For Legal Reasons")     \
    F(INTERNAL_SERVER_ERROR, 500, "Internal Server Error")                     \
    F(NOT_IMPLEMENTED, 501, "Not Implemented")                                 \
    F(BAD_GATEWAY, 502, "Bad Gateway")                                         \
    F(SERVICE_UNAVAILABLE, 503, "Service Unavailable")                         \
    F(GATEWAY_TIMEOUT, 504, "Gateway Timeout")                                 \
    F(HTTP_VERSION_NOT_SUPPORTED, 505, "HTTP Version Not Supported")           \
    F(VARIANT_ALSO_NEGOTIATES, 506, "Variant Also Negotiates")                 \
    F(INSUFFICIENT_STORAGE, 507, "Insufficient Storage")                       \
    F(LOOP_DETECTED, 508, "Loop Detected")                                     \
    F(NOT_EXTENDED, 510, "Not Extended")                                       \
    F(NETWORK_AUTHENTICATION_REQUIRED, 511, "Network Authentication Required")

enum HTTPStatusCode {
#define DEFINE_HTTP_STATUS_CODE(name, code, text) HTTP_STATUS_##name = code,
    STARFISH_ENUM_HTTP_STATUS(DEFINE_HTTP_STATUS_CODE)
#undef DEFINE_HTTP_STATUS_CODE
};

static String* httpStatusCodeToText(long responseCode)
{
    switch (responseCode) {
#define ADD_CASE_FOR_HTTP_STATUS_TEXT(name, code, text) \
    case code:                                          \
        return String::createASCIIString(text);

        STARFISH_ENUM_HTTP_STATUS(ADD_CASE_FOR_HTTP_STATUS_TEXT)
#undef ADD_CASE_FOR_HTTP_STATUS_TEXT
    default:
        return String::createASCIIString("Unassigned");
    }
}
}

#endif
