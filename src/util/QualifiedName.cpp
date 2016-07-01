/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "QualifiedName.h"
#include "StarFish.h"

namespace StarFish {

bool checkNameProductionRuleStart_internal(char32_t c)
{
    // https://www.w3.org/TR/xml/#NT-NameStartChar
    if ((c >= 0x02BB && c <= 0x02C1) || c == 0x559 || c == 0x6E5 || c == 0x6E6)
        return true;

    if (c == ':' || c == '_')
        return true;

    if (!(U_GET_GC_MASK(c) & (U_GC_LL_MASK | U_GC_LU_MASK | U_GC_LO_MASK | U_GC_LT_MASK | U_GC_NL_MASK)))
        return false;

    if (c >= 0xF900 && c < 0xFFFE)
        return false;

    int type = u_getIntPropertyValue(c, UCHAR_DECOMPOSITION_TYPE);
    if (type == U_DT_FONT || type == U_DT_COMPAT)
        return false;

    return true;
}

bool checkNameProductionRule_internal(char32_t c)
{
    // https://www.w3.org/TR/xml/#NT-NameChar
    if (checkNameProductionRuleStart_internal(c))
        return true;

    if (c == 0x00B7 || c == 0x0387)
        return true;

    if (c == '-' || c == '.')
        return true;

    if (!(U_GET_GC_MASK(c) & (U_GC_M_MASK | U_GC_LM_MASK | U_GC_ND_MASK)))
        return false;

    if (c >= 0xF900 && c < 0xFFFE)
        return false;

    int type = u_getIntPropertyValue(c, UCHAR_DECOMPOSITION_TYPE);
    if (type == U_DT_FONT || type == U_DT_COMPAT)
        return false;

    return true;
}

bool QualifiedName::checkNameProductionRule(String* str, unsigned length)
{
    if (!checkNameProductionRuleStart_internal(str->charAt(0)))
        return false;

    for (unsigned i = 1; i < length; ++i) {
        if (!checkNameProductionRule_internal(str->charAt(i)))
            return false;
    }

    return true;
}


}
