/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __INLINE_CODE_H__
#define __INLINE_CODE_H__

/*
** {===========================================================================
** Inline code
*/

#ifndef RES_CODE_PLAY_MUSIC
#	define RES_CODE_PLAY_MUSIC \
		"sound on\n" \
		"play #0\n"
#endif /* RES_CODE_PLAY_MUSIC */
#ifndef RES_CODE_SET_MUSIC_POSITION
#	define RES_CODE_SET_MUSIC_POSITION(P) \
		"start set_pos\n" \
		"end\n" \
		"set_pos:\n" \
		"  wait 3\n" /* A few ticks later. */ \
		"  option MUSIC_POSITION, " + (P) + "\n"
#endif /* RES_CODE_SET_MUSIC_POSITION */

#ifndef RES_CODE_PLAY_SFX
#	define RES_CODE_PLAY_SFX \
		"sound on\n" \
		"sound #0\n"
#endif /* RES_CODE_PLAY_SFX */

/* ===========================================================================} */

#endif /* __INLINE_CODE_H__ */
