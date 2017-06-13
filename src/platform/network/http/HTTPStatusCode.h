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

#ifndef __StarFishHTTPStatusCode__
#define __StarFishHTTPStatusCode__

namespace StarFish {
// We only define state codes that currently we need or that we expect to need
// to implement soon. If you need it, please refer to the link below and add it.
// * https://www.iana.org/assignments/http-status-codes/http-status-codes.xhtml

// Informational 100~
// Successful 200~
// Redirection 300~
// Client error 400~
// Server error 500~
#define STARFISH_ENUM_HTTP_STATUS_CODE(F) \
    F(CONTINUE, 100)                      \
    F(SWITCHING_PROTOCOLS, 101)           \
    F(OK, 200)                            \
    F(CREATED, 201)                       \
    F(ACCEPTED, 202)                      \
    F(NON_AUTHORITATIVE_INFORMATION, 203) \
    F(NO_CONTENT, 204)                    \
    F(RESET_CONTENT, 205)                 \
    F(PARTIAL_CONTENT, 206)               \
    F(MULTIPLE_CHOICES, 300)              \
    F(MOVED_PERMANENTLY, 301)             \
    F(FOUND, 302)                         \
    F(SEE_OTHER, 303)                     \
    F(NOT_MODIFIED, 304)                  \
    F(USE_PROXY, 305)                     \
    F(BAD_REQUEST, 400)                   \
    F(UNAUTHORIZED, 401)                  \
    F(PAYMENT_REQUIRED, 402)              \
    F(FORBIDDEN, 403)                     \
    F(NOT_FOUND, 404)                     \
    F(METHOD_NOT_ALLOWED, 405)            \
    F(NOT_ACCEPTABLE, 406)                \
    F(PROXY_AUTHENTICATION_REQUIRED, 407) \
    F(REQUEST_TIMEOUT, 408)               \
    F(CONFLICT, 409)                      \
    F(GONE, 410)                          \
    F(LENGTH_REQUIRED, 411)               \
    F(PRECONDITION_FAILED, 412)           \
    F(PAYLOAD_TOO_LARGE, 413)             \
    F(URI_TOO_LONG, 414)                  \
    F(UNSUPPORTED_MEDIA_TYPE, 415)        \
    F(RANGE_NOT_SATISFIABLE, 416)         \
    F(EXPECTATION_FAILED, 417)            \
    F(INTERNAL_SERVER_ERROR, 500)         \
    F(NOT_IMPLEMENTED, 501)               \
    F(BAD_GATEWAY, 502)                   \
    F(SERVICE_UNAVAILABLE, 503)           \
    F(GATEWAY_TIMEOUT, 504)               \
    F(HTTP_VERSION_NOT_SUPPORTED, 505)

enum HTTPStatusCode {
#define DEFINE_HTTP_STATUS_CODE(name, code) HTTP_STATUS_##name = code,
    STARFISH_ENUM_HTTP_STATUS_CODE(DEFINE_HTTP_STATUS_CODE)
#undef DEFINE_HTTP_STATUS_CODE
};
}

#endif
