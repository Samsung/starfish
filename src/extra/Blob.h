#ifndef __StarFishBlobObject__
#define __StarFishBlobObject__

#include "dom/binding/ScriptWrappable.h"

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
