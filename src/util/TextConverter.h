#ifndef __StarFishTextConverter__
#define __StarFishTextConverter__

#include "util/String.h"

namespace StarFish {

class TextConverter : public gc {
public:
    TextConverter(String* charsetName);
    TextConverter(String* mimetype, String* preferredEncoding,
                  const char* bytes, size_t len);
    ~TextConverter();
    String* convert(const char* bytes, size_t len, bool isEndOfStream);
    String* encoding()
    {
        return m_encoding;
    }

protected:
    void registerFinalizer();
    UConverter* m_converter;
    String* m_encoding;
    std::basic_string<char, std::char_traits<char>,
                      gc_allocator_ignore_off_page<char>>
        m_bufferToConvert;
};
}

#endif
