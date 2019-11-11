/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#if defined(STARFISH_ENABLE_WEBRTC)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/modules/mediastream/MediaDevices.h"

#include "EscargotPublic.h"
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/page/Navigator.h"
#include "core/page/WebBase.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/mediastream/MediaStream.h"

namespace Starfish {

MediaDevices* Navigator::mediaDevices()
{
    if (m_mediaDevices == nullptr) {
        m_mediaDevices = new MediaDevices(executionContext(), m_document);
    }
    return m_mediaDevices;
}

MediaDevices::MediaDevices(ExecutionContext* executionContext,
                           Document* document)
    : EventTarget()
    , DocumentHoldable(document)
    , m_executionContext(executionContext)
{
}

MediaDevices::~MediaDevices()
{
}

ScriptBindingInstance* MediaDevices::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

ExecutionContext* MediaDevices::executionContext() const
{
    return m_executionContext;
}

// https://w3c.github.io/mediacapture-main/#dom-mediadevices-getusermedia
Promise* MediaDevices::getUserMedia(MediaStreamConstraints constraints)
{
    Promise* promise = new Promise(scriptBindingInstance());

    struct Params {
        MediaDevices* self;
        MediaStreamConstraints constraints;
    };
    Params* p = new Params();
    p->self = this;
    p->constraints = constraints;

    executionContext()->webBase()->messageLoop()->addIdler(
        executionContext()->globalScope(),
        [](size_t handle, void* data1, void* data2) {
            Promise* promise = castTo<Promise*>(data1);
            Params* p = castTo<Params*>(data2);
            MediaDevices* md = p->self;
            MediaStreamConstraints constraints = p->constraints;
            delete p;

            // 1-3: TODO: Accept constraint sets
            if (!constraints.m_audio && !constraints.m_video) {
                auto exception = new DOMException(md->executionContext(),
                                                  DOMException::SCRIPT_TYPE_ERR,
                                                  "TypeError");
                promise->reject(exception->scriptValue());
                return;
            }

            // 4
            if (!md->document()->isFullyActive()) {
                auto exception = new DOMException(
                    md->executionContext(), DOMException::INVALID_STATE_ERR,
                    "InvalidStateError");
                promise->reject(exception->scriptValue());
                return;
            }

            // 5-6.3.1
            MediaStream* mediaStream = new MediaStream(md->executionContext());
            if (constraints.audio()) {
                AudioStreamTrack* audioTrack =
                    new AudioStreamTrack(md->executionContext());
                mediaStream->addTrack(audioTrack);
            }

            if (constraints.video()) {
                WebCamStreamTrack* videoTrack =
                    new WebCamStreamTrack(md->executionContext());
                if (!videoTrack->backend()) {
                    auto exception = new DOMException(
                        md->executionContext(), DOMException::NOT_FOUND_ERR,
                        "NotFoundErr: video is not supported");
                    promise->reject(exception->scriptValue());
                    return;
                }

                // 6.3.2-6.10: TODO: Support constraints and permissions
                // 7
                STARFISH_LOG_INFO("%s: video successful\n", __func__);
                MediaStream* mediaStream =
                    new MediaStream(md->executionContext());
                mediaStream->addTrack(videoTrack);
            }
            promise->fulfill(mediaStream->scriptValue());
        },
        promise, p);

    return promise;
}
} // namespace Starfish

#endif
