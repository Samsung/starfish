#include "StarFishConfig.h"
#include "QualifiedName.h"
#include "StarFish.h"

namespace StarFish
{

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

bool QualifiedName::checkNameProductionRule(const char* str, unsigned length)
{
    if (!checkNameProductionRuleStart_internal(str[0]))
        return false;

    for (unsigned i = 1; i < length; ++i) {
        if (!checkNameProductionRule_internal(str[i]))
            return false;
    }

    return true;
}
bool QualifiedName::checkNameProductionRuleStart_internal(char c)
{
    if (c == ':' || c == '_')
        return true;

    if (isalpha(c))
        return true;

    return false;
}
bool QualifiedName::checkNameProductionRule_internal(char c)
{
    if (checkNameProductionRuleStart_internal(c))
        return true;

    if (isdigit(c))
        return true;

    if (c == '-' || c == '.')
        return true;

    return false;
}
}
