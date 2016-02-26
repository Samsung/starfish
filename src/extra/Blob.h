#ifndef __StarFishBlobObject__
#define __StarFishBlobObject__

#include "dom/binding/ScriptWrappable.h"

// "blob:http%3A//ohgyun.com/a9f2fd30-adf0-43dd-a413-9ee8423bec6e"

namespace StarFish {

class Blob : public ScriptWrappable {
public:
    Blob(uint32_t size, String* type, void* data)
        : ScriptWrappable(this)
    {
        m_size = size;
        m_type = type;
        m_data = data;
        // init
        initScriptWrappable(this);
    }

    void* data()
    {
        return m_data;
    }

    uint32_t size()
    {
        return m_size;
    }

protected:
    uint32_t m_size;
    String* m_type;
    void* m_data;
};
}

#endif
