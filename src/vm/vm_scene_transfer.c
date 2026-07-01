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

#define SCENE_TRANSFER_SPEED   2

// Blits a w * h rectangle of tiles from a banked source map into VRAM at an
// arbitrary destination that may differ from the source coordinates. Handles
// both tile data and CGB attribute data.
STATIC void scene_transfer_blit(
    UINT8 map_bank, const UINT8 * map, UINT8 map_w,
    UINT8 attr_bank, const UINT8 * attr,
    UINT8 src_x, UINT8 src_y,
    UINT8 dst_x, UINT8 dst_y,
    UINT8 w, UINT8 h,
    UINT8 base_tile
) NONBANKED {
    UINT8 buf[DEVICE_SCREEN_WIDTH];
    const UINT8 _save = CURRENT_BANK;
    UINT8 i;

    SWITCH_ROM_BANK(map_bank);
    if (w == 1) {
        for (i = 0; i != h; ++i)
            buf[i] = map[src_x + (UINT16)(src_y + i) * map_w] + base_tile;
        set_bkg_tiles(dst_x, dst_y, 1, h, buf);
        if ((device_type & DEVICE_TYPE_CGB) && attr_bank) {
            VBK_REG = VBK_ATTRIBUTES;
            SWITCH_ROM_BANK(attr_bank);
            for (i = 0; i != h; ++i)
                buf[i] = attr[src_x + (UINT16)(src_y + i) * map_w];
            set_bkg_tiles(dst_x, dst_y, 1, h, buf);
            VBK_REG = VBK_TILES;
        }
    } else {
        for (i = 0; i != h; ++i) {
            const UINT8 * src = map + src_x + (UINT16)(src_y + i) * map_w;
            for (UINT8 c = 0; c != w; ++c) buf[c] = src[c] + base_tile;
            set_bkg_tiles(dst_x, dst_y + i, w, 1, buf);
        }
        if ((device_type & DEVICE_TYPE_CGB) && attr_bank) {
            VBK_REG = VBK_ATTRIBUTES;
            SWITCH_ROM_BANK(attr_bank);
            for (i = 0; i != h; ++i) {
                const UINT8 * src = attr + src_x + (UINT16)(src_y + i) * map_w;
                for (UINT8 c = 0; c != w; ++c) buf[c] = src[c];
                set_bkg_tiles(dst_x, dst_y + i, w, 1, buf);
            }
            VBK_REG = VBK_TILES;
        }
    }
    SWITCH_ROM_BANK(_save);
}

// Scrolls the currently displayed scene out of the viewport while a new scene
// scrolls in from the opposite side.
//
// The function follows the INVOKABLE convention: it is called once with
// start == TRUE to initialise, then repeatedly (start == FALSE) once per frame
// until it returns TRUE.
//
// Parameters:
//   [5] dir         DIRECTION_UP/DOWN/LEFT/RIGHT
//                     The direction the old scene scrolls out of the viewport.
//                     The new scene enters from the opposite side.
//   [4] map_bank    ROM bank of the new scene tilemap.
//   [3] map_addr    Pointer to the new scene tilemap data.
//   [2] attr_bank   ROM bank of the new scene CGB attribute map (0 = none).
//   [1] attr_addr   Pointer to the new scene attribute map data.
//   [0] base_tile   Base tile offset applied to every tile index.
//
// Calling rules:
//   * The old scene must already be loaded and visible (scene.map_bank != 0).
//   * The new scene should share the same tile set (tile patterns) as the old
//     scene; only the tilemap changes during the transition.
//   * Both the old and new scenes must be at least
//     DEVICE_SCREEN_WIDTH x DEVICE_SCREEN_HEIGHT (20x18) tiles so that the full
//     viewport can be just filled.
//   * The function is non-blocking: it sets ctx->waitable = TRUE and returns
//     FALSE each frame until the animation completes, then returns TRUE.
//   * After completion the global `scene` struct is updated with the new
//     scene's data and the camera/scroll registers are reset to (0, 0).
//   * Actors, triggers and other scene metadata are not loaded by this
//     function; the caller is responsible for setting those up afterwards.
//
// Algorithm overview:
//   The VRAM background tilemap is 32x32 tiles while the visible viewport is
//   20x18. The extra 12 columns (or 14 rows) of off-screen VRAM are used as a
//   staging area:
//   1. Pre-load (start == TRUE):
//      The first 12 columns (or 14 rows) of the new scene are copied into the
//      off-screen VRAM area (columns 20-31/rows 18-31). The hardware scroll is
//      left at (0, 0) so the old scene is still fully visible.
//   2. Progressive scroll (each frame):
//      The hardware scroll register is advanced by `SCENE_TRANSFER_SPEED`
//      pixels per frame. Whenever a tile boundary is crossed, the column
//      (or row) of the old scene that just scrolled off-screen is overwritten
//      with the corresponding column (or row) of the new scene. Because the
//      VRAM tilemap wraps at 32, this circular reuse keeps both scenes visible
//      simultaneously without needing a second buffer.
//   3. Normalise (final frame):
//      Once the full viewport distance (160 px horizontal/144 px vertical) has
//      been scrolled, the visible area is reloaded from the new scene at VRAM
//      (0,0) and the scroll is reset to (0, 0). The `scene` struct and camera
//      variables are updated to reflect the new scene.
//
//   Direction mapping (old scene exit -> new scene enter):
//     DIRECTION_LEFT : old exits left,  new enters right (scroll_x increases)
//     DIRECTION_RIGHT: old exits right, new enters left  (scroll_x decreases)
//     DIRECTION_UP   : old exits up,    new enters below (scroll_y increases)
//     DIRECTION_DOWN : old exits down,  new enters above (scroll_y decreases)
BOOLEAN transition_scene(POINTER THIS, UINT8 start, UINT16 * stack_frame) OLDCALL BANKED {
    // Prepare.
    SCRIPT_CTX * ctx = (SCRIPT_CTX *)THIS;

    const UINT8 dir       = (UINT8)  stack_frame[5];
    const UINT8 map_bank  = (UINT8)  stack_frame[4];
    const UINT8 * map     = (UINT8 *)stack_frame[3];
    const UINT8 attr_bank = (UINT8)  stack_frame[2];
    const UINT8 * attr    = (UINT8 *)stack_frame[1];
    const UINT8 map_w     = DEVICE_SCREEN_WIDTH;
    const UINT8 map_h     = DEVICE_SCREEN_HEIGHT;
    const UINT8 base_tile = (UINT8)  stack_frame[0];

    UINT16 * state = ctx->stack_ptr;

    const UINT8 horizontal = IS_DIRECTION_HORIZONTAL(dir);
    const UINT16 total = horizontal ? DEVICE_SCREEN_PX_WIDTH : DEVICE_SCREEN_PX_HEIGHT;
    const UINT8 preload = horizontal ?
        (DEVICE_SCREEN_BUFFER_WIDTH - DEVICE_SCREEN_WIDTH) :
        (DEVICE_SCREEN_BUFFER_HEIGHT - DEVICE_SCREEN_HEIGHT);
    const UINT8 prog = horizontal ?
        (DEVICE_SCREEN_WIDTH - preload) :
        (DEVICE_SCREEN_HEIGHT - preload);

    // Pre-load.
    if (start) {
        FEATURE_MAP_MOVEMENT_CLEAR;
        *state = 0;

        switch (dir) {
        case DIRECTION_LEFT:
            for (UINT8 k = 0; k < preload; ++k) {
                scene_transfer_blit(
                    map_bank, map, map_w, attr_bank, attr,
                    k, 0,
                    DEVICE_SCREEN_BUFFER_WIDTH - preload + k, 0,
                    1, DEVICE_SCREEN_HEIGHT, base_tile
                );
            }

            break;
        case DIRECTION_RIGHT:
            for (UINT8 k = 0; k < preload; ++k) {
                scene_transfer_blit(
                    map_bank, map, map_w, attr_bank, attr,
                    prog + k, 0,
                    DEVICE_SCREEN_BUFFER_WIDTH - preload + k, 0,
                    1, DEVICE_SCREEN_HEIGHT, base_tile
                );
            }

            break;
        case DIRECTION_UP:
            for (UINT8 k = 0; k < preload; ++k) {
                scene_transfer_blit(
                    map_bank, map, map_w, attr_bank, attr,
                    0, k,
                    0, DEVICE_SCREEN_BUFFER_HEIGHT - preload + k,
                    DEVICE_SCREEN_WIDTH, 1, base_tile
                );
            }

            break;
        case DIRECTION_DOWN:
            for (UINT8 k = 0; k < preload; ++k) {
                scene_transfer_blit(
                    map_bank, map, map_w, attr_bank, attr,
                    0, prog + k,
                    0, DEVICE_SCREEN_BUFFER_HEIGHT - preload + k,
                    DEVICE_SCREEN_WIDTH, 1, base_tile
                );
            }

            break;
        }

        move_bkg(0, 0);

        ctx->waitable = TRUE;

        return FALSE;
    }

    // Progressive scroll.
    const UINT16 offset = *state;
    const UINT16 new_offset = offset + SCENE_TRANSFER_SPEED;
    const UINT8 prev_tile = (UINT8)(DIV8(offset));
    const UINT8 curr_tile = (UINT8)(DIV8(new_offset));

    if (curr_tile > prev_tile && curr_tile <= prog) {
        switch (dir) {
        case DIRECTION_LEFT:
            scene_transfer_blit(
                map_bank, map, map_w, attr_bank, attr,
                preload + prev_tile, 0,
                prev_tile, 0,
                1, DEVICE_SCREEN_HEIGHT, base_tile
            );

            break;
        case DIRECTION_RIGHT:
            scene_transfer_blit(
                map_bank, map, map_w, attr_bank, attr,
                prog - 1 - prev_tile, 0,
                DEVICE_SCREEN_WIDTH - 1 - prev_tile, 0,
                1, DEVICE_SCREEN_HEIGHT, base_tile
            );

            break;
        case DIRECTION_UP:
            scene_transfer_blit(
                map_bank, map, map_w, attr_bank, attr,
                0, preload + prev_tile,
                0, prev_tile,
                DEVICE_SCREEN_WIDTH, 1, base_tile
            );

            break;
        case DIRECTION_DOWN:
            scene_transfer_blit(
                map_bank, map, map_w, attr_bank, attr,
                0, prog - 1 - prev_tile,
                0, DEVICE_SCREEN_HEIGHT - 1 - prev_tile,
                DEVICE_SCREEN_WIDTH, 1, base_tile
            );

            break;
        }
    }

    if (horizontal) {
        if (dir == DIRECTION_LEFT) move_bkg((UINT8)new_offset, 0);
        else                       move_bkg((UINT8)(-new_offset), 0);
    } else {
        if (dir == DIRECTION_UP)   move_bkg(0, (UINT8)new_offset);
        else                       move_bkg(0, (UINT8)(-new_offset));
    }

    *state = new_offset;

    if (new_offset < total) {
        ctx->waitable = TRUE;

        return FALSE;
    }

    // Normalise.
    SCENE_LOAD(
        map_bank, (UINT8 *)map,
        attr_bank, (UINT8 *)attr,
        0, 0,
        DEVICE_SCREEN_WIDTH, DEVICE_SCREEN_HEIGHT,
        map_w,
        base_tile,
        set_bkg_submap
    );
    move_bkg(0, 0);

    scene.map_bank     = map_bank;
    scene.map_address  = (UINT8 *)map;
    scene.attr_bank    = attr_bank;
    scene.attr_address = (UINT8 *)attr;
    scene.width        = map_w;
    scene.height       = map_h;
    scene.base_tile    = base_tile;

    scene_camera_x     = 0;
    scene_camera_y     = 0;
    scene_map_x        = 0;
    scene_map_y        = 0;
    graphics_map_x     = 0;
    graphics_map_y     = 0;

    // Finish.
    return TRUE;
}

#endif /* USE_SCENE_TRANSFER */
