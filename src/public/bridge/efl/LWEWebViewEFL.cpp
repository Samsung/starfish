/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "StarfishConfig.h"
#include "PlatformIntegrationData.h"
#include "public/delegate/LWEWebViewDelegateImpl.h"
#include "public/delegate/LWEWebContainerDelegate.h"

#if defined(STARFISH_SHELL_EFL)

#define STARFISH_ENABLE_PROFILE_TIMER

#if defined(STARFISH_UV_CAIRO_GL)
// uv_cairo_gl backend: the engine runs on a dedicated LWE thread and renders
// into TBM buffers via a private raw-EGL context (NOT EvasGL, which is bound to
// the EFL main thread). Finished TBM buffers are presented on the main thread
// through an Evas image's native surface (EVAS_NATIVE_SURFACE_TBM), so this
// path needs no Evas_GL at all. See onMakeCurrent/onSwapBuffers/onTick
// below.
//
// These headers MUST precede <Elementary.h>: EGL/egl.h drags in
// KHR/khrplatform.h which defines KHRONOS_SUPPORT_INT64 + `typedef int64_t
// khronos_int64_t`. If Elementary.h (which transitively includes Evas_GL.h) is
// seen first, Evas_GL.h emits its own `typedef signed long long
// khronos_int64_t` fallback, which on a 64-bit target conflicts with
// khrplatform's `long` and poisons all GLES2 decls.
//
// Elementary.h always pulls in Evas_GL.h. Including the native GLES2 headers
// first makes Evas_GL.h skip its duplicate GL type/enum block (guarded by
// __gl2_h_), but it still hard-#errors unless we tell it we deliberately use
// the native headers. We do (uv presents via EVAS_NATIVE_SURFACE_TBM, never
// EvasGL).
#define EVAS_GL_NO_GL_H_CHECK
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <tbm_surface.h>
#include <tbm_surface_internal.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#ifndef EGL_NATIVE_SURFACE_TIZEN
#define EGL_NATIVE_SURFACE_TIZEN 0x32A1
#endif
#ifndef EGL_IMAGE_PRESERVED_KHR
#define EGL_IMAGE_PRESERVED_KHR 0x30D2
#endif
#endif

#include <Elementary.h>
#include <Ecore_Input.h>
#include <Ecore_Input_Evas.h>
#include <Ecore_IMF.h>
#include <Ecore_IMF_Evas.h>

#if !defined(STARFISH_UV_CAIRO_GL)
#include <Evas_GL.h>
#endif

#ifdef STREAMLINE_PROFILE
#include "streamline_annotate.h"
#else
#define ANNOTATE_SETUP
#define ANNOTATE_CHANNEL_COLOR(channel, color, str)
#define ANNOTATE_CHANNEL_END(channel)
#define ANNOTATE_GREEN 0x00ff001b
#endif

namespace LWEDelegate {

using namespace LWE;

#if defined(STARFISH_UV_CAIRO_GL)
// Owns a private raw-EGL context (created lazily on the LWE/engine thread) plus
// a ring of TBM-backed FBOs. The engine composites into the current buffer's
// FBO; on swap we glFinish and hand the buffer to the EFL main thread, which
// shows it via the Evas image's TBM native surface. Render (LWE thread) and
// present (main thread) are decoupled; buffer ownership is tracked under a
// lock.
class UvTbmPresenter {
public:
    // Elastic buffer pool. We aim for BASE_BUF buffers, but if the engine wants
    // to render and no buffer is free (the main/EFL thread is stalled —
    // possibly waiting on us — so it cannot present/recycle), we allocate an
    // extra buffer on the spot instead of blocking. This makes the producer
    // NEVER wait on the consumer, so the two threads can never deadlock.
    // Surplus buffers are reclaimed once the pool is idle again. Capacity is
    // bounded by MAX_BUF.
    static const int BASE_BUF = 2;
    static const int MAX_BUF = 6;
    // A surplus buffer must sit idle (FREE) this many consecutive swaps before
    // it is reclaimed. Prevents churn: during smooth rendering the working set
    // (~3 buffers) is reused within a frame or two, so its streak never reaches
    // the threshold and we don't destroy/recreate a TBM every frame. Only a
    // genuinely unused buffer (light/idle scene, or a stall-grown spare) is
    // cut.
    static const int SHRINK_HYSTERESIS = 30;
    // Freeze the present animator after this many consecutive ticks with no new
    // frame, so the main loop can sleep instead of spinning at 60Hz forever.
    // The producer thaws it (once) when it next has a frame to show.
    static const int IDLE_TICKS_TO_FREEZE = 6;

    struct Buf {
        tbm_surface_h tbm;
        EGLImageKHR img;
        GLuint tex;
        GLuint fbo;
        bool alloc; // GL/TBM storage committed for this slot
    };
    // Buffer state. Only the producer (LWE thread) sets ENGINE; only the
    // consumer (main thread) sets DISPLAYING. FREE/READY are handed across.
    enum Owner {
        FREE,      // allocated-and-idle, or an empty slot (alloc == false)
        ENGINE,    // engine is rendering into it (m_renderIdx)
        READY,     // finished, waiting for the animator to show it
        DISPLAYING // currently set as the Evas native surface
    };

    explicit UvTbmPresenter(Evas_Object* image)
        : m_image(image)
        , m_animator(nullptr)
        , m_dpy(EGL_NO_DISPLAY)
        , m_cfg(nullptr)
        , m_ctx(EGL_NO_CONTEXT)
        , m_pbuf(EGL_NO_SURFACE)
        , m_w(0)
        , m_h(0)
        , m_renderIdx(0)
        , m_displayingIdx(-1)
        , m_latestReady(-1)
        , m_idleTicks(0)
        , m_animatorActive(true)
        , m_pCreateImage(nullptr)
        , m_pDestroyImage(nullptr)
        , m_pImgTargetTex(nullptr)
    {
        for (int i = 0; i < MAX_BUF; i++) {
            m_buf[i] = { nullptr, EGL_NO_IMAGE_KHR, 0, 0, false };
            m_owner[i] = FREE;
            m_freeStreak[i] = 0;
        }
        pthread_mutex_init(&m_lock, nullptr);

        // Present is PULLED by a vsync-paced animator on the main thread, not
        // pushed per-frame via thread_safe_call. This keeps the main loop
        // ticking at the refresh rate (so a near-idle light page still presents
        // at ~60), and decouples the display rate from the engine's render rate
        // without ever blocking the producer. ecore animators run at the
        // compositor frame rate.
        m_animator = ecore_animator_add(&UvTbmPresenter::animatorCb, this);
    }

    ~UvTbmPresenter()
    {
        if (m_animator) {
            ecore_animator_del(m_animator);
        }
        pthread_mutex_destroy(&m_lock);
    }

    // ---- LWE (engine/render) thread ----

    // Make our private context current and bind the current render buffer's FBO
    // so the engine's compositor renders into the TBM buffer.
    void onMakeCurrent(uint32_t w, uint32_t h)
    {
        ensureContext();
        if (w != m_w || h != m_h) {
            reallocAll(w, h);
        }
        eglMakeCurrent(m_dpy, m_pbuf, m_pbuf, m_ctx);
        bindRenderFBO();
    }

    EGLContext context() const
    {
        return m_ctx;
    }
    EGLDisplay display() const
    {
        return m_dpy;
    }
    EGLConfig config() const
    {
        return m_cfg;
    }

    // Main-thread teardown for the leaked presenter: stop the animator so it
    // does not fire against a torn-down canvas.
    void shutdownMainThreadResources()
    {
        if (m_animator) {
            ecore_animator_del(m_animator);
            m_animator = nullptr;
        }
    }

    // End of frame: ensure GPU finished, mark the just-rendered buffer READY
    // (the newest frame for the animator to show), acquire the next render
    // buffer (growing the pool if none is free — never blocking), reclaim any
    // surplus, and bind for the next frame. No present is pushed here; the
    // main-thread animator pulls the latest READY buffer at vsync.
    void onSwapBuffers()
    {
        if (m_ctx == EGL_NO_CONTEXT) {
            return;
        }
        pthread_mutex_lock(&m_lock);
        int published = m_renderIdx;
        bool ok = (published >= 0 && published < MAX_BUF &&
                   m_owner[published] == ENGINE && m_buf[published].alloc);
        if (!ok) {
            pthread_mutex_unlock(&m_lock);
            return;
        }
        pthread_mutex_unlock(&m_lock);

        glFinish();

        pthread_mutex_lock(&m_lock);
        m_owner[published] = READY;
        m_latestReady = published; // newest finished frame (latest-wins)
        bool needAlloc = false;
        int next = pickNextSlotLocked(&needAlloc);
        if (next < 0) {
            // Truly nothing reusable (should not happen: pickNext can reuse a
            // superseded ready). Reuse the slot we rendered as a last resort.
            next = published;
            needAlloc = false;
        }
        m_owner[next] = ENGINE;
        m_renderIdx = next;
        // If the present animator went to sleep while idle, wake it once for
        // this fresh frame. Only on the idle->active edge, never per frame.
        bool needThaw = !m_animatorActive;
        if (needThaw) {
            m_animatorActive = true;
        }
        pthread_mutex_unlock(&m_lock);

        if (needThaw) {
            ecore_main_loop_thread_safe_call_async(&UvTbmPresenter::thawOnMain,
                                                   this);
        }
        if (needAlloc) {
            allocBuffer(next); // slot reserved as ENGINE; no lock needed for GL
        }
        shrinkSurplus();
        bindRenderFBO();
    }

    // WebGL / shared-context plumbing (all on the LWE thread).
    uintptr_t createSharedContext()
    {
        ensureContext();
        EGLint ctxAttr[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
        EGLContext c = eglCreateContext(m_dpy, m_cfg, m_ctx, ctxAttr);
        return reinterpret_cast<uintptr_t>(c);
    }
    bool destroyContext(uintptr_t c)
    {
        eglDestroyContext(m_dpy, reinterpret_cast<EGLContext>(c));
        return true;
    }
    bool clearCurrent()
    {
        return eglMakeCurrent(m_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE,
                              EGL_NO_CONTEXT);
    }
    bool makeCurrentWithContext(uintptr_t c)
    {
        return eglMakeCurrent(m_dpy, m_pbuf, m_pbuf,
                              reinterpret_cast<EGLContext>(c));
    }

private:
    // Choose the next slot to render into. m_lock held by caller.
    //   1) an idle allocated buffer (cheapest);
    //   2) a superseded pending frame — one the producer made while a present
    //      was already queued; it will never be shown (latest-wins), so its
    //      buffer is free to reuse. This bounds the pool when the producer runs
    //      far ahead of the display, instead of growing every frame;
    //   3) grow into an empty slot (caller allocates it).
    // Returns -1 only if literally every slot is engine/displaying/latest.
    int pickNextSlotLocked(bool* needAlloc)
    {
        for (int i = 0; i < MAX_BUF; i++) {
            if (m_owner[i] == FREE && m_buf[i].alloc) {
                *needAlloc = false;
                return i;
            }
        }
        for (int i = 0; i < MAX_BUF; i++) {
            if (m_owner[i] == READY && i != m_latestReady) {
                *needAlloc = false;
                return i;
            }
        }
        for (int i = 0; i < MAX_BUF; i++) {
            if (m_owner[i] == FREE && !m_buf[i].alloc) {
                *needAlloc = true;
                return i;
            }
        }
        return -1;
    }

    void loadExt()
    {
        m_pCreateImage =
            (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
        m_pDestroyImage =
            (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
        m_pImgTargetTex =
            (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress(
                "glEGLImageTargetTexture2DOES");
    }

    void ensureContext()
    {
        if (m_ctx != EGL_NO_CONTEXT) {
            return;
        }
        m_dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        eglInitialize(m_dpy, nullptr, nullptr);
        eglBindAPI(EGL_OPENGL_ES_API);
        EGLint cfgAttr[] = { EGL_SURFACE_TYPE,
                             EGL_PBUFFER_BIT,
                             EGL_RENDERABLE_TYPE,
                             EGL_OPENGL_ES2_BIT,
                             EGL_RED_SIZE,
                             8,
                             EGL_GREEN_SIZE,
                             8,
                             EGL_BLUE_SIZE,
                             8,
                             EGL_ALPHA_SIZE,
                             8,
                             EGL_NONE };
        EGLint n = 0;
        eglChooseConfig(m_dpy, cfgAttr, &m_cfg, 1, &n);
        EGLint ctxAttr[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
        m_ctx = eglCreateContext(m_dpy, m_cfg, EGL_NO_CONTEXT, ctxAttr);
        EGLint pb[] = { EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE };
        m_pbuf = eglCreatePbufferSurface(m_dpy, m_cfg, pb);
        eglMakeCurrent(m_dpy, m_pbuf, m_pbuf, m_ctx);
        loadExt();
    }

    void bindRenderFBO()
    {
        if (m_renderIdx >= 0 && m_renderIdx < MAX_BUF &&
            m_buf[m_renderIdx].fbo) {
            glBindFramebuffer(GL_FRAMEBUFFER, m_buf[m_renderIdx].fbo);
            glViewport(0, 0, m_w, m_h);
        }
    }

    // (Re)create GL/TBM storage for a single slot. Context must be current.
    void allocBuffer(int i)
    {
        m_buf[i].tbm = tbm_surface_create(m_w, m_h, TBM_FORMAT_ARGB8888);
        EGLint imgAttr[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE };
        m_buf[i].img =
            m_pCreateImage(m_dpy, EGL_NO_CONTEXT, EGL_NATIVE_SURFACE_TIZEN,
                           (EGLClientBuffer)(intptr_t)m_buf[i].tbm, imgAttr);
        glGenTextures(1, &m_buf[i].tex);
        glBindTexture(GL_TEXTURE_2D, m_buf[i].tex);
        m_pImgTargetTex(GL_TEXTURE_2D, (GLeglImageOES)m_buf[i].img);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glGenFramebuffers(1, &m_buf[i].fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_buf[i].fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, m_buf[i].tex, 0);
        GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (st != GL_FRAMEBUFFER_COMPLETE) {
            STARFISH_LOG_ERROR("UvTbmPresenter: tbm FBO incomplete 0x%x", st);
        }
        m_buf[i].alloc = true;
    }

    // Release a single slot's GL/TBM storage. Context must be current.
    void destroyBuffer(int i)
    {
        if (m_buf[i].fbo) {
            glDeleteFramebuffers(1, &m_buf[i].fbo);
        }
        if (m_buf[i].tex) {
            glDeleteTextures(1, &m_buf[i].tex);
        }
        if (m_buf[i].img != EGL_NO_IMAGE_KHR && m_pDestroyImage) {
            m_pDestroyImage(m_dpy, m_buf[i].img);
        }
        if (m_buf[i].tbm) {
            tbm_surface_destroy(m_buf[i].tbm);
        }
        m_buf[i] = { nullptr, EGL_NO_IMAGE_KHR, 0, 0, false };
    }

    // Reclaim buffers beyond BASE_BUF that have stayed idle (FREE) for at least
    // SHRINK_HYSTERESIS consecutive swaps. The streak gate avoids churning a
    // buffer that is still part of the active working set. Context must be
    // current; runs on the LWE thread.
    void shrinkSurplus()
    {
        pthread_mutex_lock(&m_lock);
        int allocated = 0;
        for (int i = 0; i < MAX_BUF; i++) {
            if (m_owner[i] == FREE && m_buf[i].alloc) {
                m_freeStreak[i]++;
            } else {
                m_freeStreak[i] = 0;
            }
            if (m_buf[i].alloc) {
                allocated++;
            }
        }
        for (int i = MAX_BUF - 1; i >= 0 && allocated > BASE_BUF; i--) {
            if (m_owner[i] == FREE && m_buf[i].alloc &&
                m_freeStreak[i] >= SHRINK_HYSTERESIS) {
                destroyBuffer(i); // stays FREE, now alloc == false
                allocated--;
            }
        }
        pthread_mutex_unlock(&m_lock);
    }

    void reallocAll(uint32_t w, uint32_t h)
    {
        pthread_mutex_lock(&m_lock);
        for (int i = 0; i < MAX_BUF; i++) {
            destroyBuffer(i);
            m_owner[i] = FREE;
            m_freeStreak[i] = 0;
        }
        m_w = w;
        m_h = h;
        for (int i = 0; i < BASE_BUF; i++) {
            allocBuffer(i);
        }
        m_renderIdx = 0;
        m_owner[0] = ENGINE;
        m_displayingIdx = -1;
        m_latestReady = -1; // drop any stale present target on resize
        pthread_mutex_unlock(&m_lock);
    }

    // ---- main (EFL) thread, vsync-paced ----
    static Eina_Bool animatorCb(void* data)
    {
        static_cast<UvTbmPresenter*>(data)->onTick();
        return ECORE_CALLBACK_RENEW;
    }

    // Wake the frozen animator on the idle->active edge (called from the LWE
    // thread via thread_safe_call). Runs on the main thread.
    static void thawOnMain(void* data)
    {
        UvTbmPresenter* self = static_cast<UvTbmPresenter*>(data);
        if (self->m_animator) {
            ecore_animator_thaw(self->m_animator);
        }
    }

    // One tick == one display refresh. Show the latest finished (READY) frame,
    // recycle every superseded READY frame plus the outgoing on-screen buffer.
    // If nothing new is ready for a while, freeze so the main loop can sleep.
    void onTick()
    {
        bool shouldFreeze = false;
        pthread_mutex_lock(&m_lock);
        int idx = m_latestReady;
        bool ok = (idx >= 0 && idx < MAX_BUF && m_buf[idx].alloc &&
                   m_owner[idx] == READY);
        int old = m_displayingIdx;
        if (ok) {
            m_idleTicks = 0;
            // Recycle frames the producer raced ahead to make; only the newest
            // (idx) is shown (latest-wins).
            for (int i = 0; i < MAX_BUF; i++) {
                if (i != idx && m_owner[i] == READY) {
                    m_owner[i] = FREE;
                }
            }
            m_owner[idx] = DISPLAYING;
            m_displayingIdx = idx;
            m_latestReady = -1;
            // The buffer leaving the screen returns to the pool. (Accepted
            // side-effect: the engine may reuse it before the compositor has
            // fully finished with the outgoing frame.)
            if (old >= 0 && old != idx && m_owner[old] == DISPLAYING) {
                m_owner[old] = FREE;
            }
        } else {
            m_idleTicks++;
            if (m_idleTicks >= IDLE_TICKS_TO_FREEZE && m_animatorActive) {
                m_animatorActive = false;
                shouldFreeze = true;
            }
        }
        pthread_mutex_unlock(&m_lock);

        if (shouldFreeze && m_animator) {
            // Main serializes freeze (here) and thaw (thawOnMain); if the
            // producer raced a thaw in, it runs after this and re-activates.
            ecore_animator_freeze(m_animator);
        }
        if (!ok) {
            return;
        }

        Evas_Native_Surface ns;
        memset(&ns, 0, sizeof(ns));
        ns.version = EVAS_NATIVE_SURFACE_VERSION;
        ns.type = EVAS_NATIVE_SURFACE_TBM;
        ns.data.tbm.buffer = m_buf[idx].tbm;
        ns.data.tbm.rot = 0;
        ns.data.tbm.ratio = 0;
        ns.data.tbm.flip =
            EVAS_IMAGE_FLIP_HORIZONTAL | EVAS_IMAGE_FLIP_VERTICAL;
        evas_object_image_native_surface_set(m_image, &ns);
        evas_object_image_pixels_dirty_set(m_image, EINA_TRUE);
    }

    Evas_Object* m_image;
    Ecore_Animator* m_animator; // vsync-paced present pull (main thread)
    EGLDisplay m_dpy;
    EGLConfig m_cfg;
    EGLContext m_ctx;
    EGLSurface m_pbuf;
    uint32_t m_w;
    uint32_t m_h;
    Buf m_buf[MAX_BUF];
    Owner m_owner[MAX_BUF];
    int m_freeStreak[MAX_BUF]; // consecutive swaps a slot has stayed FREE
    int m_renderIdx;           // buffer the engine renders into (LWE thread)
    int m_displayingIdx; // currently set as native surface (held by compositor)
    int m_latestReady;   // newest finished frame awaiting present (-1 if none)
    int m_idleTicks;     // consecutive animator ticks with no new frame
    bool m_animatorActive; // false while the animator is frozen (idle)
    pthread_mutex_t m_lock;
    PFNEGLCREATEIMAGEKHRPROC m_pCreateImage;
    PFNEGLDESTROYIMAGEKHRPROC m_pDestroyImage;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC m_pImgTargetTex;
};
#endif

const int g_arrowKeyDownMinimumDelayInMS = 150;
static int g_arrowKeyDownTimestamp[4];

static const char* getImfMethod()
{
    Eina_List* modules;

    modules = ecore_imf_context_available_ids_get();
    if (!modules)
        return NULL;

    void* module;
    EINA_LIST_FREE(modules, module)
    {
        return (const char*)module;
    }

    return NULL;
}

static bool isASCIIPrintableKey(char c)
{
    if (c >= 32 && c <= 126) {
        return true;
    }
    return false;
}

static KeyValue ecoreEventKeyToKeyValue(const char* ecoreKeyString,
                                        bool isShiftPressed)
{
    if (strcmp("Left", ecoreKeyString) == 0) {
        return KeyValue::ArrowLeftKey;
    } else if (strcmp("Right", ecoreKeyString) == 0) {
        return KeyValue::ArrowRightKey;
    } else if (strcmp("Up", ecoreKeyString) == 0) {
        return KeyValue::ArrowUpKey;
    } else if (strcmp("Down", ecoreKeyString) == 0) {
        return KeyValue::ArrowDownKey;
    } else if (strcmp("space", ecoreKeyString) == 0) {
        return KeyValue::SpaceKey;
    } else if (strcmp("Return", ecoreKeyString) == 0) {
        return KeyValue::EnterKey;
    } else if (strcmp("Tab", ecoreKeyString) == 0) {
        return KeyValue::TabKey;
    } else if (strcmp("BackSpace", ecoreKeyString) == 0) {
        return KeyValue::BackspaceKey;
    } else if (strcmp("Escape", ecoreKeyString) == 0) {
        return KeyValue::EscapeKey;
    } else if (strcmp("Delete", ecoreKeyString) == 0) {
        return KeyValue::DeleteKey;
    } else if (strcmp("at", ecoreKeyString) == 0) {
        return KeyValue::AtMarkKey;
    } else if (strcmp("minus", ecoreKeyString) == 0) {
        if (isShiftPressed) {
            return KeyValue::UnderScoreMarkKey;
        } else {
            return KeyValue::MinusMarkKey;
        }
    } else if (strcmp("equal", ecoreKeyString) == 0) {
        if (isShiftPressed) {
            return KeyValue::PlusMarkKey;
        } else {
            return KeyValue::EqualitySignKey;
        }
    } else if (strcmp("bracketleft", ecoreKeyString) == 0) {
        if (isShiftPressed) {
            return KeyValue::LeftCurlyBracketMarkKey;
        } else {
            return KeyValue::LeftSquareBracketKey;
        }
    } else if (strcmp("bracketright", ecoreKeyString) == 0) {
        if (isShiftPressed) {
            return KeyValue::RightCurlyBracketMarkKey;
        } else {
            return KeyValue::RightSquareBracketKey;
        }
    } else if (strcmp("semicolon", ecoreKeyString) == 0) {
        if (isShiftPressed) {
            return KeyValue::ColonMarkKey;
        } else {
            return KeyValue::SemiColonMarkKey;
        }
    } else if (strcmp("apostrophe", ecoreKeyString) == 0) {
        if (isShiftPressed) {
            return KeyValue::DoubleQuoteMarkKey;
        } else {
            return KeyValue::SingleQuoteMarkKey;
        }
    } else if (strcmp("comma", ecoreKeyString) == 0) {
        if (isShiftPressed) {
            return KeyValue::LessThanMarkKey;
        } else {
            return KeyValue::CommaMarkKey;
        }
    } else if (strcmp("period", ecoreKeyString) == 0) {
        if (isShiftPressed) {
            return KeyValue::GreaterThanSignKey;
        } else {
            return KeyValue::PeriodKey;
        }
    } else if (strcmp("slash", ecoreKeyString) == 0) {
        if (isShiftPressed) {
            return KeyValue::QuestionMarkKey;
        } else {
            return KeyValue::SlashKey;
        }
    } else if (strlen(ecoreKeyString) == 1) {
        char ch = ecoreKeyString[0];
        if (ch >= '0' && ch <= '9') {
            if (isShiftPressed) {
                switch (ch) {
                case '1':
                    return KeyValue::ExclamationMarkKey;
                case '2':
                    return KeyValue::AtMarkKey;
                case '3':
                    return KeyValue::SharpMarkKey;
                case '4':
                    return KeyValue::DollarMarkKey;
                case '5':
                    return KeyValue::PercentMarkKey;
                case '6':
                    return KeyValue::CaretMarkKey;
                case '7':
                    return KeyValue::AmpersandMarkKey;
                case '8':
                    return KeyValue::AsteriskMarkKey;
                case '9':
                    return KeyValue::LeftParenthesisMarkKey;
                case '0':
                    return KeyValue::RightParenthesisMarkKey;
                }
            }
            return (KeyValue)(KeyValue::Digit0Key + ch - '0');
        } else if (ch >= 'a' && ch <= 'z') {
            return (KeyValue)(KeyValue::LowerAKey + ch - 'a');
        } else if (ch >= 'A' && ch <= 'Z') {
            return (KeyValue)(KeyValue::AKey + ch - 'A');
        }
    } else if (strcmp("XF86AudioRaiseVolume", ecoreKeyString) == 0) {
        return KeyValue::TVVolumeUpKey;
    } else if (strcmp("XF86AudioLowerVolume", ecoreKeyString) == 0) {
        return KeyValue::TVVolumeDownKey;
    } else if (strcmp("XF86AudioMute", ecoreKeyString) == 0) {
        return KeyValue::TVMuteKey;
    } else if (strcmp("XF86RaiseChannel", ecoreKeyString) == 0) {
        return KeyValue::TVChannelUpKey;
    } else if (strcmp("XF86LowerChannel", ecoreKeyString) == 0) {
        return KeyValue::TVChannelDownKey;
    } else if (strcmp("XF86AudioRewind", ecoreKeyString) == 0) {
        return KeyValue::MediaTrackPreviousKey;
    } else if (strcmp("XF86AudioNext", ecoreKeyString) == 0) {
        return KeyValue::MediaTrackNextKey;
    } else if (strcmp("XF86AudioPause", ecoreKeyString) == 0) {
        return KeyValue::MediaPauseKey;
    } else if (strcmp("XF86AudioRecord", ecoreKeyString) == 0) {
        return KeyValue::MediaRecordKey;
    } else if (strcmp("XF86AudioPlay", ecoreKeyString) == 0) {
        return KeyValue::MediaPlayKey;
    } else if (strcmp("XF86AudioStop", ecoreKeyString) == 0) {
        return KeyValue::MediaStopKey;
    } else if (strcmp("XF86Info", ecoreKeyString) == 0) {
        return KeyValue::TVInfoKey;
    } else if (strcmp("XF86Back", ecoreKeyString) == 0) {
        return KeyValue::TVReturnKey;
    } else if (strcmp("XF86Red", ecoreKeyString) == 0) {
        return KeyValue::TVRedKey;
    } else if (strcmp("XF86Green", ecoreKeyString) == 0) {
        return KeyValue::TVGreenKey;
    } else if (strcmp("XF86Yellow", ecoreKeyString) == 0) {
        return KeyValue::TVYellowKey;
    } else if (strcmp("XF86Blue", ecoreKeyString) == 0) {
        return KeyValue::TVBlueKey;
    } else if (strcmp("XF86SysMenu", ecoreKeyString) == 0) {
        return KeyValue::TVMenuKey;
    } else if (strcmp("XF86Home", ecoreKeyString) == 0) {
        return KeyValue::TVHomeKey;
    } else if (strcmp("XF86Exit", ecoreKeyString) == 0) {
        return KeyValue::TVExitKey;
    } else if (strcmp("XF86PreviousChannel", ecoreKeyString) == 0) {
        return KeyValue::TVPreviousChannel;
    } else if (strcmp("XF86ChannelList", ecoreKeyString) == 0) {
        return KeyValue::TVChannelList;
    } else if (strcmp("XF86ChannelGuide", ecoreKeyString) == 0) {
        return KeyValue::TVChannelGuide;
    } else if (strcmp("XF86SimpleMenu", ecoreKeyString) == 0) {
        return KeyValue::TVSimpleMenu;
    } else if (strcmp("XF86EManual", ecoreKeyString) == 0) {
        return KeyValue::TVEManual;
    } else if (strcmp("XF86ExtraApp", ecoreKeyString) == 0) {
        return KeyValue::TVExtraApp;
    } else if (strcmp("XF86Search", ecoreKeyString) == 0) {
        return KeyValue::TVSearch;
    } else if (strcmp("XF86PictureSize", ecoreKeyString) == 0) {
        return KeyValue::TVPictureSize;
    } else if (strcmp("XF86Sleep", ecoreKeyString) == 0) {
        return KeyValue::TVSleep;
    } else if (strcmp("XF86Caption", ecoreKeyString) == 0) {
        return KeyValue::TVCaption;
    } else if (strcmp("XF86More", ecoreKeyString) == 0) {
        return KeyValue::TVMore;
    } else if (strcmp("XF86BTVoice", ecoreKeyString) == 0) {
        return KeyValue::TVBTVoice;
    } else if (strcmp("XF86Color", ecoreKeyString) == 0) {
        return KeyValue::TVColor;
    } else if (strcmp("XF86PlayBack", ecoreKeyString) == 0) {
        return KeyValue::TVPlayBack;
    }

    STARFISH_LOG_ERROR("WebViewEFL - unimplemented key %s", ecoreKeyString);
    return KeyValue::UnidentifiedKey;
}

const uint32_t CLICK_REFRESH_DELAY = 400;

static void elm_box_layout_cb(Evas_Object* o, Evas_Object_Box_Data* priv,
                              void* user_data)
{
    int x, y, width, height;
    evas_object_geometry_get(o, &x, &y, &width, &height);

    Evas_Object_Box_Option* opt;
    Eina_List* l;
    for (l = priv->children,
        opt = (Evas_Object_Box_Option*)eina_list_data_get(l);
         l; l = eina_list_next(l),
        opt = (Evas_Object_Box_Option*)eina_list_data_get(l)) {
        evas_object_geometry_set(opt->obj, x, y, width, height);
    }
}

class WebViewEFL : public WebViewImpl {
public:
    WebViewEFL(void* winArg, unsigned x, unsigned y, unsigned width,
               unsigned height, float devicePixelRatio,
               const char* defaultFontName, const char* locale,
               const char* timezoneID)
        : m_resizeHandler(nullptr)
        , m_shownHandler(nullptr)
        , m_mouseDownEventHandler(nullptr)
        , m_mouseMoveEventHandler(nullptr)
        , m_mouseUpEventHandler(nullptr)
        , m_mouseWheelEventHandler(nullptr)
        , m_keyDownEventHandler(nullptr)
        , m_keyUpEventHandler(nullptr)
        , m_buttonForClickClickEventHandler(nullptr)
        , m_buttonForClickMouseDownEventHandler(nullptr)
        , m_buttonForClickMouseMoveEventHandler(nullptr)
        , m_buttonForClickMouseUpEventHandler(nullptr)
        , m_buttonForClick(nullptr)
        , m_buttonForClickCipper(nullptr)
#if defined(STARFISH_UV_CAIRO_GL)
        , m_uvPresenter(nullptr)
#else
        , m_glSync(nullptr)
#endif
        , m_lastMouseX(0)
        , m_lastMouseY(0)
        , m_isMouseLbuttonDown(false)
        , m_isKeyDown(false)
        , m_isDestroyed(false)
        , m_lastRenderingTime(0)
        , m_lastInputTime(0)
        , m_evasGlRotationDegrees(-1)
    {
        STARFISH_LOG_INFO("WebViewEFL::WebViewEFL");
        Evas_Object* win = (Evas_Object*)winArg;

        m_windowObject = win;

#if !defined(STARFISH_UV_CAIRO_GL)
        // glib backend runs the engine loop on the EFL main thread; integrate
        // it into ecore. uv backend runs the engine on its own LWE thread
        // instead.
        ecore_main_loop_glib_integrate();
#endif

        m_windowDelEventHandler = [](void* data, Evas* e, Evas_Object* obj,
                                     void* event_info) {
            WebViewEFL* wv = (WebViewEFL*)data;
            wv->m_isDestroyed = true;
        };

        evas_object_event_callback_add(m_windowObject, EVAS_CALLBACK_DEL,
                                       m_windowDelEventHandler, this);

        m_nonIMEKeyEventBox = elm_label_add(win);
        evas_object_show(m_nonIMEKeyEventBox);

        m_mainBox = elm_box_add(win);
        evas_object_resize(m_mainBox, width, height);
        evas_object_move(m_mainBox, x, y);
        elm_box_layout_set(m_mainBox, elm_box_layout_cb, NULL, NULL);
        evas_object_show(m_mainBox);

        m_graphicsAdapter =
            evas_object_image_filled_add(evas_object_evas_get(win));
        evas_object_resize(m_graphicsAdapter, width, height);
        evas_object_move(m_graphicsAdapter, x, y);
        evas_object_image_size_set(m_graphicsAdapter, width, height);
        evas_object_image_alpha_set(m_graphicsAdapter, EINA_TRUE);

        elm_box_pack_end(m_mainBox, m_nonIMEKeyEventBox);
        elm_box_pack_end(m_mainBox, m_graphicsAdapter);

        m_isRenderedOnce = false;
        m_immediatelyClearScreenAnimator = nullptr;
#if defined(STARFISH_UV_CAIRO_GL)
        // No EvasGL here. The engine (LWE thread) renders into TBM buffers via
        // a private EGL context owned by the presenter; the native surface of
        // m_graphicsAdapter is set at present time on the main thread.
        m_uvPresenter = new UvTbmPresenter(m_graphicsAdapter);
        evas_object_show(m_graphicsAdapter);
#else
        m_glEvasgl = evas_gl_new(evas_object_evas_get(win));
        // Set a surface config
        m_glCfg = evas_gl_config_new();
        m_glCfg->color_format = EVAS_GL_RGBA_8888;

// we need to set these secret flags reducing memory usage
// see platform/upstream/efl/src/modules/evas/engines/gl_common/evas_gl_core.c
// in tizen
// or ./src/modules/evas/engines/gl_common/evas_gl_core.c in efl git
#define EVAS_GL_OPTIONS_DIRECT_MEMORY_OPTIMIZE (1 << 12)
#define EVAS_GL_OPTIONS_DIRECT_OVERRIDE (1 << 13)
        m_glCfg->options_bits = (Evas_GL_Options_Bits)(
            EVAS_GL_OPTIONS_DIRECT | EVAS_GL_OPTIONS_DIRECT_OVERRIDE |
            EVAS_GL_OPTIONS_DIRECT_MEMORY_OPTIMIZE |
            EVAS_GL_OPTIONS_CLIENT_SIDE_ROTATION);
        STARFISH_LOG_INFO("try to use EvasGL direct mode");

        // Create a surface and context
        m_glSfc = evas_gl_surface_create(m_glEvasgl, m_glCfg, width, height);
        m_glCtx = evas_gl_context_version_create(
            m_glEvasgl, NULL, Evas_GL_Context_Version::EVAS_GL_GLES_3_X);

        if (m_glCtx == nullptr) {
            STARFISH_LOG_ERROR(
                "failed to create openGL 3.0 context... try to use 2.0 "
                "instead");
            m_glCtx = evas_gl_context_version_create(
                m_glEvasgl, NULL, Evas_GL_Context_Version::EVAS_GL_GLES_2_X);
        }
        if (m_glCtx == nullptr) {
            STARFISH_LOG_ERROR("failed to create openGL 2.0 context...");
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }

        m_glGlapi = evas_gl_context_api_get(m_glEvasgl, m_glCtx);

        Evas_Native_Surface ns;
        evas_gl_native_surface_get(m_glEvasgl, m_glSfc, &ns);
        evas_object_image_native_surface_set(m_graphicsAdapter, &ns);

        // This is how to set up evasgl's viewport correctly.
        // This guide was received from efl team.
        evas_gl_make_current(m_glEvasgl, m_glSfc, m_glCtx);
        evas_object_show(m_graphicsAdapter);
        evas_gl_make_current(m_glEvasgl, nullptr, nullptr);
#endif

        m_windowShownHandler = [](void* data, Evas* e, Evas_Object* obj,
                                  void* event_info) {
            WebViewEFL* wv = (WebViewEFL*)data;
            STARFISH_LOG_INFO("WebViewEFL::windowShownHandler");
            wv->immediatelyClearScreen();
            wv->Resume();
        };
        m_windowHiddenHandler = [](void* data, Evas* e, Evas_Object* obj,
                                   void* event_info) {
            WebViewEFL* wv = (WebViewEFL*)data;
            STARFISH_LOG_INFO("WebViewEFL::windowHiddenHandler");
            wv->Pause();
        };
        evas_object_event_callback_add(m_windowObject, EVAS_CALLBACK_SHOW,
                                       m_windowShownHandler, this);
        evas_object_event_callback_add(m_windowObject, EVAS_CALLBACK_HIDE,
                                       m_windowHiddenHandler, this);

        m_windowRotaionChangedHandler = [](void* data, Evas_Object* object,
                                           void* event_info) {
            STARFISH_LOG_INFO("WebViewEFL::windowRotaionChangedHandler");
            WebViewEFL* wv = static_cast<WebViewEFL*>(data);
            wv->FetchWebContainer()->SetNeedsFullRepainting();
        };
        evas_object_smart_callback_add(m_windowObject, "rotation,changed",
                                       m_windowRotaionChangedHandler, this);

        m_isKeyDown = false;
        m_lastClickedTimestamp = 0;
        m_clickedCount = 0;
        m_imfContext = nullptr;
        m_lastKeyPressedTimestamp = 0;
        m_offsetYDueToSoftwareKeyboard = 0;

        // Initialize screen matrix.
        updateScreenMatrix(0, width, height);

        m_mouseDownEventHandler = [](void* data, Evas* evas, Evas_Object* obj,
                                     void* event_info) -> void {
            WebViewEFL* webView = (WebViewEFL*)data;
            Evas_Event_Mouse_Down* ev = (Evas_Event_Mouse_Down*)event_info;
            // We care just left button now
            int currentPosX = ev->output.x;
            int currentPosY = ev->output.y;

            int x, y;
            evas_object_geometry_get(webView->m_graphicsAdapter, &x, &y, 0, 0);
            currentPosX -= x;
            currentPosY -= y;

            if (ev->button == 1 && (currentPosX >= 0 && currentPosY >= 0)) {
                if (ev->timestamp - webView->m_lastClickedTimestamp >
                    CLICK_REFRESH_DELAY) {
                    webView->m_clickedCount = 1;
                    webView->m_lastClickedTimestamp = ev->timestamp;
                } else {
                    webView->m_clickedCount++;
                }
                webView->FetchWebContainer()->DispatchMouseDownEvent(
                    MouseButtonValue::LeftButton,
                    MouseButtonsValue::LeftButtonDown, currentPosX,
                    currentPosY);
                webView->m_isMouseLbuttonDown = true;
            }

            webView->HideSoftwareKeyboardIfPossible();

            return;
        };
        evas_object_event_callback_add(m_graphicsAdapter,
                                       EVAS_CALLBACK_MOUSE_DOWN,
                                       m_mouseDownEventHandler, this);

        m_mouseUpEventHandler = [](void* data, Evas* evas, Evas_Object* obj,
                                   void* event_info) -> void {
            WebViewEFL* webView = (WebViewEFL*)data;
            Evas_Event_Mouse_Up* ev = (Evas_Event_Mouse_Up*)event_info;
            // We care just left button now
            int currentPosX = ev->output.x;
            int currentPosY = ev->output.y;
            int x, y;
            evas_object_geometry_get(webView->m_graphicsAdapter, &x, &y, 0, 0);
            currentPosX -= x;
            currentPosY -= y;

            if (ev->button == 1 && (currentPosX >= 0 && currentPosY >= 0)) {
                if (ev->timestamp - webView->m_lastClickedTimestamp >
                    CLICK_REFRESH_DELAY) {
                    webView->m_clickedCount = 1;
                    webView->m_lastClickedTimestamp = ev->timestamp;
                } else {
                    webView->m_clickedCount++;
                }
                webView->FetchWebContainer()->DispatchMouseUpEvent(
                    MouseButtonValue::NoButton, MouseButtonsValue::NoButtonDown,
                    currentPosX, currentPosY);
                webView->m_isMouseLbuttonDown = false;
            }
            return;
        };
        evas_object_event_callback_add(m_graphicsAdapter,
                                       EVAS_CALLBACK_MOUSE_UP,
                                       m_mouseUpEventHandler, this);

        m_mouseWheelEventHandler = [](void* data, Evas* evas, Evas_Object* obj,
                                      void* event_info) -> void {
            WebViewEFL* wv = (WebViewEFL*)data;
            Evas_Event_Mouse_Wheel* ev = (Evas_Event_Mouse_Wheel*)event_info;
            // We care just left button now
            int currentPosX = ev->output.x;
            int currentPosY = ev->output.y;
            int x, y;
            evas_object_geometry_get(wv->m_graphicsAdapter, &x, &y, 0, 0);
            currentPosX -= x;
            currentPosY -= y;
            wv->FetchWebContainer()->DispatchMouseWheelEvent(
                currentPosX, currentPosY, ev->z);
            return;
        };
        evas_object_event_callback_add(m_graphicsAdapter,
                                       EVAS_CALLBACK_MOUSE_WHEEL,
                                       m_mouseWheelEventHandler, this);

        m_mouseMoveEventHandler = [](void* data, Evas* evas, Evas_Object* obj,
                                     void* event_info) -> void {
            WebViewEFL* webView = (WebViewEFL*)data;
            Evas_Event_Mouse_Move* ev = (Evas_Event_Mouse_Move*)event_info;
            // We care just left button now
            int currentPosX = ev->cur.output.x;
            int currentPosY = ev->cur.output.y;
            int x, y;
            evas_object_geometry_get(webView->m_graphicsAdapter, &x, &y, 0, 0);
            currentPosX -= x;
            currentPosY -= y;
            unsigned char buttons = webView->m_isMouseLbuttonDown
                                        ? MouseButtonsValue::LeftButtonDown
                                        : MouseButtonsValue::NoButtonDown;
            webView->FetchWebContainer()->DispatchMouseMoveEvent(
                MouseButtonValue::NoButton, (MouseButtonsValue)buttons,
                currentPosX, currentPosY);
            return;
        };
        evas_object_event_callback_add(m_graphicsAdapter,
                                       EVAS_CALLBACK_MOUSE_MOVE,
                                       m_mouseMoveEventHandler, this);

        m_keyDownEventHandler = [](void* data, Evas* evas, Evas_Object* obj,
                                   void* event_info) -> void {
            WebViewEFL* webView = (WebViewEFL*)data;
            Evas_Event_Key_Down* ev = (Evas_Event_Key_Down*)event_info;
            STARFISH_LOG_INFO(
                "EVAS_CALLBACK_KEY_DOWN for m_nonIMEKeyEventBox [%s,%d]",
                ev->key, (int)ev->keycode);
            if (evas_object_focus_get(webView->m_mainBox) == EINA_TRUE) {
                STARFISH_LOG_INFO(
                    "EVAS_CALLBACK_KEY_DOWN for m_nonIMEKeyEventBox but "
                    "m_mainBox has focus[%s]",
                    ev->key);
                return;
            }

            if (webView->m_lastInputTime == 0) {
                ANNOTATE_SETUP;
                ANNOTATE_CHANNEL_COLOR(3000, ANNOTATE_GREEN,
                                       "EVAS_CALLBACK_KEY_DOWN");
                webView->m_lastInputTime = Starfish::longTickCount();
                ANNOTATE_CHANNEL_END(3000);
            }

#ifdef STARFISH_TIZEN_TV
            if ((strncmp(ev->key, "XF86Red", 7) == 0)) {
                ev->key = "Tab";
            } else if ((strncmp(ev->key, "XF86Back", 8) == 0)) {
                ev->key = "Escape";
            }
#endif
            auto keyValue = ecoreEventKeyToKeyValue(
                ev->key, (evas_key_modifier_is_set(ev->modifiers, "Shift_L") ==
                          EINA_TRUE) ||
                             (evas_key_modifier_is_set(
                                  ev->modifiers, "Shift_R") == EINA_TRUE));

            if (keyValue >= ArrowDownKey && keyValue <= ArrowRightKey) {
                unsigned int currentTimestamp = ev->timestamp;
                if (currentTimestamp -
                        g_arrowKeyDownTimestamp[keyValue - ArrowDownKey] <
                    g_arrowKeyDownMinimumDelayInMS) {
                    return;
                }
                g_arrowKeyDownTimestamp[keyValue - ArrowDownKey] =
                    currentTimestamp;
            }

            webView->FetchWebContainer()->DispatchKeyDownEvent(keyValue);
            webView->FetchWebContainer()->DispatchKeyPressEvent(keyValue);
            webView->m_isKeyDown = true;
        };
        evas_object_event_callback_add(m_nonIMEKeyEventBox,
                                       EVAS_CALLBACK_KEY_DOWN,
                                       m_keyDownEventHandler, this);

        m_keyUpEventHandler = [](void* data, Evas* evas, Evas_Object* obj,
                                 void* event_info) -> void {
            WebViewEFL* webView = (WebViewEFL*)data;
            Evas_Event_Key_Up* ev = (Evas_Event_Key_Up*)event_info;
            if (evas_object_focus_get(webView->m_mainBox) == EINA_TRUE) {
                return;
            }

#ifdef STARFISH_TIZEN_TV
            if ((strncmp(ev->key, "XF86Red", 7) == 0)) {
                ev->key = "Tab";
            } else if ((strncmp(ev->key, "XF86Back", 8) == 0)) {
                ev->key = "Escape";
            }
#endif
            auto keyValue = ecoreEventKeyToKeyValue(
                ev->key, (evas_key_modifier_is_set(ev->modifiers, "Shift_L") ==
                          EINA_TRUE) ||
                             (evas_key_modifier_is_set(
                                  ev->modifiers, "Shift_R") == EINA_TRUE));

            if (keyValue >= ArrowDownKey && keyValue <= ArrowRightKey) {
                g_arrowKeyDownTimestamp[keyValue - ArrowDownKey] = 0;
            }

            webView->FetchWebContainer()->DispatchKeyUpEvent(keyValue);
            webView->m_isKeyDown = false;
            return;
        };
        evas_object_event_callback_add(m_nonIMEKeyEventBox,
                                       EVAS_CALLBACK_KEY_UP,
                                       m_keyUpEventHandler, this);

        m_resizeHandler = [](void* data, Evas* e, Evas_Object* obj,
                             void* event_info) {
            WebViewEFL* wv = (WebViewEFL*)data;
            int w, h;
            evas_object_geometry_get(wv->m_mainBox, NULL, NULL, &w, &h);
            if (w == 0 || h == 0) {
                STARFISH_LOG_WARN("the main box has a zero size");
                w = 1;
                h = 1;
            }
            evas_object_resize(wv->m_graphicsAdapter, w, h);

#if defined(STARFISH_UV_CAIRO_GL)
            // The presenter reallocs its TBM buffers on the LWE thread when it
            // observes the new size at the next onMakeCurrent. Just drop the
            // current native surface and update the image size here.
            evas_object_image_native_surface_set(wv->m_graphicsAdapter, NULL);
            evas_object_image_size_set(wv->m_graphicsAdapter, w, h);
#else
            evas_object_image_native_surface_set(wv->m_graphicsAdapter, NULL);
            evas_gl_surface_destroy(wv->m_glEvasgl, wv->m_glSfc);
            evas_object_image_size_set(wv->m_graphicsAdapter, w, h);
            Evas_Native_Surface ns;
            wv->m_glSfc =
                evas_gl_surface_create(wv->m_glEvasgl, wv->m_glCfg, w, h);
            evas_gl_native_surface_get(wv->m_glEvasgl, wv->m_glSfc, &ns);
            evas_object_image_native_surface_set(wv->m_graphicsAdapter, &ns);
#endif
            wv->m_isRenderedOnce = false;

            STARFISH_LOG_INFO("WebViewEFL::resizeCallback::clearEvasGL %d %d",
                              w, h);

            wv->immediatelyClearScreen();
            wv->FetchWebContainer()->ResizeTo(w, h);
        };
        evas_object_event_callback_add(m_mainBox, EVAS_CALLBACK_RESIZE,
                                       m_resizeHandler, this);

        m_shownHandler = [](void* data, Evas* e, Evas_Object* obj,
                            void* event_info) {
            WebViewEFL* wv = (WebViewEFL*)data;
            STARFISH_LOG_INFO("WebViewEFL::shownCallback");
            wv->immediatelyClearScreen();
        };

        evas_object_event_callback_add(m_mainBox, EVAS_CALLBACK_SHOW,
                                       m_shownHandler, this);

        evas_object_event_callback_add(
            m_mainBox, EVAS_CALLBACK_MOVE,
            [](void* data, Evas* e, Evas_Object* obj, void* event_info) {
                WebViewEFL* wv = (WebViewEFL*)data;
                int x, y;
                evas_object_geometry_get(wv->m_mainBox, &x, &y, NULL, NULL);
                evas_object_move(wv->m_graphicsAdapter, x, y);
            },
            this);

        ecore_imf_init();
        // Register IMF callbacks
        if (ecore_imf_context_default_id_get()) {
            m_imfContext =
                ecore_imf_context_add(ecore_imf_context_default_id_get());
        } else {
            STARFISH_LOG_ERROR(
                "ecore_imf_context_default_id_get returns null.. use fallback "
                "method");
            m_imfContext = ecore_imf_context_add(getImfMethod());
        }

        ecore_imf_context_client_window_set(
            m_imfContext,
            (void*)ecore_evas_window_get(ecore_evas_ecore_evas_get(
                evas_object_evas_get(m_graphicsAdapter))));
        ecore_imf_context_client_canvas_set(
            m_imfContext, evas_object_evas_get(m_graphicsAdapter));

        // register commit event callback
        ecore_imf_context_event_callback_add(
            m_imfContext, ECORE_IMF_CALLBACK_COMMIT,
            [](void* data, Ecore_IMF_Context* ctx, void* event_info) {
                WebViewEFL* self = (WebViewEFL*)data;
                char* commit_str = (char*)event_info;
                STARFISH_LOG_INFO("ECORE_IMF_CALLBACK_COMMIT %s", commit_str);
                self->FetchWebContainer()->DispatchCompositionEndEvent(
                    commit_str);
            },
            this);

        // register preedit changed event handler
        ecore_imf_context_event_callback_add(
            m_imfContext, ECORE_IMF_CALLBACK_PREEDIT_CHANGED,
            [](void* data, Ecore_IMF_Context* ctx, void* event_info) {
                WebViewEFL* self = (WebViewEFL*)data;
                char* str = NULL;
                int cursor_pos;
                ecore_imf_context_preedit_string_get(self->m_imfContext, &str,
                                                     &cursor_pos);
                STARFISH_LOG_INFO("ECORE_IMF_CALLBACK_PREEDIT_CHANGED %s %d",
                                  str, cursor_pos);
                if (str) {
                    self->FetchWebContainer()->DispatchCompositionUpdateEvent(
                        str);
                    free(str);
                }
            },
            this);

        // register key event handler
        evas_object_event_callback_add(
            m_mainBox, EVAS_CALLBACK_KEY_DOWN,
            [](void* data, Evas* e, Evas_Object* obj, void* event_info) {
                WebViewEFL* wv = (WebViewEFL*)data;
                Evas_Event_Key_Down* ev = (Evas_Event_Key_Down*)event_info;
                STARFISH_LOG_INFO("EVAS_CALLBACK_KEY_DOWN for ime object [%s]",
                                  ev->key);

                if (evas_object_focus_get(wv->m_mainBox) == EINA_FALSE) {
                    return;
                }

#ifdef STARFISH_TIZEN_TV
                if ((strcmp(ev->key, "XF86Red") == 0)) {
                    ev->key = "Tab";
                }
#endif

                bool tryFilter = true;
                if ((strcmp(ev->key, "Tab") == 0)) {
                    tryFilter = false;
                }

                if ((strcmp(ev->key, "XF86Exit") == 0) ||
                    (strcmp(ev->key, "Select") == 0) ||
                    (strcmp(ev->key, "Cancel") == 0)) {
                    if (strcmp(ev->key, "Select") == 0) {
                        wv->FetchWebContainer()->AddIdleCallback(
                            [](void* data) {
                                WebViewEFL* self = (WebViewEFL*)data;
                                KeyValue kv = KeyValue::EnterKey;
                                self->FetchWebContainer()->DispatchKeyDownEvent(
                                    kv);
                                self->FetchWebContainer()
                                    ->DispatchKeyPressEvent(kv);
                                self->FetchWebContainer()->DispatchKeyUpEvent(
                                    kv);
                                self->HideSoftwareKeyboardIfPossible();
                            },
                            wv);
                    } else {
                        wv->FetchWebContainer()->AddIdleCallback(
                            [](void* data) {
                                WebViewEFL* self = (WebViewEFL*)data;
                                self->HideSoftwareKeyboardIfPossible();
                            },
                            wv);
                    }
                }

                // process non-char keys
                STARFISH_LOG_INFO("process non-char [%s]", ev->key);
                auto keyValue = ecoreEventKeyToKeyValue(
                    ev->key,
                    (evas_key_modifier_is_set(ev->modifiers, "Shift_L") ==
                     EINA_TRUE) ||
                        (evas_key_modifier_is_set(ev->modifiers, "Shift_R") ==
                         EINA_TRUE));
                if ((strcmp(ev->key, "Up") != 0) &&
                    (strcmp(ev->key, "Down") != 0)) {
                    wv->FetchWebContainer()->DispatchKeyDownEvent(keyValue);
                    wv->FetchWebContainer()->DispatchKeyPressEvent(keyValue);
                    wv->m_isKeyDown = true;
                }

                if (tryFilter && !isASCIIPrintableKey(keyValue)) {
                    Ecore_IMF_Event_Key_Down ecore_ev;
                    ecore_imf_evas_event_key_down_wrap(ev, &ecore_ev);
                    ecore_imf_context_filter_event(wv->m_imfContext,
                                                   ECORE_IMF_EVENT_KEY_DOWN,
                                                   (Ecore_IMF_Event*)&ecore_ev);
                }
            },
            this);
        evas_object_event_callback_add(
            m_mainBox, EVAS_CALLBACK_KEY_UP,
            [](void* data, Evas* e, Evas_Object* obj, void* event_info) {
                WebViewEFL* wv = (WebViewEFL*)data;
                Evas_Event_Key_Up* ev = (Evas_Event_Key_Up*)event_info;
                STARFISH_LOG_INFO("EVAS_CALLBACK_KEY_UP for ime object [%s]",
                                  ev->key);

                if (evas_object_focus_get(wv->m_mainBox) == EINA_FALSE) {
                    return;
                }

#ifdef STARFISH_TIZEN_TV
                if ((strcmp(ev->key, "XF86Red") == 0)) {
                    ev->key = "Tab";
                }
#endif

                bool tryFilter = true;

                if ((strcmp(ev->key, "Tab") == 0)) {
                    tryFilter = false;
                }

                if (tryFilter) {
                    Ecore_IMF_Event_Key_Up ecore_ev;
                    ecore_imf_evas_event_key_up_wrap(ev, &ecore_ev);
                    if (ecore_imf_context_filter_event(
                            wv->m_imfContext, ECORE_IMF_EVENT_KEY_UP,
                            (Ecore_IMF_Event*)&ecore_ev)) {
                        return;
                    }
                }

                if ((strcmp(ev->key, "Up") != 0) &&
                    (strcmp(ev->key, "Down") != 0)) {
                    // process non-char keys
                    auto keyValue = ecoreEventKeyToKeyValue(
                        ev->key,
                        (evas_key_modifier_is_set(ev->modifiers, "Shift_L") ==
                         EINA_TRUE) ||
                            (evas_key_modifier_is_set(ev->modifiers,
                                                      "Shift_R") == EINA_TRUE));
                    wv->FetchWebContainer()->DispatchKeyUpEvent(keyValue);
                    wv->m_isKeyDown = false;
                }
            },
            this);

        evas_object_event_callback_add(
            m_mainBox, EVAS_CALLBACK_FOCUS_IN,
            [](void* data, Evas* e, Evas_Object* obj, void* event_info) {
                WebViewEFL* wv = (WebViewEFL*)data;
                Ecore_IMF_Context* ctx = wv->m_imfContext;
                Ecore_IMF_Event_Key_Down ev;
                ecore_imf_evas_event_key_down_wrap(
                    (Evas_Event_Key_Down*)event_info, &ev);
                ecore_imf_context_reset(ctx);
                ecore_imf_context_focus_in(ctx);
                ecore_imf_context_show(ctx);
            },
            this);

        evas_object_event_callback_add(
            m_mainBox, EVAS_CALLBACK_FOCUS_OUT,
            [](void* data, Evas* e, Evas_Object* obj, void* event_info) {
                WebViewEFL* wv = (WebViewEFL*)data;
                Ecore_IMF_Context* ctx = wv->m_imfContext;
                Ecore_IMF_Event_Key_Down ev;

                if (ecore_imf_context_input_panel_state_get(ctx) ==
                    ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
                    ecore_imf_evas_event_key_down_wrap(
                        (Evas_Event_Key_Down*)event_info, &ev);
                    // ecore_imf_context_reset(ctx);
                    ecore_imf_context_focus_out(ctx);
                    ecore_imf_context_hide(ctx);
                }
            },
            this);

        ecore_imf_context_autocapital_type_set(m_imfContext,
                                               ECORE_IMF_AUTOCAPITAL_TYPE_NONE);
        ecore_imf_context_prediction_allow_set(m_imfContext, EINA_FALSE);

        float glScale = 1;
        if (getenv("LWE_GL_COMPOSITOR_SCALE")) {
            glScale = atof(getenv("LWE_GL_COMPOSITOR_SCALE"));
        }
        if (glScale != 1) {
            width = (unsigned)(width / glScale);
            height = (unsigned)(height / glScale);
            devicePixelRatio = 1 / glScale;
        }

        WebContainer::WebContainerArguments args{
            width,           height, devicePixelRatio,
            defaultFontName, locale, timezoneID,
        };

        WebContainer::RendererGLConfiguration config;
#if defined(STARFISH_UV_CAIRO_GL)
        config.onMakeCurrent = [this](WebContainer* wc) {
            m_uvPresenter->onMakeCurrent(wc->Width(), wc->Height());
        };
        config.onSwapBuffers = [this](WebContainer* wc, bool mayNeedsSync) {
            m_uvPresenter->onSwapBuffers();
            if (m_lastInputTime) {
                ANNOTATE_SETUP;
                ANNOTATE_CHANNEL_COLOR(3002, ANNOTATE_GREEN, "response time");
#ifdef STARFISH_ENABLE_PROFILE_TIMER
                uint64_t end = Starfish::longTickCount();
                float time = (float)((end - m_lastInputTime) / 1000.f);
                STARFISH_LOG_INFO("response time is %f ms", time);
#endif
                m_lastInputTime = 0;
                ANNOTATE_CHANNEL_END(3002);
            }
        };
        config.onCreateSharedContext = [this](WebContainer* wc) -> uintptr_t {
            return m_uvPresenter->createSharedContext();
        };
        config.onDestroyContext = [this](WebContainer* wc,
                                         uintptr_t context) -> bool {
            return m_uvPresenter->destroyContext(context);
        };
        config.onClearCurrentContext = [this](WebContainer* wc) -> bool {
            return m_uvPresenter->clearCurrent();
        };
        config.onMakeCurrentWithContext = [this](WebContainer* wc,
                                                 uintptr_t context) -> bool {
            return m_uvPresenter->makeCurrentWithContext(context);
        };
        config.onGetProcAddress = [this](WebContainer* wc,
                                         const char* name) -> void* {
            return reinterpret_cast<void*>(eglGetProcAddress(name));
        };
        config.onIsSupportedExtension = [this](WebContainer* wc,
                                               const char* extension) -> bool {
            const char* extensions =
                reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
            return extensions && strstr(extensions, extension) != nullptr;
        };
#else
        config.onMakeCurrent = [this](WebContainer* wc) {
            evas_gl_make_current(m_glEvasgl, m_glSfc, m_glCtx);

            if (m_glSync) {
                Starfish::LongTaskFinder t("evasglWaitSync");
                m_glGlapi->evasglClientWaitSync(
                    m_glEvasgl, m_glSync, EVAS_GL_SYNC_PRIOR_COMMANDS_COMPLETE,
                    EVAS_GL_FOREVER);
                m_glGlapi->evasglDestroySync(m_glEvasgl, m_glSync);
                m_glSync = nullptr;
            }
        };
        config.onSwapBuffers = [this](WebContainer* wc, bool mayNeedsSync) {
        // Since tizen 9, we always needs glFence
#if defined(STARFISH_TIZEN_MAJOR_VERSION) && STARFISH_TIZEN_MAJOR_VERSION >= 9
            mayNeedsSync = true;
#endif
            if (mayNeedsSync && m_glGlapi->evasglCreateSync && !m_glSync) {
                int attr[] = { EVAS_GL_NONE };
                m_glSync = m_glGlapi->evasglCreateSync(
                    m_glEvasgl, EVAS_GL_SYNC_FENCE, attr);
            }
            if (m_lastInputTime) {
                ANNOTATE_SETUP;
                ANNOTATE_CHANNEL_COLOR(3002, ANNOTATE_GREEN, "response time");
#ifdef STARFISH_ENABLE_PROFILE_TIMER
                uint64_t end = Starfish::longTickCount();
                float time = (float)((end - m_lastInputTime) / 1000.f);
                STARFISH_LOG_INFO("response time is %f ms", time);
#endif
                m_lastInputTime = 0;
                ANNOTATE_CHANNEL_END(3002);
            }
        };
        config.onCreateSharedContext = [this](WebContainer* wc) -> uintptr_t {
            Evas_GL_Context* sharedContext = nullptr;
            sharedContext = evas_gl_context_version_create(
                m_glEvasgl, m_glCtx, Evas_GL_Context_Version::EVAS_GL_GLES_3_X);
            if (sharedContext == nullptr) {
                sharedContext = evas_gl_context_version_create(
                    m_glEvasgl, m_glCtx,
                    Evas_GL_Context_Version::EVAS_GL_GLES_2_X);
            }
            STARFISH_ASSERT(sharedContext != nullptr);
            return reinterpret_cast<uintptr_t>(sharedContext);
        };
        config.onDestroyContext = [this](WebContainer* wc,
                                         uintptr_t context) -> bool {
            evas_gl_context_destroy(
                m_glEvasgl, reinterpret_cast<Evas_GL_Context*>(context));
            return true;
        };
        config.onClearCurrentContext = [this](WebContainer* wc) -> bool {
            return evas_gl_make_current(m_glEvasgl, nullptr, nullptr);
        };
        config.onMakeCurrentWithContext = [this](WebContainer* wc,
                                                 uintptr_t context) -> bool {
            return evas_gl_make_current(
                m_glEvasgl, m_glSfc,
                reinterpret_cast<Evas_GL_Context*>(context));
        };

        config.onIsSupportedExtension = [this](WebContainer* wc,
                                               const char* extension) -> bool {
#if defined(STARFISH_TIZEN)
            // The retuned string of evas_gl_string_query is never contain this
            // extension. However, we should assume that Tizen supports this.
            if (strncmp(extension, "EVAS_GL_TIZEN_image_native_surface", 34) ==
                0) {
                return true;
            }
#endif
            const char* extensions =
                evas_gl_string_query(m_glEvasgl, EVAS_GL_EXTENSIONS);
            return strstr(extensions, extension) != nullptr;
        };
#endif

        WebContainer* webContainer = WebContainer::CreateGL(args, config);

        m_pixelDirtyCallback = [](void* data, Evas_Object* o) {
            // We need to draw every time for preventing screen blinking
            WebViewEFL* s = (WebViewEFL*)data;
            if (s->m_lastDoRenderingFunction && !s->m_isDestroyed) {
                s->m_isRenderedOnce = true;
                s->m_lastDoRenderingFunction();
            }
        };
#if !defined(STARFISH_UV_CAIRO_GL)
        // glib: rendering is pulled by Evas via the pixels-get callback. uv
        // does not register this, so the engine self-drives rendering on its
        // LWE thread; presentation happens via the TBM native surface set in
        // UvTbmPresenter::onTick (vsync animator, main thread).
#if !(defined(STARFISH_TIZEN) && defined(STARFISH_ENABLE_TEST))
        webContainer->RegisterSetNeedsRenderingCallback(
            [this](WebContainer* wc,
                   const std::function<void()>& doRenderingFunction) {
                evas_object_image_pixels_dirty_set(m_graphicsAdapter,
                                                   EINA_TRUE);
                evas_object_image_pixels_get_callback_set(
                    m_graphicsAdapter, m_pixelDirtyCallback, this);
                m_lastDoRenderingFunction = doRenderingFunction;
            });
#endif
        evas_object_image_pixels_get_callback_set(m_graphicsAdapter,
                                                  m_pixelDirtyCallback, this);
#endif

        webContainer->RegisterOnShowSoftwareKeyboardIfPossibleHandler(
            [this](WebContainer*) { ShowSoftwareKeyboardIfPossible(); });

        webContainer->RegisterOnHideSoftwareKeyboardIfPossibleHandler(
            [this](WebContainer* t) { HideSoftwareKeyboardIfPossible(); });

        webContainer->RegisterGetScreenMatrixHandler(
            [this](WebContainer*) -> WebContainer::TransformationMatrix {
#if defined(STARFISH_UV_CAIRO_GL)
                int degrees = 0;
#else
                int degrees = evas_gl_rotation_get(m_glEvasgl);
#endif
                if (m_evasGlRotationDegrees != degrees) {
                    m_evasGlRotationDegrees = degrees;
                    int width = 0, height = 0;
                    evas_object_geometry_get(m_mainBox, nullptr, nullptr,
                                             &width, &height);
                    updateScreenMatrix(m_evasGlRotationDegrees, width, height);
                }
                return m_screenMatrix;
            });

        m_hideKeyboardTimeoutId = m_keyboardTimeoutId = SIZE_MAX;
        SetWebContainer(webContainer);

#if !defined(STARFISH_UV_CAIRO_GL)
        // Routes the engine's GL calls through EvasGL. uv intentionally omits
        // this so GL::create falls back to GenericGL (raw GLES) on the LWE
        // thread via config.onGetProcAddress.
        webContainer->SetUserData("__internalLWEWebViewEvasGLAPI", m_glGlapi);
#endif

        webContainer->SetUserData(
            "__internalLWEWebViewEFLNativeWindowEvasObject", win);

#if defined(STARFISH_TIZEN_MAJOR_VERSION) && STARFISH_TIZEN_MAJOR_VERSION >= 5
        webContainer->SetUserData(
            "__internalLWEWebViewEFLEcoreWaylandHandle",
            ecore_evas_wayland2_window_get(
                ecore_evas_ecore_evas_get(evas_object_evas_get(win))));
#elif defined(STARFISH_TIZEN_MAJOR_VERSION) && STARFISH_TIZEN_MAJOR_VERSION == 4
        webContainer->SetUserData(
            "__internalLWEWebViewEFLEcoreWaylandHandle",
            ecore_evas_wayland_window_get(
                ecore_evas_ecore_evas_get(evas_object_evas_get(win))));
#endif
    }

    virtual void Destroy() override
    {
        m_isDestroyed = true;

        Blur();

        FetchWebContainer()->Destroy();

        if (m_imfContext) {
            ecore_imf_context_del(m_imfContext);
        }

        ecore_imf_shutdown();

        evas_object_hide(m_graphicsAdapter);

        if (m_immediatelyClearScreenAnimator) {
            ecore_animator_freeze(m_immediatelyClearScreenAnimator);
            ecore_animator_del(m_immediatelyClearScreenAnimator);
            m_immediatelyClearScreenAnimator = nullptr;
        }
#if defined(STARFISH_UV_CAIRO_GL)
        if (m_uvPresenter) {
            // Stop the present animator before the canvas goes away.
            m_uvPresenter->shutdownMainThreadResources();
        }
        evas_object_image_native_surface_set(m_graphicsAdapter, NULL);
        // NOTE: m_uvPresenter owns GL/TBM resources on the LWE thread; it is
        // intentionally leaked here rather than torn down from the main thread
        // (cross-thread GL teardown would need an LWE-thread round-trip).
#else
        if (m_glSync) {
            m_glGlapi->evasglDestroySync(m_glEvasgl, m_glSync);
        }
        evas_object_image_native_surface_set(m_graphicsAdapter, NULL);
        evas_gl_surface_destroy(m_glEvasgl, m_glSfc);
        evas_gl_context_destroy(m_glEvasgl, m_glCtx);
        evas_gl_config_free(m_glCfg);
        evas_gl_free(m_glEvasgl);
#endif

        if (m_resizeHandler) {
            evas_object_event_callback_del(m_mainBox, EVAS_CALLBACK_RESIZE,
                                           m_resizeHandler);
        }

        if (m_shownHandler) {
            evas_object_event_callback_del(m_mainBox, EVAS_CALLBACK_SHOW,
                                           m_shownHandler);
        }

        if (m_mouseDownEventHandler) {
            evas_object_event_callback_del(m_graphicsAdapter,
                                           EVAS_CALLBACK_MOUSE_DOWN,
                                           m_mouseDownEventHandler);
        }

        if (m_mouseUpEventHandler) {
            evas_object_event_callback_del(m_graphicsAdapter,
                                           EVAS_CALLBACK_MOUSE_UP,
                                           m_mouseUpEventHandler);
        }

        if (m_mouseWheelEventHandler) {
            evas_object_event_callback_del(m_graphicsAdapter,
                                           EVAS_CALLBACK_MOUSE_WHEEL,
                                           m_mouseWheelEventHandler);
        }

        if (m_mouseMoveEventHandler) {
            evas_object_event_callback_del(m_graphicsAdapter,
                                           EVAS_CALLBACK_MOUSE_MOVE,
                                           m_mouseMoveEventHandler);
        }

        evas_object_event_callback_del(m_windowObject, EVAS_CALLBACK_SHOW,
                                       m_windowShownHandler);
        evas_object_event_callback_del(m_windowObject, EVAS_CALLBACK_HIDE,
                                       m_windowHiddenHandler);
        evas_object_smart_callback_del(m_windowObject, "rotation,changed",
                                       m_windowRotaionChangedHandler);

        evas_object_event_callback_del(
            m_nonIMEKeyEventBox, EVAS_CALLBACK_KEY_DOWN, m_keyDownEventHandler);
        evas_object_event_callback_del(
            m_nonIMEKeyEventBox, EVAS_CALLBACK_KEY_UP, m_keyUpEventHandler);

        evas_object_event_callback_del(m_windowObject, EVAS_CALLBACK_DEL,
                                       m_windowDelEventHandler);

        if (m_graphicsAdapter) {
            evas_object_del(m_graphicsAdapter);
            m_graphicsAdapter = nullptr;
        }

        if (m_nonIMEKeyEventBox) {
            evas_object_del(m_nonIMEKeyEventBox);
            m_nonIMEKeyEventBox = nullptr;
        }

        if (m_mainBox) {
            evas_object_del(m_mainBox);
            m_mainBox = nullptr;
        }

        if (m_buttonForClickCipper) {
            evas_object_del(m_buttonForClickCipper);
            m_buttonForClickCipper = nullptr;
        }

        if (m_buttonForClickClickEventHandler) {
            evas_object_smart_callback_del(m_buttonForClick, "clicked",
                                           m_buttonForClickClickEventHandler);
        }

        if (m_buttonForClickMouseDownEventHandler) {
            evas_object_event_callback_del(
                m_buttonForClick, EVAS_CALLBACK_MOUSE_DOWN,
                m_buttonForClickMouseDownEventHandler);
        }

        if (m_buttonForClickMouseMoveEventHandler) {
            evas_object_event_callback_del(
                m_buttonForClick, EVAS_CALLBACK_MOUSE_MOVE,
                m_buttonForClickMouseMoveEventHandler);
        }

        if (m_buttonForClickMouseUpEventHandler) {
            evas_object_event_callback_del(m_buttonForClick,
                                           EVAS_CALLBACK_MOUSE_UP,
                                           m_buttonForClickMouseUpEventHandler);
        }

        if (m_buttonForClick) {
            evas_object_del(m_buttonForClick);
            m_buttonForClick = nullptr;
        }

        delete this;
    }

    virtual void* Unwrap() override
    {
        return m_mainBox;
    }

    virtual void Focus() override
    {
        WebViewImpl::Focus();

        evas_object_focus_set(m_mainBox, EINA_FALSE);
        evas_object_focus_set(m_nonIMEKeyEventBox, EINA_TRUE);
    }

    virtual void Blur() override
    {
        WebViewImpl::Blur();

        evas_object_focus_set(m_mainBox, EINA_FALSE);
        evas_object_focus_set(m_nonIMEKeyEventBox, EINA_FALSE);
    }

    void ShowSoftwareKeyboardIfPossible()
    {
        if (ecore_imf_input_panel_hide() == EINA_FALSE) {
            FetchWebContainer()->AddIdleCallback(
                [](void* data) {
                    WebViewEFL* self = (WebViewEFL*)data;
                    evas_object_focus_set(self->m_nonIMEKeyEventBox,
                                          EINA_FALSE);
                    evas_object_focus_set(self->m_mainBox, EINA_TRUE);
                },
                this);
            FetchWebContainer()->ClearTimeout(m_keyboardTimeoutId);
            m_keyboardTimeoutId = SIZE_MAX;
        } else {
            m_keyboardTimeoutId = FetchWebContainer()->AddTimeout(
                [](void* data) {
                    WebViewEFL* self = (WebViewEFL*)data;
                    self->ShowSoftwareKeyboardIfPossible();
                    self->m_keyboardTimeoutId = SIZE_MAX;
                },
                this, 100);
        }
    }

    void HideSoftwareKeyboardIfPossible()
    {
        FetchWebContainer()->ClearTimeout(m_hideKeyboardTimeoutId);
        m_hideKeyboardTimeoutId = FetchWebContainer()->AddTimeout(
            [](void* data) {
                WebViewEFL* self = (WebViewEFL*)data;
                evas_object_focus_set(self->m_mainBox, EINA_FALSE);
                evas_object_focus_set(self->m_nonIMEKeyEventBox, EINA_TRUE);
                self->m_hideKeyboardTimeoutId = SIZE_MAX;
            },
            this, 100);
    }

protected:
    Evas_Object* m_nonIMEKeyEventBox;

    void (*m_resizeHandler)(void* data, Evas* evas, Evas_Object* obj,
                            void* event_info);
    void (*m_shownHandler)(void* data, Evas* evas, Evas_Object* obj,
                           void* event_info);
    void (*m_mouseDownEventHandler)(void* data, Evas* evas, Evas_Object* obj,
                                    void* event_info);
    void (*m_mouseMoveEventHandler)(void* data, Evas* evas, Evas_Object* obj,
                                    void* event_info);
    void (*m_mouseUpEventHandler)(void* data, Evas* evas, Evas_Object* obj,
                                  void* event_info);
    void (*m_mouseWheelEventHandler)(void* data, Evas* evas, Evas_Object* obj,
                                     void* event_info);
    void (*m_keyDownEventHandler)(void* data, Evas* evas, Evas_Object* obj,
                                  void* event_info);
    void (*m_keyUpEventHandler)(void* data, Evas* evas, Evas_Object* obj,
                                void* event_info);
    void (*m_windowDelEventHandler)(void* data, Evas* evas, Evas_Object* obj,
                                    void* event_info);
    void (*m_windowShownHandler)(void* data, Evas* evas, Evas_Object* obj,
                                 void* event_info);
    void (*m_windowHiddenHandler)(void* data, Evas* evas, Evas_Object* obj,
                                  void* event_info);
    void (*m_windowRotaionChangedHandler)(void* data, Evas_Object* object,
                                          void* event_info);
    void (*m_buttonForClickClickEventHandler)(void* data, Evas_Object* obj,
                                              void* event_info);
    void (*m_buttonForClickMouseDownEventHandler)(void* data, Evas* evas,
                                                  Evas_Object* obj,
                                                  void* event_info);
    void (*m_buttonForClickMouseMoveEventHandler)(void* data, Evas* evas,
                                                  Evas_Object* obj,
                                                  void* event_info);
    void (*m_buttonForClickMouseUpEventHandler)(void* data, Evas* evas,
                                                Evas_Object* obj,
                                                void* event_info);

    Evas_Object* m_windowObject;
    Evas_Object* m_mainBox;
    Evas_Object* m_buttonForClick;
    Evas_Object* m_buttonForClickCipper;
    Evas_Object* m_graphicsAdapter;
#if defined(STARFISH_UV_CAIRO_GL)
    UvTbmPresenter* m_uvPresenter;
#else
    Evas_GL_Context* m_glCtx;
    Evas_GL_Surface* m_glSfc;
    Evas_GL_Config* m_glCfg;
    Evas_GL* m_glEvasgl;
    Evas_GL_API* m_glGlapi;
    EvasGLSync m_glSync;
#endif
    bool m_isRenderedOnce;
    void (*m_pixelDirtyCallback)(void* data, Evas_Object* o);
    Ecore_Animator* m_immediatelyClearScreenAnimator;

    void immediatelyClearScreen()
    {
#if defined(STARFISH_UV_CAIRO_GL)
        // No EvasGL surface to clear; the engine will render the first frame
        // into a TBM buffer and present it. Nothing to do here.
#else
        evas_object_image_pixels_dirty_set(m_graphicsAdapter, EINA_TRUE);
        if (!m_isRenderedOnce) {
            evas_object_image_pixels_get_callback_set(
                m_graphicsAdapter,
                [](void* data, Evas_Object* o) {
                    WebViewEFL* wv = (WebViewEFL*)data;
                    evas_gl_make_current(wv->m_glEvasgl, wv->m_glSfc,
                                         wv->m_glCtx);
                    wv->m_glGlapi->glClearColor(0, 0, 0, 0);
                    wv->m_glGlapi->glClear(GL_COLOR_BUFFER_BIT);
                    wv->m_glGlapi->glFlush();

                    evas_object_image_pixels_get_callback_set(
                        wv->m_graphicsAdapter, wv->m_pixelDirtyCallback, wv);
                    if (!wv->m_immediatelyClearScreenAnimator) {
                        wv->m_immediatelyClearScreenAnimator =
                            ecore_animator_add(
                                [](void* data) -> Eina_Bool {
                                    WebViewEFL* wv = (WebViewEFL*)data;
                                    evas_object_image_pixels_dirty_set(
                                        wv->m_graphicsAdapter, EINA_TRUE);
                                    ecore_animator_freeze(
                                        wv->m_immediatelyClearScreenAnimator);
                                    ecore_animator_del(
                                        wv->m_immediatelyClearScreenAnimator);
                                    wv->m_immediatelyClearScreenAnimator =
                                        nullptr;
                                    return ECORE_CALLBACK_CANCEL;
                                },
                                wv);
                    }
                },
                this);
        }
#endif
    }

    void updateScreenMatrix(int degrees, int width, int height)
    {
        double translateX = 0.0, translateY = 0.0;
        if (degrees == 90) {
            translateY = height;
        } else if (degrees == 180) {
            translateX = width;
            translateY = height;
        } else if (degrees == 270) {
            translateX = width;
        }

        double scaleX = 1.0, scaleY = 1.0;
        if (degrees % 180 == 90) {
            scaleX = height / static_cast<double>(width);
            scaleY = width / static_cast<double>(height);
        }

        double radians = (360 - degrees) * M_PI / 180;
        // clang-format off
        m_screenMatrix = {
            cos(radians) * scaleY, -sin(radians) * scaleY, translateX, // x
            sin(radians) * scaleX, cos(radians) * scaleX, translateY, // y
            0.0, 0.0, 1.0 // perspective
        };
        // clang-format on
    }

    Ecore_IMF_Context* m_imfContext;

    float m_lastMouseX, m_lastMouseY;
    bool m_isMouseLbuttonDown;
    bool m_isKeyDown;
    bool m_isDestroyed;
    uint32_t m_lastClickedTimestamp;
    uint32_t m_clickedCount;
    uint32_t m_lastKeyPressedTimestamp;
    uint64_t m_lastRenderingTime;
    uint64_t m_lastInputTime;
    int m_offsetYDueToSoftwareKeyboard;
    size_t m_keyboardTimeoutId;
    size_t m_hideKeyboardTimeoutId;
    int m_evasGlRotationDegrees;
    WebContainer::TransformationMatrix m_screenMatrix;

    std::function<void()> m_lastDoRenderingFunction;
};

WebView* WebView::Create(void* win, unsigned x, unsigned y, unsigned width,
                         unsigned height, float devicePixelRatio,
                         const char* defaultFontName, const char* locale,
                         const char* timezoneID)
{
    return new WebViewEFL(win, x, y, width, height, devicePixelRatio,
                          defaultFontName, locale, timezoneID);
}
} // namespace LWEDelegate

#endif
