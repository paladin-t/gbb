/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __EDITOR_SFX_H__
#define __EDITOR_SFX_H__

#include "editor.h"

/*
** {===========================================================================
** Macros and constants
*/

// Hard coded. Associated with "sfx_player.c".
#ifndef EDITOR_SFX_PLAYER_IS_PLAYING_RAM_STUB
#	define EDITOR_SFX_PLAYER_IS_PLAYING_RAM_STUB "sfx_play_bank"
#endif /* EDITOR_SFX_PLAYER_IS_PLAYING_RAM_STUB */
// Hard coded. Associated with "sfx_player.c".
#ifndef EDITOR_SFX_PLAYER_IS_PLAYING_RAM_OFFSET
#	define EDITOR_SFX_PLAYER_IS_PLAYING_RAM_OFFSET 0
#endif /* EDITOR_SFX_PLAYER_IS_PLAYING_RAM_OFFSET */
// Hard coded. Associated with "sfx_player.c".
#ifndef EDITOR_SFX_HALT_BANK
#	define EDITOR_SFX_HALT_BANK 0xff
#endif /* EDITOR_SFX_HALT_BANK */

#ifndef EDITOR_SFX_SHOW_SOUND_SHAPE_ENABLED
#	define EDITOR_SFX_SHOW_SOUND_SHAPE_ENABLED 1
#endif /* EDITOR_SFX_SHOW_SOUND_SHAPE_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Sfx editor
*/

class EditorSfx : public Editor, public virtual Object {
private:
	static EditorSfx* instance;
	static int refCount;

public:
	GBBASIC_CLASS_TYPE('S', 'F', 'X', 'E')

	static EditorSfx* create(class Window* wnd, class Renderer* rnd, class Workspace* ws, class Project* prj);
	static EditorSfx* destroy(EditorSfx* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_SFX_H__ */
