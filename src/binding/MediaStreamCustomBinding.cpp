/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_WEBRTC)

#include "StarfishConfig.h"
#include "core/dom/DOMException.h"
#include "core/modules/mediastream/MediaStream.h"
#include <EscargotPublic.h>
using namespace Escargot;

namespace Starfish {
ValueRef* mediastreamConstructor(ExecutionStateRef* state, ValueRef* thisValue,
                                 size_t argc, ValueRef** argv,
                                 bool isNewExpression)
{
    if (!isNewExpression) {
        COMPOSE_MESSAGE(msg, CALLED_CONSTRUCTOR_WITHOUT_NEW, "MediaStream");
        THROW_EXCEPTION(msg);
    }

    if (argc == 0) {
        // Handle 'Constructor'
        MediaStream* result = nullptr;
        ExecutionContext* callWith = fetchExecutionContext(state->context());
        result = new MediaStream(callWith);
        return result->scriptValue();
    } else {
        ValueRef* arg = argv[0];
        if (arg->isObject()) {
            // Handle 'Constructor (MediaStream stream)'
            if (arg->asObject()->extraData()) {
                ScriptWrappable* scriptWrappable =
                    static_cast<ScriptWrappable*>(arg->asObject()->extraData());
                if (scriptWrappable->isMediaStream()) {
                    ExecutionContext* callWith =
                        fetchExecutionContext(state->context());
                    MediaStream* result = new MediaStream(
                        callWith, *scriptWrappable->asMediaStream());
                    return result->scriptValue();
                }
            } else {
                // Handle 'Constructor (sequence<MediaStreamTrack> tracks)'
                int length = static_cast<int>(
                    arg->asObject()
                        ->get(state, ValueRef::create(
                                         StringRef::createFromASCII("length")))
                        ->toNumber(state));
                GCVector<MediaStreamTrack*> mediaStreamTracks;
                for (int i = 0; i < length; i++) {
                    ValueRef* valueRef =
                        arg->asObject()->get(state, ValueRef::create(i));
                    bool isValidItem = false;
                    if (valueRef->isObject() &&
                        valueRef->asObject()->extraData()) {
                        ScriptWrappable* scriptWrappable =
                            static_cast<ScriptWrappable*>(
                                valueRef->asObject()->extraData());
                        if (scriptWrappable->isMediaStreamTrack()) {
                            MediaStreamTrack* mediaStreamTrack =
                                static_cast<MediaStreamTrack*>(
                                    valueRef->asObject()->extraData());
                            mediaStreamTracks.push_back(mediaStreamTrack);
                            isValidItem = true;
                        }
                    }

                    if (!isValidItem) {
                        THROW_EXCEPTION(ILLEGAL_INVOKE);
                    }
                }
                ExecutionContext* callWith =
                    fetchExecutionContext(state->context());
                MediaStream* result =
                    new MediaStream(callWith, mediaStreamTracks);
                return result->scriptValue();
            }
        }
    }
    THROW_EXCEPTION(ILLEGAL_INVOKE);
}

} // namespace Starfish

#endif
