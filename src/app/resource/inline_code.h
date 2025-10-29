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
#	define RES_CODE_PLAY_MAP_TESTING(W, H, T) \
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
#	define RES_CODE_PLAY_SCENE_TESTING(W, H, P) \
		"auto update on\n" \
		"map on\n" \
		"sprite on\n" \
		"let cx = 0\n" \
		"let cy = 0\n" \
		"let s = 2\n" \
		"let w = " + (W) + "\n" \
		"let h = " + (H) + "\n" \
		"let mx = (w - 20) * 8\n" \
		"let my = (h - 18) * 8\n" \
		"load scene(0, 0) = #0\n" \
		"\n" \
		"if " + (P) + " then\n" \
		"  on btn(LEFT_BTN) start on_left\n" \
		"  on btn(RIGHT_BTN) start on_right\n" \
		"  on btn(UP_BTN) start on_up\n" \
		"  on btn(DOWN_BTN) start on_down\n" \
		"  on btnu(START_BTN) start on_start\n" \
		"end if\n" \
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
#endif /* RES_CODE_PLAY_SCENE_TESTING */

#ifndef RES_CODE_PLAY_ACTOR_TESTING
#	define RES_CODE_PLAY_ACTOR_TESTING(X, Y, T, E) \
		"def DOWN_WALK  = 4\n" \
		"def RIGHT_WALK = 5\n" \
		"def UP_WALK    = 6\n" \
		"def LEFT_WALK  = 7\n" \
		"def TO_IDLE    = 16\n" \
		"let DIR_BTNS   = LEFT_BTN bor RIGHT_BTN bor UP_BTN bor DOWN_BTN\n" \
		"\n" \
		"auto update on\n" \
		"sprite on\n" \
		"if " + (E) + " then\n" \
		"  option SPRITE8x16_ENABLED, true\n" \
		"end if\n" \
		"fill actor(0, " + (T) + ") = #0\n" \
		"let a = new actor()\n" \
		"def actor(a, " + (X) + ", " + (Y) + ", 0) = #0\n" \
		"control actor a, NONE_BEHAVIOUR\n" \
		"play actor a, 0\n" \
		"\n" \
		"on btn(LEFT_BTN) start on_left\n" \
		"on btn(RIGHT_BTN) start on_right\n" \
		"on btn(UP_BTN) start on_up\n" \
		"on btn(DOWN_BTN) start on_down\n" \
		"on btnu(LEFT_BTN) start on_btn_up\n" \
		"on btnu(RIGHT_BTN) start on_btn_up\n" \
		"on btnu(UP_BTN) start on_btn_up\n" \
		"on btnu(DOWN_BTN) start on_btn_up\n" \
		"end\n" \
		"\n" \
		"on_left:\n" \
		"  play actor a, LEFT_WALK\n" \
		"  move actor(a, 1) with -1, 0\n" \
		"  end\n" \
		"on_right:\n" \
		"  play actor a, RIGHT_WALK\n" \
		"  move actor(a, 1) with 1, 0\n" \
		"  end\n" \
		"on_up:\n" \
		"  play actor a, UP_WALK\n" \
		"  move actor(a, 1) with 0, -1\n" \
		"  end\n" \
		"on_down:\n" \
		"  play actor a, DOWN_WALK\n" \
		"  move actor(a, 1) with 0, 1\n" \
		"  end\n" \
		"on_btn_up:\n" \
		"  if not btn(DIR_BTNS) then\n" \
		"    play actor a, TO_IDLE\n" \
		"  end if\n" \
		"  end\n"
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
