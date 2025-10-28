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

#ifndef RES_CODE_PLAY_MAP_TESTING
#	define RES_CODE_PLAY_MAP_TESTING(T, W, H) \
		"auto update on\n" \
		"map on\n" \
		"let cx = 0\n" \
		"let cy = 0\n" \
		"let s = 2\n" \
		"let w = " + (W) + "\n" \
		"let h = " + (H) + "\n" \
		"let mx = (w - 20) * 8\n" \
		"let my = (h - 18) * 8\n" \
		"fill tile(0, " + (T) + ") = #0\n" \
		"def scene(w, h, 0) = #0\n" \
		"\n" \
		"on btn(LEFT_BTN) start on_left\n" \
		"on btn(RIGHT_BTN) start on_right\n" \
		"on btn(UP_BTN) start on_up\n" \
		"on btn(DOWN_BTN) start on_down\n" \
		"on btnu(START_BTN) start on_start\n" \
		"end\n" \
		"\n" \
		"on_left:\n" \
		"  cx = cx - s\n" \
		"  cx = max(cx, 0)\n" \
		"  camera cx, cy\n" \
		"  end\n" \
		"on_right:\n" \
		"  cx = cx + s\n" \
		"  cx = min(cx, mx)\n" \
		"  camera cx, cy\n" \
		"  end\n" \
		"on_up:\n" \
		"  cy = cy - s\n" \
		"  cy = max(cy, 0)\n" \
		"  camera cx, cy\n" \
		"  end\n" \
		"on_down:\n" \
		"  cy = cy + s\n" \
		"  cy = min(cy, my)\n" \
		"  camera cx, cy\n" \
		"  end\n" \
		"on_start:\n" \
		"  cx = 0\n" \
		"  cy = 0\n" \
		"  camera cx, cy\n" \
		"  end\n"
#endif /* RES_CODE_PLAY_MAP_TESTING */

#ifndef RES_CODE_PLAY_SCENE_TESTING
#	define RES_CODE_PLAY_SCENE_TESTING \
		"map on"
#endif /* RES_CODE_PLAY_SCENE_TESTING */

#ifndef RES_CODE_PLAY_ACTOR_TESTING
#	define RES_CODE_PLAY_ACTOR_TESTING \
		"sprite on"
#endif /* RES_CODE_PLAY_ACTOR_TESTING */

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
