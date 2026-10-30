/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_AUDIO_WORKLET)

#ifndef __StarfishAudioParamMap__
#define __StarfishAudioParamMap__

#include "binding/ScriptWrappable.h"
#include "binding/Maplike.h"
// The generated maplike binding converts values with AudioParam's members.
#include "core/modules/webaudio/AudioParam.h"

namespace Starfish {
class ExecutionContext;

// https://webaudio.github.io/web-audio-api/#AudioParamMap
// Read-only to script; filled once by the AudioWorkletNode constructor.
class AudioParamMap : public ScriptWrappable,
                      public Maplike<String*, AudioParam*> {
public:
    explicit AudioParamMap(ExecutionContext* executionContext);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(AudioParamMap)

    uint32_t size() const
    {
        return static_cast<uint32_t>(m_entries.size());
    }
    String* keyAt(size_t index) const
    {
        return m_entries[index].first;
    }
    IterationSource<Optional<String*>, Optional<AudioParam*>>* startIteration(
        Escargot::ExecutionStateRef* state) override;
    Optional<AudioParam*> get(String* key) override;
    bool has(String* key) override;
    // Appends; the constructor never adds a name twice.
    void set(String* key, AudioParam* value) override;
    bool deleteItem(String*) override
    {
        return false;
    }
    void clear() override
    {
    }

private:
    ExecutionContext* m_executionContext;
    GCVector<std::pair<String*, AudioParam*>> m_entries;
};
} // namespace Starfish

#endif
#endif
