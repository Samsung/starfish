/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License, version 2.1 or
 * (at your option) any later version.
 */
#ifndef __LWEPlatformImageGLAdapter__
#define __LWEPlatformImageGLAdapter__

#if defined(STARFISH_ENABLE_TBM_PLATFORM_IMAGE)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <tbm_bufmgr.h>
#include <tbm_surface.h>
#include <tbm_surface_internal.h>

#include <cstring>
#include <vector>

#ifndef EGL_DMA_BUF_PLANE3_FD_EXT
#define EGL_DMA_BUF_PLANE3_FD_EXT 0x3440
#endif
#ifndef EGL_DMA_BUF_PLANE3_OFFSET_EXT
#define EGL_DMA_BUF_PLANE3_OFFSET_EXT 0x3441
#endif
#ifndef EGL_DMA_BUF_PLANE3_PITCH_EXT
#define EGL_DMA_BUF_PLANE3_PITCH_EXT 0x3442
#endif

namespace LWEDelegate {
// Compatibility adapter for the older API that accepts a TBM handle.
class PlatformImageGLAdapter {
public:
    bool acquire(tbm_surface_h surface, Starfish::GLRenderTarget& target)
    {
        if (!surface || m_image != EGL_NO_IMAGE_KHR) {
            return false;
        }
        tbm_surface_info_s info{};
        if (tbm_surface_get_info(surface, &info) != TBM_SURFACE_ERROR_NONE) {
            return false;
        }
        m_display = eglGetCurrentDisplay();
        auto createImage = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(
            eglGetProcAddress("eglCreateImageKHR"));
        auto imageTarget =
            reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(
                eglGetProcAddress("glEGLImageTargetTexture2DOES"));
        if (m_display == EGL_NO_DISPLAY || !createImage || !imageTarget) {
            return false;
        }
        const char* extensions = eglQueryString(m_display, EGL_EXTENSIONS);
        if (hasExtension(extensions, "EGL_TIZEN_image_native_surface")) {
            const EGLint attrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
                                     EGL_NONE };
            m_image =
                createImage(m_display, EGL_NO_CONTEXT, 0x32A1,
                            reinterpret_cast<EGLClientBuffer>(surface), attrs);
        } else if (hasExtension(extensions, "EGL_EXT_image_dma_buf_import")) {
            const EGLint fds[] = { EGL_DMA_BUF_PLANE0_FD_EXT,
                                   EGL_DMA_BUF_PLANE1_FD_EXT,
                                   EGL_DMA_BUF_PLANE2_FD_EXT,
                                   EGL_DMA_BUF_PLANE3_FD_EXT };
            const EGLint offsets[] = { EGL_DMA_BUF_PLANE0_OFFSET_EXT,
                                       EGL_DMA_BUF_PLANE1_OFFSET_EXT,
                                       EGL_DMA_BUF_PLANE2_OFFSET_EXT,
                                       EGL_DMA_BUF_PLANE3_OFFSET_EXT };
            const EGLint pitches[] = { EGL_DMA_BUF_PLANE0_PITCH_EXT,
                                       EGL_DMA_BUF_PLANE1_PITCH_EXT,
                                       EGL_DMA_BUF_PLANE2_PITCH_EXT,
                                       EGL_DMA_BUF_PLANE3_PITCH_EXT };
            int planes = tbm_surface_internal_get_num_planes(info.format);
            if (planes < 1 || planes > 4) {
                return false;
            }
            std::vector<EGLint> attrs = { EGL_WIDTH,
                                          static_cast<EGLint>(info.width),
                                          EGL_HEIGHT,
                                          static_cast<EGLint>(info.height),
                                          EGL_LINUX_DRM_FOURCC_EXT,
                                          static_cast<EGLint>(info.format) };
            for (int i = 0; i < planes; ++i) {
                int index = tbm_surface_internal_get_plane_bo_idx(surface, i);
                tbm_bo bo = tbm_surface_internal_get_bo(surface, index);
                if (!bo) {
                    return false;
                }
                int fd = static_cast<int>(reinterpret_cast<intptr_t>(
                    tbm_bo_get_handle(bo, TBM_DEVICE_3D).ptr));
                attrs.insert(attrs.end(),
                             { fds[i], fd, offsets[i],
                               static_cast<EGLint>(info.planes[i].offset),
                               pitches[i],
                               static_cast<EGLint>(info.planes[i].stride) });
            }
            attrs.push_back(EGL_NONE);
            m_image = createImage(m_display, EGL_NO_CONTEXT,
                                  EGL_LINUX_DMA_BUF_EXT, nullptr, attrs.data());
        }
        if (m_image == EGL_NO_IMAGE_KHR) {
            return false;
        }
        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        imageTarget(GL_TEXTURE_2D, m_image);
        glGenFramebuffers(1, &m_framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, m_texture, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
            release(false);
            return false;
        }
        target = {};
        target.width = info.width;
        target.height = info.height;
        target.clipWidth = info.width;
        target.clipHeight = info.height;
        target.opacity = 1;
        target.color[0] = target.color[1] = target.color[2] = 1;
        target.framebuffer = m_framebuffer;
        return true;
    }

    void release(bool rendered)
    {
        if (rendered) {
            glFinish();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (m_framebuffer) {
            glDeleteFramebuffers(1, &m_framebuffer);
        }
        if (m_texture) {
            glDeleteTextures(1, &m_texture);
        }
        m_framebuffer = m_texture = 0;
        if (m_image != EGL_NO_IMAGE_KHR) {
            auto destroy = reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(
                eglGetProcAddress("eglDestroyImageKHR"));
            if (destroy) {
                destroy(m_display, m_image);
            }
            m_image = EGL_NO_IMAGE_KHR;
        }
    }

private:
    static bool hasExtension(const char* extensions, const char* name)
    {
        if (!extensions) {
            return false;
        }
        size_t length = std::strlen(name);
        for (const char* p = extensions; (p = std::strstr(p, name));
             p += length) {
            if ((p == extensions || p[-1] == ' ') &&
                (p[length] == ' ' || !p[length])) {
                return true;
            }
        }
        return false;
    }

    EGLDisplay m_display = EGL_NO_DISPLAY;
    EGLImageKHR m_image = EGL_NO_IMAGE_KHR;
    GLuint m_texture = 0;
    GLuint m_framebuffer = 0;
};
} // namespace LWEDelegate
#endif
#endif
