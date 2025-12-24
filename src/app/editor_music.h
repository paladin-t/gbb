/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __EDITOR_MUSIC_H__
#define __EDITOR_MUSIC_H__

#include "editor.h"

/*
** {===========================================================================
** Macros and constants
*/

// Hard coded. Associated with "vm_audio.c".
#ifndef EDITOR_MUSIC_PLAYER_IS_PLAYING_RAM_STUB
#	define EDITOR_MUSIC_PLAYER_IS_PLAYING_RAM_STUB "audio_music_playing"
#endif /* EDITOR_MUSIC_PLAYER_IS_PLAYING_RAM_STUB */
// Hard coded. Associated with "vm_audio.c".
#ifndef EDITOR_MUSIC_PLAYER_IS_PLAYING_RAM_OFFSET
#	define EDITOR_MUSIC_PLAYER_IS_PLAYING_RAM_OFFSET 0
#endif /* EDITOR_MUSIC_PLAYER_IS_PLAYING_RAM_OFFSET */
// Hard coded. Associated with "vm_audio.c".
#ifndef EDITOR_MUSIC_HALT_BANK
#	define EDITOR_MUSIC_HALT_BANK 0xff
#endif /* EDITOR_MUSIC_HALT_BANK */

// Hard coded. Associated with "hUGEDriver.asm".
#ifndef EDITOR_MUSIC_PLAYER_ORDER_CURSOR_RAM_STUB
#	define EDITOR_MUSIC_PLAYER_ORDER_CURSOR_RAM_STUB "hUGE_mute_mask"
#endif /* EDITOR_MUSIC_PLAYER_ORDER_CURSOR_RAM_STUB */
// Hard coded. Associated with "hUGEDriver.asm".
#ifndef EDITOR_MUSIC_PLAYER_ORDER_CURSOR_RAM_OFFSET
#	define EDITOR_MUSIC_PLAYER_ORDER_CURSOR_RAM_OFFSET 1
#endif /* EDITOR_MUSIC_PLAYER_ORDER_CURSOR_RAM_OFFSET */
// Hard coded. Associated with "hUGEDriver.asm".
#ifndef EDITOR_MUSIC_PLAYER_LINE_CURSOR_RAM_STUB
#	define EDITOR_MUSIC_PLAYER_LINE_CURSOR_RAM_STUB "hUGE_mute_mask"
#endif /* EDITOR_MUSIC_PLAYER_LINE_CURSOR_RAM_STUB */
// Hard coded. Associated with "hUGEDriver.asm".
#ifndef EDITOR_MUSIC_PLAYER_LINE_CURSOR_RAM_OFFSET
#	define EDITOR_MUSIC_PLAYER_LINE_CURSOR_RAM_OFFSET 6
#endif /* EDITOR_MUSIC_PLAYER_LINE_CURSOR_RAM_OFFSET */
// Hard coded. Associated with "hUGEDriver.asm".
#ifndef EDITOR_MUSIC_PLAYER_TICKS_RAM_STUB
#	define EDITOR_MUSIC_PLAYER_TICKS_RAM_STUB "hUGE_mute_mask"
#endif /* EDITOR_MUSIC_PLAYER_TICKS_RAM_STUB */
// Hard coded. Associated with "hUGEDriver.asm".
#ifndef EDITOR_MUSIC_PLAYER_TICKS_RAM_OFFSET
#	define EDITOR_MUSIC_PLAYER_TICKS_RAM_OFFSET 7
#endif /* EDITOR_MUSIC_PLAYER_TICKS_RAM_OFFSET */

/* ===========================================================================} */

/*
** {===========================================================================
** Music editor
*/

class EditorMusic : public Editor, public virtual Object {
public:
	GBBASIC_CLASS_TYPE('M', 'U', 'S', 'I')

	static EditorMusic* create(void);
	static void destroy(EditorMusic* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_MUSIC_H__ */
