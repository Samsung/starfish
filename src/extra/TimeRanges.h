#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined(__StarFishTimeRanges__)
#define __StarFishTimeRanges__

#include "dom/EventTarget.h"
#include "extra/TimeRange.h"

namespace StarFish {

class TimeRanges : public ScriptWrappable {
public:
    TimeRanges() : ScriptWrappable(this)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual Type type()
    {
        return ScriptWrappable::Type::TimeRangesObject;
    }

    double start(unsigned long idx)
    {
        if (idx < m_list.size()) {
            return m_list[idx].start();
        }
        return DBL_MAX;
    }

    double end(unsigned long idx)
    {
        if (idx < m_list.size()) {
            return m_list[idx].end();
        }
        return DBL_MAX;
    }

    void push_back(TimeRange item)
    {
        m_list.push_back(item);
    }

    void push_back(double start, double end)
    {
        m_list.push_back(TimeRange(start, end));
    }

    unsigned long length()
    {
        return m_list.size();
    }

    TimeRange& at(unsigned long idx)
    {
        STARFISH_ASSERT(idx < m_list.size());
        return m_list[idx];
    }

private:
    GCVector<TimeRange> m_list;
};
}

#endif
