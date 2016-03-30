#ifndef __StarFishQualifiedName__
#define __StarFishQualifiedName__

namespace StarFish {

class StarFish;

class QualifiedName {
    friend class StarFish;
    friend class StaticStrings;
    QualifiedName()
    {
        m_string = nullptr;
    }
    QualifiedName(String* str)
    {
        m_string = str;
    }

public:
    static QualifiedName fromString(StarFish* sf, String* str);
    static QualifiedName fromString(StarFish* sf, const char* str);

    static QualifiedName emptyQualifiedName()
    {
        return QualifiedName(String::emptyString);
    }

    static bool checkNameProductionRule(String* str, unsigned length);

    bool operator==(const QualifiedName& src)
    {
        return m_string == src.m_string;
    }

    String* string() const
    {
        return m_string;
    }

    String* stringConst() const
    {
        return m_string;
    }

    operator String*()
    {
        return m_string;
    }

protected:
    String* m_string;
    // TODO add namespace..
};
}

namespace std {
template <>
struct hash<StarFish::QualifiedName> {
    size_t operator()(const StarFish::QualifiedName& qn) const
    {
        return hash<StarFish::String*>()(qn.stringConst());
    }
};

template <>
struct equal_to<StarFish::QualifiedName> {
    size_t operator()(const StarFish::QualifiedName& lqn, const StarFish::QualifiedName& rqn) const
    {
        return lqn.stringConst() == rqn.stringConst();
    }
};
}
#endif
