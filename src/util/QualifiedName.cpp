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
    if (c == ':' || c == '_')
        return true;

    if (isalpha(c))
        return true;

    if (((c >= 0xC0)&&(c <= 0xD6)) || ((c >= 0xD8)&&(c <= 0xF6)) || ((c >= 0xF8)&&(c <= 0x2FF)))
        return true;

    if (((c >= 0x370)&&(c <= 0x37D)) || ((c >= 0x37F)&&(c <= 0x1FFF)) || ((c >= 0x200C)&&(c <= 0x200D)))
        return true;

    if (((c >= 0x2070)&&(c <= 0x218F)) || ((c >= 0x2C00)&&(c <= 0x2FEF)) || ((c >= 0x3001)&&(c <= 0xD7FF)))
        return true;
    if (((c >= 0xF900)&&(c <= 0xFDCF)) || ((c >= 0xFDF0)&&(c <= 0xFFFD)) || ((c >= 0x10000)&&(c <= 0xEFFFF)))
        return true;

    return false;
}

bool checkNameProductionRule_internal(char32_t c)
{
    // https://www.w3.org/TR/xml/#NT-NameChar
    if (checkNameProductionRuleStart_internal(c))
        return true;

    if (isdigit(c))
        return true;

    if (c == '-' || c == '.' || c == 0xB7 || ((c >= 0x300) && (c <= 0x036F)) || ((c >= 0x203F) && (c <= 0x2040)))
        return true;

    return false;
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
