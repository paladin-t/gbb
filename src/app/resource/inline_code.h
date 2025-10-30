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
#	define RES_CODE_PLAY_MAP_TESTING \
		"auto update on\n" \
		"map on\n" \
		"let t = get tile len(#0)\n" \
		"let w = get map width(#0)\n" \
		"let h = get map height(#0)\n" \
		"let mx = (w - 20) * 8\n" \
		"let my = (h - 18) * 8\n" \
		"let s = 2\n" \
		"let cx = iif(w >= 20, 0, (w - 20) * 4)\n" \
		"let cy = iif(h >= 18, 0, (h - 18) * 4)\n" \
		"fill tile(0, t) = #0\n" \
		"def scene(w, h, 0) = #0\n" \
		"camera cx, cy\n" \
		"\n" \
		"if w > 20 then\n" \
		"  on btn(LEFT_BTN) start on_left\n" \
		"  on btn(RIGHT_BTN) start on_right\n" \
		"end if\n" \
		"if h > 18 then\n" \
		"  on btn(UP_BTN) start on_up\n" \
		"  on btn(DOWN_BTN) start on_down\n" \
		"end if\n" \
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
		"auto update on\n" \
		"map on\n" \
		"sprite on\n" \
		"let w = get map width(#0)\n" \
		"let h = get map height(#0)\n" \
		"let mx = (w - 20) * 8\n" \
		"let my = (h - 18) * 8\n" \
		"let s = 2\n" \
		"let cx = iif(w >= 20, 0, (w - 20) * 4)\n" \
		"let cy = iif(h >= 18, 0, (h - 18) * 4)\n" \
		"load scene(0, 0) = #0\n" \
		"if not {0} then camera cx, cy\n" \
		"\n" \
		"if not {0} then\n" \
		"  if w > 20 then\n" \
		"    on btn(LEFT_BTN) start on_left\n" \
		"    on btn(RIGHT_BTN) start on_right\n" \
		"  end if\n" \
		"  if h > 18 then\n" \
		"    on btn(UP_BTN) start on_up\n" \
		"    on btn(DOWN_BTN) start on_down\n" \
		"  end if\n" \
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
#	define RES_CODE_PLAY_ACTOR_TESTING \
		"def DOWN_WALK  = 4\n" \
		"def RIGHT_WALK = 5\n" \
		"def UP_WALK    = 6\n" \
		"def LEFT_WALK  = 7\n" \
		"def TO_IDLE    = 16\n" \
		"let DIR_BTNS   = LEFT_BTN bor RIGHT_BTN bor UP_BTN bor DOWN_BTN\n" \
		"\n" \
		"auto update on\n" \
		"sprite on\n" \
		"if {3} then\n" \
		"  option SPRITE8x16_ENABLED, true\n" \
		"end if\n" \
		"let ax\n" \
		"let ay\n" \
		"fill actor(0, {2}) = #0\n" \
		"let a = new actor()\n" \
		"def actor(a, {0}, {1}, 0) = #0\n" \
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
		"  lock\n" \
		"    play actor a, LEFT_WALK\n" \
		"    ax = get actor property(a, POSITION_X_PROP)\n" \
		"    if ax > 1 then\n" \
		"      move actor(a, 1) with -1, 0\n" \
		"    end if\n" \
		"  unlock\n" \
		"  end\n" \
		"on_right:\n" \
		"  lock\n" \
		"    play actor a, RIGHT_WALK\n" \
		"    ax = get actor property(a, POSITION_X_PROP)\n" \
		"    if ax < 159 then\n" \
		"      move actor(a, 1) with 1, 0\n" \
		"    end if\n" \
		"  unlock\n" \
		"  end\n" \
		"on_up:\n" \
		"  lock\n" \
		"    play actor a, UP_WALK\n" \
		"    ay = get actor property(a, POSITION_Y_PROP)\n" \
		"    if ay > 1 then\n" \
		"      move actor(a, 1) with 0, -1\n" \
		"    end if\n" \
		"  unlock\n" \
		"  end\n" \
		"on_down:\n" \
		"  lock\n" \
		"    play actor a, DOWN_WALK\n" \
		"    ay = get actor property(a, POSITION_Y_PROP)\n" \
		"    if ay < 143 then\n" \
		"      move actor(a, 1) with 0, 1\n" \
		"    end if\n" \
		"  unlock\n" \
		"  end\n" \
		"on_btn_up:\n" \
		"  lock\n" \
		"    if not btn(DIR_BTNS) then\n" \
		"      play actor a, TO_IDLE\n" \
		"    end if\n" \
		"  unlock\n" \
		"  end\n"
#endif /* RES_CODE_PLAY_ACTOR_TESTING */

#ifndef RES_CODE_PLAY_MUSIC
#	define RES_CODE_PLAY_MUSIC \
		"sound on\n" \
		"play #0\n"
#endif /* RES_CODE_PLAY_MUSIC */
#ifndef RES_CODE_SET_MUSIC_POSITION
#	define RES_CODE_SET_MUSIC_POSITION \
		"start set_pos\n" \
		"end\n" \
		"set_pos:\n" \
		"  wait 3\n" /* A few ticks later. */ \
		"  option MUSIC_POSITION, {0}\n"
#endif /* RES_CODE_SET_MUSIC_POSITION */

#ifndef RES_CODE_PLAY_SFX
#	define RES_CODE_PLAY_SFX \
		"sound on\n" \
		"sound #0\n"
#endif /* RES_CODE_PLAY_SFX */

/* ===========================================================================} */

#endif /* __INLINE_CODE_H__ */
