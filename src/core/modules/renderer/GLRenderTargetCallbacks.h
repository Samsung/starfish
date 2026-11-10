/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version. This library is distributed without any warranty.
 */
#ifndef __StarfishGLRenderTargetCallbacks__
#define __StarfishGLRenderTargetCallbacks__

#include <cstddef>
#include <functional>

namespace Starfish {
struct GLRenderTarget {
    unsigned width;
    unsigned height;
    int clipX;
    int clipY;
    unsigned clipWidth;
    unsigned clipHeight;
    double matrix[9];
    float opacity;
    float color[3];
    unsigned framebuffer;
    bool preserveContents;
};

struct SoftwareRenderTarget {
    void* pixels;
    size_t stride;
    unsigned width;
    unsigned height;
};

struct GLRenderTargetCallbacks {
    std::function<bool(GLRenderTarget&)> acquire;
    std::function<void(bool)> release;
};
struct SoftwareRenderTargetCallbacks {
    std::function<bool(SoftwareRenderTarget&)> acquire;
    std::function<void(bool)> release;
};
} // namespace Starfish

#endif
