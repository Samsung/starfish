/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_AUDIO_WORKLET)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/modules/webaudio/AudioParamMap.h"

#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/AudioParam.h"

namespace Starfish {

class AudioParamMapIterationSource final
    : public IterationSource<Optional<String*>, Optional<AudioParam*>> {
public:
    explicit AudioParamMapIterationSource(AudioParamMap* map)
        : m_map(map)
    {
    }

    bool next(Escargot::ExecutionStateRef*, Optional<String*>& key,
              Optional<AudioParam*>& value) override
    {
        if (m_index >= m_map->size()) {
            return false;
        }
        key = m_map->keyAt(m_index);
        value = m_map->get(key.value());
        m_index++;
        return true;
    }

private:
    // Keeps the map alive while an iterator over it is.
    AudioParamMap* m_map;
    size_t m_index{ 0 };
};

AudioParamMap::AudioParamMap(ExecutionContext* executionContext)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
{
}

ScriptBindingInstance* AudioParamMap::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

IterationSource<Optional<String*>, Optional<AudioParam*>>*
AudioParamMap::startIteration(Escargot::ExecutionStateRef*)
{
    return new AudioParamMapIterationSource(this);
}

Optional<AudioParam*> AudioParamMap::get(String* key)
{
    for (const auto& entry : m_entries) {
        if (entry.first->equals(key)) {
            return entry.second;
        }
    }
    return nullptr;
}

bool AudioParamMap::has(String* key)
{
    return get(key).hasValue();
}

void AudioParamMap::set(String* key, AudioParam* value)
{
    m_entries.push_back(std::make_pair(key, value));
}
} // namespace Starfish

#endif
