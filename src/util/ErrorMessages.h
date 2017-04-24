/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#ifndef __StarFishErrorMessages__
#define __StarFishErrorMessages__

#include "StarFishConfig.h"

namespace StarFish {
static const char* SET_NONFINITE_PROPERTY_WHERE_EXPECTED_DOUBLE =
    "Failed to set the '%s' property on '%s': The provided double value is "
    "non-finite.";

size_t bufferSize(std::initializer_list<const char*> args);

#define COMPOSE_ERROR_MESSAGE(TEMPLATE_STR, ...)            \
    size_t siz = bufferSize({ TEMPLATE_STR, __VA_ARGS__ }); \
    char errorMsg[siz + 1];                                 \
    snprintf(errorMsg, siz, TEMPLATE_STR, __VA_ARGS__)
}

#endif
