/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License, version 2.1 or
 * (at your option) any later version.
 */
#ifndef __LWEPlatformImageSoftwareAdapter__
#define __LWEPlatformImageSoftwareAdapter__

#if defined(STARFISH_ENABLE_TBM_PLATFORM_IMAGE)
#include <tbm_surface.h>
#endif

namespace LWEDelegate {
// Compatibility adapter for the earlier TBM-handle software API.
class PlatformImageSoftwareAdapter {
public:
    ~PlatformImageSoftwareAdapter()
    {
        unmap();
    }

    Starfish::SoftwareRenderTarget map(void* image)
    {
        unmap();
        Starfish::SoftwareRenderTarget result{};
#if defined(STARFISH_ENABLE_TBM_PLATFORM_IMAGE)
        tbm_surface_info_s info{};
        if (image && tbm_surface_map(static_cast<tbm_surface_h>(image),
                                     TBM_SURF_OPTION_WRITE,
                                     &info) == TBM_SURFACE_ERROR_NONE) {
            result.pixels = info.planes[0].ptr;
            result.stride = info.planes[0].stride;
            result.width = info.width;
            result.height = info.height;
            m_surface = image;
        }
#endif
        return result;
    }

    void unmap()
    {
#if defined(STARFISH_ENABLE_TBM_PLATFORM_IMAGE)
        if (m_surface) {
            tbm_surface_unmap(static_cast<tbm_surface_h>(m_surface));
            m_surface = nullptr;
        }
#endif
    }

private:
#if defined(STARFISH_ENABLE_TBM_PLATFORM_IMAGE)
    void* m_surface = nullptr;
#endif
};
} // namespace LWEDelegate
#endif
