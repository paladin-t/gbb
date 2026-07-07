#pragma bank 255

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "vm_device.h"
#include "vm_graphics.h"
#include "vm_scene.h"
#include "vm_scene_transfer.h"

#if USE_SCENE_TRANSFER

#warning "Message: Compiling with scene transfer."

BANKREF(VM_SCENE_TRANSFER)

// Makes blit of a region of scene data to the current scene.
BOOLEAN blit_scene(POINTER THIS, UINT8 start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)THIS; (void)start;

    const UINT8 x = (UINT8)stack_frame[3];
    const UINT8 y = (UINT8)stack_frame[2];
    const UINT8 w = (UINT8)stack_frame[1];
    const UINT8 h = (UINT8)stack_frame[0];

    if (scene.map_bank == 0)
        return TRUE;

    SCENE_LOAD(
        scene.map_bank, scene.map_address,
        scene.attr_bank, scene.attr_address,
        scene_map_x + x, scene_map_y + y,
        w, h,
        scene.width,
        scene.base_tile,
        set_bkg_submap
    );

    return TRUE;
}

// Transfers a w * h rectangle of tiles from a banked source map into VRAM at an
// arbitrary destination that may differ from the source coordinates. Handles
// both tile data and CGB attribute data.
STATIC void transfer_scene_data(
    UINT8 map_bank, const UINT8 * map,
    UINT8 attr_bank, const UINT8 * attr,
    UINT8 map_w,
    UINT8 src_x, UINT8 src_y,
    UINT8 dst_x, UINT8 dst_y,
    UINT8 w, UINT8 h,
    UINT8 base_tile,
    UINT8 * vram_addr
) {
    UINT8 buf[MAX(DEVICE_SCREEN_WIDTH, DEVICE_SCREEN_HEIGHT)];

    if (w == 1) {
        for (UINT8 i = 0; i != h; ++i)
            buf[i] = get_uint8(map_bank, map + src_x + (UINT16)(src_y + i) * map_w) + base_tile;
        set_tiles(dst_x, dst_y, 1, h, vram_addr, buf);
        if ((device_type & DEVICE_TYPE_CGB) && attr_bank) {
            VBK_REG = VBK_ATTRIBUTES;
            for (UINT8 i = 0; i != h; ++i)
                buf[i] = get_uint8(attr_bank, attr + src_x + (UINT16)(src_y + i) * map_w);
            set_tiles(dst_x, dst_y, 1, h, vram_addr, buf);
            VBK_REG = VBK_TILES;
        }
    } else {
        for (UINT8 i = 0; i != h; ++i) {
            const UINT8 * src = map + src_x + (UINT16)(src_y + i) * map_w;
            for (UINT8 c = 0; c != w; ++c) buf[c] = get_uint8(map_bank, src + c) + base_tile;
            set_tiles(dst_x, (dst_y + i) & 31, w, 1, vram_addr, buf);
        }
        if ((device_type & DEVICE_TYPE_CGB) && attr_bank) {
            VBK_REG = VBK_ATTRIBUTES;
            for (UINT8 i = 0; i != h; ++i) {
                const UINT8 * src = attr + src_x + (UINT16)(src_y + i) * map_w;
                for (UINT8 c = 0; c != w; ++c) buf[c] = get_uint8(attr_bank, src + c);
                set_tiles(dst_x, (dst_y + i) & 31, w, 1, vram_addr, buf);
            }
            VBK_REG = VBK_TILES;
        }
    }
}

#define SCENE_TRANSITION_SPEED                4
#define SCENE_TRANSITION_SECONDARY_BUFFER_X   ((DEVICE_SCREEN_BUFFER_WIDTH - DEVICE_SCREEN_WIDTH) / 2)
#define SCENE_TRANSITION_SECONDARY_BUFFER_Y   ((DEVICE_SCREEN_BUFFER_HEIGHT - DEVICE_SCREEN_HEIGHT) / 2)

INLINE void transition_init(
    UINT8 scene_w, UINT8 scene_h, UINT8 scene_y,
    UINT8 base_tile,
    UINT8 map_bank, const UINT8 * map,
    UINT8 attr_bank, const UINT8 * attr,
    UINT16 * state
) {
    UINT8 * another_vram = (UINT8 *)((LCDC_REG & LCDCF_BG9C00) ? 0x9800 : 0x9C00);

    FEATURE_MAP_MOVEMENT_CLEAR;

    *state = 0;

    transfer_scene_data(
        map_bank, map, attr_bank, attr,
        scene_w,
        0, 0, SCENE_TRANSITION_SECONDARY_BUFFER_X, SCENE_TRANSITION_SECONDARY_BUFFER_Y,
        DEVICE_SCREEN_WIDTH, scene_h,
        base_tile, another_vram
    );

    move_bkg(0, -MUL8(scene_y));
}

INLINE BOOLEAN transition_scroll(
    UINT8 scene_w, UINT8 scene_h, UINT8 scene_y,
    UINT8 dir,
    UINT8 base_tile,
    UINT8 map_bank, const UINT8 * map,
    UINT8 attr_bank, const UINT8 * attr,
    UINT16 * state
) {
    const UINT8 horizontal = IS_DIRECTION_HORIZONTAL(dir);
    const UINT8 total = horizontal ? DEVICE_SCREEN_PX_WIDTH : MUL8(scene_h);
    if (*state >= total) return TRUE;

    const UINT8 total_tiles = horizontal ? DEVICE_SCREEN_WIDTH : scene_h;
    const UINT8 offset = (UINT8)*state;
    const UINT8 new_offset = offset + SCENE_TRANSITION_SPEED;
    const UINT8 prev_tile = (UINT8)DIV8(offset);
    const UINT8 curr_tile = (UINT8)DIV8(new_offset);
    UINT8 * current_vram = (UINT8 *)((LCDC_REG & LCDCF_BG9C00) ? 0x9C00 : 0x9800);

    if ((curr_tile > prev_tile || offset == 0) && curr_tile < total_tiles) {
        if (horizontal) {
            if (dir == DIRECTION_LEFT) {
                transfer_scene_data(
                    map_bank, map, attr_bank, attr,
                    scene_w,
                    curr_tile, 0, (DEVICE_SCREEN_WIDTH + curr_tile) & 31, 0,
                    1, scene_h,
                    base_tile, current_vram
                );
            } else /* if (dir == DIRECTION_RIGHT) */ {
                transfer_scene_data(
                    map_bank, map, attr_bank, attr,
                    scene_w,
                    scene_w - 1 - curr_tile, 0, (DEVICE_SCREEN_BUFFER_WIDTH - 1 - curr_tile) & 31, 0,
                    1, scene_h,
                    base_tile, current_vram
                );
            }
        } else {
            if (dir == DIRECTION_UP) {
                transfer_scene_data(
                    map_bank, map, attr_bank, attr,
                    scene_w,
                    0, curr_tile, 0, (scene_h + curr_tile) & 31,
                    DEVICE_SCREEN_WIDTH, 1,
                    base_tile, current_vram
                );
            } else /* if (dir == DIRECTION_DOWN) */ {
                transfer_scene_data(
                    map_bank, map, attr_bank, attr,
                    scene_w,
                    0, scene_h - 1 - curr_tile, 0, (DEVICE_SCREEN_BUFFER_HEIGHT - 1 - curr_tile) & 31,
                    DEVICE_SCREEN_WIDTH, 1,
                    base_tile, current_vram
                );
            }
        }
    }

    if (horizontal) {
        if         (dir == DIRECTION_LEFT)     { scene_camera_x =         new_offset; scene_camera_y = -MUL8(scene_y);    }
        else /* if (dir == DIRECTION_RIGHT) */ { scene_camera_x = -(INT16)new_offset; scene_camera_y = -MUL8(scene_y);    }
    } else {
        if         (dir == DIRECTION_UP)       { scene_camera_x = 0; scene_camera_y =         new_offset - MUL8(scene_y); }
        else /* if (dir == DIRECTION_DOWN) */  { scene_camera_x = 0; scene_camera_y = -(INT16)new_offset - MUL8(scene_y); }
    }
    FEATURE_MAP_MOVEMENT_SET;

    *state = new_offset;

    return FALSE;
}

INLINE void transition_normalise(
    UINT8 scene_w, UINT8 scene_h, UINT8 scene_y,
    UINT8 base_tile,
    UINT8 map_bank, const UINT8 * map,
    UINT8 attr_bank, const UINT8 * attr
) {
    UINT8 * original_vram = (UINT8 *)((LCDC_REG & LCDCF_BG9C00) ? 0x9C00 : 0x9800);

    move_bkg(MUL8(SCENE_TRANSITION_SECONDARY_BUFFER_X), MUL8(SCENE_TRANSITION_SECONDARY_BUFFER_Y - scene_y));
    LCDC_REG ^= LCDCF_BG9C00;

    transfer_scene_data(
        map_bank, map, attr_bank, attr,
        scene_w,
        0, 0, 0, 0,
        DEVICE_SCREEN_WIDTH, scene_h,
        base_tile, original_vram
    );

    move_bkg(0, -MUL8(scene_y));
    LCDC_REG ^= LCDCF_BG9C00;

    scene.map_bank     = map_bank;
    scene.map_address  = (UINT8 *)map;
    scene.attr_bank    = attr_bank;
    scene.attr_address = (UINT8 *)attr;
    // scene.width     = scene_w;   // The arguments are retrieved automatically,
    // scene.height    = scene_h;   // so these few lines are not necessary.
    // scene.base_tile = base_tile;
    scene_camera_x     = 0;
    scene_camera_y     = -MUL8(scene_y);
    scene_map_x        = 0;
    scene_map_y        = 0;
    graphics_map_x     = 0;
    graphics_map_y     = 0;
    FEATURE_MAP_MOVEMENT_SET;
}

// Scrolls the currently displayed scene out of the viewport while a new scene
// scrolls in from the opposite side.
//
// The function follows the INVOKABLE convention: it is called once with
// start == TRUE to initialise, then repeatedly (start == FALSE) once per frame
// until it returns TRUE.
//
// Parameters:
//   [C] scene_w     Width of the scene in tiles, (C)onstant.
//   [D] scene_h     Height of the scene in tiles, (may be < DEVICE_SCREEN_HEIGHT),
//                   (D)etermined automatically.
//   [S] scene_y     Y offset of the scene within the viewport (tiles). Rows
//                   above/below the scene are reserved for the UI window,
//                   determined automatically and cached on (S)tack.
//   [4] dir         DIRECTION_UP/DOWN/LEFT/RIGHT. The direction the old scene
//                   scrolls out of the viewport. The new scene enters from the
//                   opposite side.
//   [D] base_tile   Base tile offset applied to every tile index, (D)etermined
//                   automatically.
//   [3] map_bank    ROM bank of the new scene tilemap.
//   [2] map_addr    Pointer to the new scene tilemap data.
//   [1] attr_bank   ROM bank of the new scene CGB attribute map (0 = none).
//   [0] attr_addr   Pointer to the new scene attribute map data.
//
// Calling rules:
//   * The old scene must already be loaded and visible (scene.map_bank != 0).
//   * The new scene should share the same tile set (tile patterns) as the old
//     scene; only the tilemap changes during the transition.
//   * Both the old and new scenes must be at least
//     DEVICE_SCREEN_WIDTH*scene_h tiles. scene_y + scene_h must not exceed
//     DEVICE_SCREEN_HEIGHT; the rows outside [scene_y, scene_y+scene_h) are
//     reserved for the UI window layer and are not touched by the transition.
//   * The function is non-blocking: it sets ctx->waitable = TRUE and returns
//     FALSE each frame until the animation completes, then returns TRUE.
//   * After completion the global `scene` struct is updated with the new
//     scene's data and the camera/scroll registers are reset to show the
//     new scene from its origin (scroll offset by -MUL8(scene_y) for the UI
//     window).
//   * Actors, triggers and other scene metadata are not loaded by this
//     function; the caller is responsible for setting those up afterwards.
//
// Algorithm overview:
//   The VRAM background tilemap is 32x32 tiles while the visible viewport is
//   20x18. The extra off-screen VRAM is used as a staging area:
//   1. Initialise (start == TRUE):
//      The hardware scroll is set to (0, -MUL8(scene_y)) so the old scene is
//      fully visible. The new scene is pre-loaded into the other VRAM bank for
//      stage 3.
//   2. Progressive scroll (each frame):
//      The hardware scroll register is advanced by `SCENE_TRANSITION_SPEED`
//      pixels per frame. Whenever a tile boundary is crossed, the column
//      (or row) of the new scene that is about to enter the viewport is loaded
//      into the off-screen VRAM area just beyond the visible edge. Because the
//      VRAM tilemap wraps at 32, this circular reuse keeps both scenes visible
//      simultaneously without needing a second buffer.
//   3. Normalise (final frame):
//      Once the full viewport distance (160px horizontal/scene_h*8px vertical)
//      has been scrolled, the visible area is reloaded from the new scene at
//      VRAM (0, 0) and the scroll is reset to (0, -MUL8(scene_y)). The `scene`
//      struct and camera variables are updated to reflect the new scene. This
//      stage uses the other VRAM bank for intermediate view.
//
//   Direction mapping (old scene exit -> new scene enter):
//     DIRECTION_LEFT : old exits left,  new enters right (scroll_x increases)
//     DIRECTION_RIGHT: old exits right, new enters left  (scroll_x decreases)
//     DIRECTION_UP   : old exits up,    new enters below (scroll_y increases)
//     DIRECTION_DOWN : old exits down,  new enters above (scroll_y decreases)
BOOLEAN transition_scene(POINTER THIS, UINT8 start, UINT16 * stack_frame) OLDCALL BANKED {
    // Prepare.
    SCRIPT_CTX * ctx = (SCRIPT_CTX *)THIS;

    const UINT8 scene_w    = DEVICE_SCREEN_WIDTH;
    const UINT8 scene_h    = scene.height;
    // const UINT8 scene_y = (UINT8)*(ctx->stack_ptr + 1); // Calculated and cached below.
    const UINT8 dir        = (UINT8)  stack_frame[4];
    const UINT8 base_tile  = scene.base_tile;
    const UINT8 map_bank   = (UINT8)  stack_frame[3];
    const UINT8 * map      = (UINT8 *)stack_frame[2];
    const UINT8 attr_bank  = (UINT8)  stack_frame[1];
    const UINT8 * attr     = (UINT8 *)stack_frame[0];

    UINT16 * state         = ctx->stack_ptr;

    if (start) {
        const UINT8 scene_y = DIV8(-scene_camera_y);
        *(ctx->stack_ptr + 1) = scene_y;

        transition_init(scene_w, scene_h, scene_y, base_tile, map_bank, map, attr_bank, attr, state);
        ctx->waitable = TRUE;

        return FALSE;
    }

    const UINT8 scene_y = (UINT8)*(ctx->stack_ptr + 1);
    if (!transition_scroll(scene_w, scene_h, scene_y, dir, base_tile, map_bank, map, attr_bank, attr, state)) {
        ctx->waitable = TRUE;

        return FALSE;
    }

    transition_normalise(scene_w, scene_h, scene_y, base_tile, map_bank, map, attr_bank, attr);

    return TRUE;
}

#endif /* USE_SCENE_TRANSFER */
