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

#ifdef STARFISH_ENABLE_TTS
#ifndef __StarfishSpeechSynthesis__
#define __StarfishSpeechSynthesis__

#include "binding/ScriptWrappable.h"
#include "core/dom/EventTarget.h"
#include "binding/WindowHoldable.h"

namespace Starfish {

class String;

class SpeechSynthesisVoice : public ScriptWrappable, public WindowHoldable {
public:
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSpeechSynthesisVoice() const override;

    SpeechSynthesisVoice(Window* window)
        : ScriptWrappable(this)
        , WindowHoldable(window)
        , m_voiceURI(String::emptyString)
        , m_name(String::emptyString)
        , m_lang(String::emptyString)
        , m_localService(true)
        , m_isDefault(false)
    {
    }

    SpeechSynthesisVoice(Window* window, String* voiceURI, String* name,
                         String* lang, bool localService, bool isDefault)
        : ScriptWrappable(this)
        , WindowHoldable(window)
        , m_voiceURI(voiceURI)
        , m_name(name)
        , m_lang(lang)
        , m_localService(localService)
        , m_isDefault(isDefault)
    {
    }

    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
        return WindowHoldable::scriptBindingInstance();
    }

    String* voiceURI()
    {
        return m_voiceURI;
        // return m_platform->voicdURI();
    }
    String* name()
    {
        return m_name;
    }
    String* lang()
    {
        return m_lang;
    }
    bool localService()
    {
        return m_localService;
    }
    bool isDefault()
    {
        return m_isDefault;
    }

protected:
    String* m_voiceURI;
    String* m_name;
    String* m_lang;
    bool m_localService;
    bool m_isDefault;
};

class SpeechSynthesisUtterance : public EventTarget {
public:
    SpeechSynthesisUtterance(Document* document,
                             String* text = String::emptyString)
        : EventTarget(document)
        , m_id(0)
        , m_text(text)
        , m_lang(String::emptyString)
        , m_voice(nullptr)
        , m_volume(1.0)
        , m_rate(1.0)
        , m_pitch(1.0)
        , m_startTime(0.0)
    {
    }

    virtual ~SpeechSynthesisUtterance()
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSpeechSynthesisUtterance() const override;

    void setId(int id)
    {
        m_id = id;
    }

    int id()
    {
        return m_id;
    }

    void setText(String* text)
    {
        m_text = text;
    }

    String* text()
    {
        return m_text;
    }

    void setLang(String* lang)
    {
        m_lang = lang;
    }

    String* lang()
    {
        return m_lang;
    }

    void setVoice(SpeechSynthesisVoice* voice)
    {
        m_voice = voice;
        if (m_voice) {
            m_lang = m_voice->lang();
        }
    }

    SpeechSynthesisVoice* voice()
    {
        return m_voice;
    }

    void setVolume(float volume)
    {
        m_volume = volume;
    }

    float volume()
    {
        return m_volume;
    }

    void setRate(float rate)
    {
        m_rate = rate;
    }

    float rate()
    {
        return m_rate;
    }

    void setPitch(float pitch)
    {
        m_pitch = pitch;
    }

    float pitch()
    {
        return m_pitch;
    }

    double startTime()
    {
        return m_startTime;
    }

    void setStartTime(double t)
    {
        m_startTime = t;
    }

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(start);
    DECLARE_EVENT_LISTENER(end);
    DECLARE_EVENT_LISTENER(error);
    DECLARE_EVENT_LISTENER(pause);
    DECLARE_EVENT_LISTENER(resume);
//    DECLARE_EVENT_LISTENER(mark);
//    DECLARE_EVENT_LISTENER(boundary);
#undef VIRTUAL
#undef OVERRIDE

private:
    int m_id;
    String* m_text;
    String* m_lang;
    SpeechSynthesisVoice* m_voice;
    float m_volume;
    float m_rate;
    float m_pitch;
    double m_startTime;
};

class SpeechSynthesis : public EventTarget {
public:
    SpeechSynthesis(Document* document)
        : EventTarget(document)
        , m_isCreatedVoiceList(false)
    {
    }

    virtual ~SpeechSynthesis()
    {
    }

    void dispose()
    {
        m_voiceList.clear();
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSpeechSynthesis() const override;

    bool paused();
    bool speaking();
    bool pending();

    void speak(SpeechSynthesisUtterance* u);
    void cancel();
    void pause();
    void resume();
    GCVector<SpeechSynthesisVoice*>& getVoices();
#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(voiceschanged);
#undef VIRTUAL
#undef OVERRIDE

private:
    bool m_isCreatedVoiceList;
    GCVector<SpeechSynthesisVoice*> m_voiceList;
};
}
#endif
#endif // STARFISH_ENABLE_TTS
