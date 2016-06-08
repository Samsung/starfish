#ifndef __StarFishFrameInline__
#define __StarFishFrameInline__

#include "layout/Frame.h"

namespace StarFish {

class FrameInline : public Frame {
public:
    FrameInline(Node* node)
        : Frame(node, nullptr)
    {

    }

    virtual bool isFrameInline()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameInline";
    }
#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth)
    {
        Frame::dump(depth);
    }
#endif

protected:
};

}

#endif
