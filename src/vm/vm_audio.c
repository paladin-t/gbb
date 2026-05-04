#pragma bank 255

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "utils/sfx_player.h"
#include "utils/utils.h"

#include "vm_audio.h"

BANKREF(VM_AUDIO)

/**< Common. */

#define AUDIO_MUTE_MASK_NONE          0x00

#define AUDIO_MUSIC_HALT_BANK         0xFF

#define AUDIO_MUSIC_TICK_MASK_60HZ    0x00
#define AUDIO_MUSIC_TICK_MASK_256HZ   0x03

volatile UINT8 audio_music_playing = AUDIO_MUSIC_HALT_BANK;
static volatile UINT8 audio_mute_mask;
static const hUGESong_t * audio_music_next_song;
static UINT8 audio_music_isr_counter;
#if AUDIO_PAUSE_ENABLED
static UINT8 audio_music_isr_pause;
#endif /* AUDIO_PAUSE_ENABLED */
static volatile UINT8 audio_sfx_priority;

void audio_init(void) BANKED {
    audio_music_playing     = AUDIO_MUSIC_HALT_BANK;
    audio_mute_mask         = AUDIO_MUTE_MASK_NONE;
    audio_music_isr_counter = 0;
#if AUDIO_PAUSE_ENABLED
    audio_music_isr_pause   = FALSE;
#endif /* AUDIO_PAUSE_ENABLED */
    audio_sfx_priority      = AUDIO_SFX_PRIORITY_MINIMAL;

    // sfx_sound_init();
    sfx_reset_sample();
    sfx_sound_cut();

#if USE_SPEECH
    speech_init(FALSE);
#endif /* USE_SPEECH */
}

void audio_update(void) NONBANKED {
    if (sfx_play_bank != SFX_STOP_BANK) {
        hUGE_mute_mask = audio_mute_mask;
        if (!sfx_play_isr()) {
            hUGE_mute_mask = AUDIO_MUTE_MASK_NONE;
            hUGE_reset_wave();
#if AUDIO_FORCE_CUT_SFX
            sfx_sound_cut_mask(audio_mute_mask);
#endif /* AUDIO_FORCE_CUT_SFX */

            audio_mute_mask = AUDIO_MUTE_MASK_NONE;
            audio_sfx_priority = AUDIO_SFX_PRIORITY_MINIMAL;
            sfx_play_bank = SFX_STOP_BANK;
        }
    }

    if (audio_music_playing != AUDIO_MUSIC_HALT_BANK) {
#if AUDIO_PAUSE_ENABLED
        if (audio_music_isr_pause)
            return;
#endif /* AUDIO_PAUSE_ENABLED */

        if (++audio_music_isr_counter & AUDIO_MUSIC_TICK_MASK_256HZ)
            return;

        const UINT8 _save = CURRENT_BANK;
        SWITCH_ROM_BANK(audio_music_playing);
        if (audio_music_next_song) {
            sfx_sound_cut();
            hUGE_init(audio_music_next_song);
            audio_music_next_song = NULL;
        } else {
            hUGE_dosound();
        }
        SWITCH_ROM_BANK(_save);
    }
}

/**< Music. */

void audio_play_music(UINT8 bank, const UINT8 * music) BANKED {
    audio_music_playing = AUDIO_MUSIC_HALT_BANK;

    audio_music_next_song = (hUGESong_t *)music;
    audio_music_playing = bank;
}

#if AUDIO_PAUSE_ENABLED
void audio_pause_music(BOOLEAN pause) BANKED {
    if (audio_music_isr_pause = pause) // Assign and compare.
        sfx_sound_cut();
}
#endif /* AUDIO_PAUSE_ENABLED */

void audio_stop_music(void) BANKED {
    audio_music_playing = AUDIO_MUSIC_HALT_BANK;
    sfx_sound_cut();
}

void audio_mute_channel(UINT8 channel, UINT8 mute) BANKED {
    hUGE_mute_channel(channel, mute);
}

void audio_mute_all_channels(UINT8 mute) BANKED {
    hUGE_mute_channel(HT_CH1, mute);
    hUGE_mute_channel(HT_CH2, mute);
    hUGE_mute_channel(HT_CH3, mute);
    hUGE_mute_channel(HT_CH4, mute);
}

/**< Sound. */

void audio_play_sound(UINT8 bank, const UINT8 * ptr, UINT8 priority) BANKED {
    const UINT8 mute_mask = get_uint8(bank, ptr++);
    if (audio_sfx_priority > priority)
        return;

    sfx_play_bank = SFX_STOP_BANK;
    sfx_sound_cut_mask(audio_mute_mask);

    audio_mute_mask = mute_mask;
    audio_sfx_priority = priority;
    sfx_set_sample(bank, ptr);
}

/**< Speech. */

#if USE_SPEECH
void audio_update_speech(void) NONBANKED {
    if (speech_is_playing()) {
        hUGE_mute_mask = audio_mute_mask;
        speech_update();
        if (!speech_is_playing()) {
            hUGE_mute_mask = AUDIO_MUTE_MASK_NONE;
            hUGE_reset_wave();

            audio_mute_mask = AUDIO_MUTE_MASK_NONE;
            audio_sfx_priority = AUDIO_SFX_PRIORITY_MINIMAL;
        }
    }
}

void audio_play_speech(UINT8 bank, const UINT8 * ptr, UINT16 len) BANKED {
    if (!FEATURE_SPEECH_ISR_INSTALLED) {
        FEATURE_SPEECH_ISR_INSTALL;
        CRITICAL {
            add_VBL(audio_update_speech);
        }
    }

    sfx_play_bank = SFX_STOP_BANK;
    sfx_sound_cut_mask(audio_mute_mask);

    audio_mute_mask = SPEECH_CHANNEL_MASK;
    audio_sfx_priority = AUDIO_SFX_PRIORITY_HIGH;
    speech_play(bank, (const char *)ptr, len);
}

void audio_hush_speech(void) BANKED {
    speech_stop();

    hUGE_mute_mask = AUDIO_MUTE_MASK_NONE;
    hUGE_reset_wave();

    audio_mute_mask = AUDIO_MUTE_MASK_NONE;
    audio_sfx_priority = AUDIO_SFX_PRIORITY_MINIMAL;

    if (FEATURE_SPEECH_ISR_INSTALLED) {
        CRITICAL {
            remove_VBL(audio_update_speech);
        }
        FEATURE_SPEECH_ISR_UNINSTALL;
    }
}
#endif /* USE_SPEECH */

/**< Instructions. */

void vm_play(SCRIPT_CTX * THIS, UINT8 bank, const UINT8 * ptr) OLDCALL BANKED {
    (void)THIS;

    audio_play_music(bank, ptr);
}

void vm_stop(SCRIPT_CTX * THIS) OLDCALL BANKED {
    (void)THIS;

    audio_stop_music();
}

void vm_sound(SCRIPT_CTX * THIS, UINT8 bank, const UINT8 * ptr, UINT8 priority, UINT8 n) OLDCALL BANKED {
    if (bank) {
        audio_play_sound(bank, ptr, priority);
    } else {
        audio_play_sound(THIS->bank, (UINT8 *)THIS->PC, priority);
        THIS->PC += n;
    }
}
