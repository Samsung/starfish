#ifndef __StarFishAtomicString__
#define __StarFishAtomicString__

namespace StarFish {

class StarFish;

class AtomicString {
    friend class StarFish;
    AtomicString(String* str)
    {
        m_string = str;
    }

public:
    static AtomicString createAtomicString(StarFish* sf, String* str);
    static AtomicString createAtomicString(StarFish* sf, const char* str);
    static AtomicString createAttrAtomicString(StarFish* sf, String* str);
    static AtomicString createAttrAtomicString(StarFish* sf, const char* str);
    static AtomicString emptyAtomicString()
    {
        return AtomicString(String::emptyString);
    }

    bool operator==(const AtomicString& src) const
    {
        return m_string == src.m_string;
    }

    String* string() const
    {
        return m_string;
    }

    operator String*() const
    {
        return m_string;
    }
private:
    String* m_string;
};

}

namespace std {
template <>
struct hash<StarFish::AtomicString> {
    size_t operator()(const StarFish::AtomicString& qn) const
    {
        return hash<StarFish::String*>()(qn.string());
    }
};

template <>
struct equal_to<StarFish::AtomicString> {
    size_t operator()(const StarFish::AtomicString& lqn, const StarFish::AtomicString& rqn) const
    {
        return lqn.string() == rqn.string();
    }
};
}
#endif
