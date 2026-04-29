#ifndef __VM_AUDIO_H__
#define __VM_AUDIO_H__

#include "drv/hUGE/player-gbdk/hUGEDriver.h"

#include "vm.h"

BANKREF_EXTERN(VM_AUDIO)

/**< Common. */

#define AUDIO_SFX_PRIORITY_MINIMAL   1
#define AUDIO_SFX_PRIORITY_NORMAL    4
#define AUDIO_SFX_PRIORITY_HIGH      8

#define AUDIO_FORCE_CUT_SFX          0
#define AUDIO_PAUSE_ENABLED          0

void audio_init(void) BANKED;
void audio_update(void) NONBANKED; // UPDATE.

/**< Music. */

void audio_play_music(UINT8 bank, const UINT8 * music) BANKED;
#if AUDIO_PAUSE_ENABLED
void audio_pause_music(BOOLEAN pause) BANKED;
#endif /* AUDIO_PAUSE_ENABLED */
void audio_stop_music(void) BANKED;
void audio_mute_channel(UINT8 channel, UINT8 mute) BANKED;
void audio_mute_all_channels(UINT8 mute) BANKED;

/**< Sound. */

void audio_play_sound(UINT8 bank, const UINT8 * ptr, UINT8 priority) BANKED;

/**< Speech. */

#if defined USE_SPEECH
void audio_update_speech(void) NONBANKED; // UPDATE.
void audio_play_speech(UINT8 bank, const UINT8 * ptr, UINT16 len) BANKED;
void audio_hush_speech(void) BANKED;
#endif /* USE_SPEECH */

/**< Instructions. */

void vm_play(SCRIPT_CTX * THIS, UINT8 bank, const UINT8 * ptr) OLDCALL BANKED;
void vm_stop(SCRIPT_CTX * THIS) OLDCALL BANKED;
void vm_sound(SCRIPT_CTX * THIS, UINT8 bank, const UINT8 * ptr, UINT8 priority, UINT8 n) OLDCALL BANKED;

#endif /* __VM_AUDIO_H__ */
