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

#ifndef __StarFishMessages__
#define __StarFishMessages__

namespace StarFish {
// Reasons
static const char* CALLED_CONSTRUCTOR_WITHOUT_NEW =
    "Constructor '%s' requires 'new'";
static const char* FAILED_TO_CONSTRUCT = "Failed to construct '%s': %s";
static const char* FAILED_TO_EXECUTE = "Failed to execute '%s' on '%s': %s";
static const char* FAILED_TO_SET_PROPERTY =
    "Failed to set the '%s' property on '%s': %s";
static const char* ILLEGAL_INVOKE = "Illegal invocation";

// Details
static const char* ARGS_NOT_ENOUGH = "needs %s parameter, but only %s present.";
static const char* ARG_TYPE_IS_NONFINITE =
    "The provided double value is non-finite";
static const char* ARG_TYPE_MISMATCH = "parameter %s ('%s') is not a(n) %s.";
static const char* ARG_TYPE_MISMATCH_2 =
    "parameter %s ('%s') must be either a '%s' or '%s' element.";
static const char* ARG_TYPE_MISMATCH_WITH_INDEXABLE_TYPE =
    "The parameter %s ('%s') is neither an array, nor does it have indexed "
    "properties.";
static const char* ARG_TYPE_MISMATCH_WITH_ENUM =
    "The provided value is not a valid enum value of type %s.";
static const char* SIGNATURE_NOT_FOUND =
    "No function was found that matched the signature provided.";
static const char* QUERY_SELECTOR_IS_EMPTY = "The provided selector is empty.";
static const char* INVALID_SIZE =
    "The value provided %s, which is an invalid size.";
static const char* INVALID_TARGET_ORIGIN =
    "Invalid target origin '%s' in a call to '%s'";
static const char* INVALID_DATA_CLONE = "'%s' could not be cloned.";
static const char* ORIGINS_ARE_NOT_MATCHED =
    "The target origin provided ('%s')"
    "does not match the recipient window's origin('%s')";

size_t bufferSize(std::initializer_list<const char*> args);

#define COMPOSE_MESSAGE(MSG, TEMPLATE_STR, ...)                  \
    size_t MSG##siz = bufferSize({ TEMPLATE_STR, __VA_ARGS__ }); \
    char MSG[MSG##siz + 1];                                      \
    snprintf(MSG, MSG##siz + 1, TEMPLATE_STR, __VA_ARGS__)
}

#endif
