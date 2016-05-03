#include "StarFishConfig.h"
#include "QualifiedName.h"
#include "StarFish.h"

namespace StarFish {

QualifiedName QualifiedName::fromString(StarFish* sf, String* str)
{
    return fromString(sf, str->utf8Data());
}

QualifiedName QualifiedName::fromString(StarFish* sf, const char* str)
{
    std::string data = str;
    std::transform(data.begin(), data.end(), data.begin(), ::tolower);
    std::unordered_map<std::string, QualifiedName,
        std::hash<std::string>, std::equal_to<std::string>, gc_allocator<std::pair<std::string, QualifiedName> > >& map = sf->staticStrings()->m_staticStringMap;
    auto iter = map.find(data);

    if (iter != map.end()) {
        return iter->second;
    }

    String* s = String::fromUTF8(data.c_str());
    QualifiedName name(s);
    map.insert(std::make_pair(data, name));

    return name;
}

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
