/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifndef __StarFishMessages__
#define __StarFishMessages__

namespace StarFish {
// Reasons
#define CALLED_CONSTRUCTOR_WITHOUT_NEW "Constructor '%s' requires 'new'"
#define FAILED_TO_CONSTRUCT "Failed to construct '%s': %s"
#define FAILED_TO_EXECUTE "Failed to execute '%s' on '%s': %s"
#define FAILED_TO_SET_PROPERTY "Failed to set the '%s' property on '%s': %s"
#define ILLEGAL_INVOKE "Illegal invocation"

// Details
#define ARGS_NOT_ENOUGH "needs %s parameter, but only %s present."
#define ARG_TYPE_IS_NONFINITE "The provided double value is non-finite"
#define ARG_TYPE_MISMATCH "parameter %s ('%s') is not a(n) %s."
#define ARG_TYPE_MISMATCH_2 \
    "parameter %s ('%s') must be either a '%s' or '%s' element."
#define ARG_TYPE_MISMATCH_WITH_INDEXABLE_TYPE                                \
    "The parameter %s ('%s') is neither an array, nor does it have indexed " \
    "properties."
#define ARG_TYPE_MISMATCH_WITH_ENUM \
    "The provided value is not a valid enum value of type %s."
#define SIGNATURE_NOT_FOUND \
    "No function was found that matched the signature provided."
#define QUERY_SELECTOR_IS_EMPTY "The provided selector is empty."
#define INVALID_SIZE "The value provided %s, which is an invalid size."
#define INVALID_TARGET_ORIGIN "Invalid target origin '%s' in a call to '%s'"
#define INVALID_DATA_CLONE "'%s' could not be cloned."
#define ORIGINS_ARE_NOT_MATCHED                                               \
    "The target origin provided('%s') does not match the recipient window's " \
    "origin('%s')"
#define NOT_POSITIVE "The value provided (%s) is not positive or 0."
#define EXCEED_MIN_BOUNDARY \
    "The value provided (%s) is less than the minimum boundary (%s)."
#define EXCEED_MAX_BOUNDARY \
    "The value provided (%s) is greater than the maximum boundary (%s)."

size_t bufferSize(std::initializer_list<const char*> args);

#define COMPOSE_MESSAGE(MSG, TEMPLATE_STR, ...)                  \
    size_t MSG##siz = bufferSize({ TEMPLATE_STR, __VA_ARGS__ }); \
    std::unique_ptr<char[]> MSG##buf(new char[MSG##siz + 1]);    \
    char* MSG = MSG##buf.get();                                  \
    snprintf(MSG, MSG##siz + 1, TEMPLATE_STR, ##__VA_ARGS__)
}

#endif
