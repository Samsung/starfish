#ifndef __StarFishHTMLAudioElement__
#define __StarFishHTMLAudioElement__

#include "dom/HTMLElement.h"
#ifdef STARFISH_TIZEN_WEARABLE
#include <player.h>
#endif

namespace StarFish {

class HTMLAudioElement : public HTMLElement {
public:
    HTMLAudioElement(Document* document)
        : HTMLElement(document)
    {
        initScriptWrappable(this);
        m_isPlaying = false;

#ifdef STARFISH_TIZEN_WEARABLE
        player_create(&m_player);
        if (player_create(&m_player) != PLAYER_ERROR_NONE) {
            STARFISH_LOG_INFO("JMP, Can not create player handler.");
        }
        if (player_set_completed_cb(m_player, _on_player_completed_cb, nullptr) != PLAYER_ERROR_NONE) {
            STARFISH_LOG_INFO("JMP, failed to register player state changed cb");
        }

        player_state_e player_state;
        if (player_get_state(m_player, &player_state) != PLAYER_ERROR_NONE) {
            STARFISH_LOG_INFO("JMP, failed to get player state");
        }

        if (player_state != PLAYER_STATE_IDLE) {
            if (player_unprepare(m_player) != PLAYER_ERROR_NONE) {
                STARFISH_LOG_INFO("JMP, unprepare error");
            }
        }
#endif
    }

    static void _on_player_completed_cb(void* user_data)
    {
        STARFISH_LOG_INFO("JMP, HTMLAudioElement::_on_player_completed_cb, start\n");
    }

    /* 4.4 Interface Node */

    virtual String* localName()
    {
        return document()->window()->starFish()->staticStrings()->m_audioLocalName;
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLAudioElement() const
    {
        return true;
    }

    void play()
    {
        STARFISH_LOG_INFO("JMP, HTMLAudioElement::play, start\n");
#ifdef STARFISH_TIZEN_WEARABLE
        player_state_e state;
        player_get_state(m_player, &state);

        if (state != PLAYER_STATE_PLAYING) {
            STARFISH_LOG_INFO("JMP, HTMLAudioElement::play, player_start\n");
            player_start(m_player);
        }
#endif
        m_isPlaying = true;
    }

    void pause()
    {
        STARFISH_LOG_INFO("JMP, HTMLAudioElement::pause, start\n");
#ifdef STARFISH_TIZEN_WEARABLE
        if (player_pause(m_player) != PLAYER_ERROR_NONE) {
            STARFISH_LOG_INFO("JMP, HTMLAudioElement::pause, error\n");
        }
#endif
        m_isPlaying = false;
    }

    bool paused() { return !m_isPlaying; }

    virtual void didAttributeChanged(QualifiedName name, String* old, String* value)
    {
        HTMLElement::didAttributeChanged(name, old, value);
        if (name == document()->window()->starFish()->staticStrings()->m_src) {
            m_src = document()->window()->starFish()->makeResourcePath(value);
#ifdef STARFISH_TIZEN_WEARABLE
            const char* path = m_src->utf8Data();
            STARFISH_LOG_INFO("JMP, path = %s", path);
            if (player_set_uri(m_player, path) != PLAYER_ERROR_NONE) {
                STARFISH_LOG_INFO("JMP, set uri error. path = %s", path);
            }

            STARFISH_LOG_INFO("JMP, m_player = %p\n", m_player);
            int prepare_error = player_prepare(m_player);
            STARFISH_LOG_INFO("JMP, prepare_error = %d\n", prepare_error);
            if (prepare_error != PLAYER_ERROR_NONE) {
                STARFISH_LOG_INFO("JMP, prepare error");
            }
#endif
        }
    }

protected:
    bool m_isPlaying;
    String* m_src;
#ifdef STARFISH_TIZEN_WEARABLE
    player_h m_player;
#endif
};
}

#endif
