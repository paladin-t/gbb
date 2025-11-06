#pragma bank 255

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "utils/graphics.h"
#include "utils/sgb.h"
#include "utils/text.h"
#include "utils/utils.h"

#include "vm_device.h"
#include "vm_graphics.h"
#include "vm_gui.h"
#include "vm_scene.h"

BANKREF(VM_GRAPHICS)

// Common.

#define GRAPHICS_AUTO_OFFSET   0x7FFF // Max value of 16-bit signed, 32767.

#define GRAPHICS_DEF(BANK, PTR, X, Y, W, H, SW, BASE_TILE, FUNC) \
    do { \
        _submap_tile_offset = (BASE_TILE); \
        call_v_bbbbpb_oldcall((X), (Y), (W), (H), (BANK), (PTR), (SW), (FUNC)); \
        _submap_tile_offset = 0; \
    } while (0)

INT16 graphics_map_x;
INT16 graphics_map_y;

void graphics_init(void) BANKED {
    graphics_map_x = 0;
    graphics_map_y = 0;
}

void graphics_put_map(void) BANKED {
    const INT16 x = scene_camera_x - graphics_map_x + scene_camera_shake_x;
    const INT16 y = scene_camera_y - graphics_map_y + scene_camera_shake_y;
    move_bkg((UINT8)x, (UINT8)y);
}

// Graphics.

void vm_color(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 forecolor = clamp_uint8((UINT8)*(--THIS->stack_ptr), WHITE, BLACK);
    const UINT8 backcolor = clamp_uint8((UINT8)*(--THIS->stack_ptr), WHITE, BLACK);
    const UINT8 mode      = clamp_uint8((UINT8)*(--THIS->stack_ptr), SOLID, AND);
    color(forecolor, backcolor, mode);
}

void vm_palette(SCRIPT_CTX * THIS, UINT8 nsargs) OLDCALL BANKED {
    switch (nsargs) {
    case 2: { // For GB.
            const UINT8 layer = (UINT8)*(--THIS->stack_ptr);
            const UINT8 val   = (UINT8)*(--THIS->stack_ptr);
            if (layer == GRAPHICS_LAYER_MAP || layer == GRAPHICS_LAYER_WINDOW)
                BGP_REG       = val;
            else /* if (layer == GRAPHICS_LAYER_SPRITE) */
                OBP0_REG      = val;
        }

        break;
    case 3: { // For GB.
            const UINT8 layer = (UINT8)*(--THIS->stack_ptr);
            const UINT8 val   = (UINT8)*(--THIS->stack_ptr);
            const UINT8 idx   = (UINT8)*(--THIS->stack_ptr);
            if (layer == GRAPHICS_LAYER_MAP || layer == GRAPHICS_LAYER_WINDOW) {
                BGP_REG       = val;
            } else /* if (layer == GRAPHICS_LAYER_SPRITE) */ {
                if (idx == 0)
                    OBP0_REG  = val;
                else /* if (idx == 1) */
                    OBP1_REG  = val;
            }
        }

        break;
    case 4: { // For CGB.
            const UINT8 layer = (UINT8)*(--THIS->stack_ptr);
            const UINT8 plt   = (UINT8)*(--THIS->stack_ptr);
            const UINT8 entry = (UINT8)*(--THIS->stack_ptr);
            const UINT16 val  = (UINT16)*(--THIS->stack_ptr);
            if (device_type & DEVICE_TYPE_CGB) {
                if (layer == GRAPHICS_LAYER_MAP || layer == GRAPHICS_LAYER_WINDOW)
                    set_bkg_palette_entry(plt, entry, val);
                else /* if (layer == GRAPHICS_LAYER_SPRITE) */
                    set_sprite_palette_entry(plt, entry, val);
            }
        }

        break;
    case 8: { // For SGB.
            sgb_palette_packet_t pck;
            pck.command       = (UINT8)*(--THIS->stack_ptr);
            pck.colors[0]     = (UINT16)*(--THIS->stack_ptr);
            pck.colors[1]     = (UINT16)*(--THIS->stack_ptr);
            pck.colors[2]     = (UINT16)*(--THIS->stack_ptr);
            pck.colors[3]     = (UINT16)*(--THIS->stack_ptr);
            pck.colors[4]     = (UINT16)*(--THIS->stack_ptr);
            pck.colors[5]     = (UINT16)*(--THIS->stack_ptr);
            pck.colors[6]     = (UINT16)*(--THIS->stack_ptr);
            sgb_transfer((POINTER)&pck);
        }

        break;
    }
}

void vm_rgb(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 r    = (UINT8)*(--THIS->stack_ptr);
    const UINT8 g    = (UINT8)*(--THIS->stack_ptr);
    const UINT8 b    = (UINT8)*(--THIS->stack_ptr);
    const UINT16 ret = RGB8(r, g, b);
    *(THIS->stack_ptr++) = ret;
}

void vm_plot(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 x = clamp_uint8((UINT8)*(--THIS->stack_ptr), 0, (UINT8)(GRAPHICS_WIDTH  - 1));
    const UINT8 y = clamp_uint8((UINT8)*(--THIS->stack_ptr), 0, (UINT8)(GRAPHICS_HEIGHT - 1));
    plot_point(x, y);
}

void vm_point(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 x   = clamp_uint8((UINT8)*(--THIS->stack_ptr), 0, (UINT8)(GRAPHICS_WIDTH  - 1));
    const UINT8 y   = clamp_uint8((UINT8)*(--THIS->stack_ptr), 0, (UINT8)(GRAPHICS_HEIGHT - 1));
    const UINT8 ret = getpix(x, y);
    *(THIS->stack_ptr++) = ret;
}

void vm_line(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 x0 = clamp_uint8((UINT8)*(--THIS->stack_ptr), 0, (UINT8)(GRAPHICS_WIDTH  - 1));
    const UINT8 y0 = clamp_uint8((UINT8)*(--THIS->stack_ptr), 0, (UINT8)(GRAPHICS_HEIGHT - 1));
    const UINT8 x1 = clamp_uint8((UINT8)*(--THIS->stack_ptr), 0, (UINT8)(GRAPHICS_WIDTH  - 1));
    const UINT8 y1 = clamp_uint8((UINT8)*(--THIS->stack_ptr), 0, (UINT8)(GRAPHICS_HEIGHT - 1));
    line(x0, y0, x1, y1);
}

void vm_rect(SCRIPT_CTX * THIS, BOOLEAN fill) OLDCALL BANKED {
    const UINT8 x0 = clamp_uint8((UINT8)*(--THIS->stack_ptr), 0, (UINT8)(GRAPHICS_WIDTH  - 1));
    const UINT8 y0 = clamp_uint8((UINT8)*(--THIS->stack_ptr), 0, (UINT8)(GRAPHICS_HEIGHT - 1));
    const UINT8 x1 = clamp_uint8((UINT8)*(--THIS->stack_ptr), 0, (UINT8)(GRAPHICS_WIDTH  - 1));
    const UINT8 y1 = clamp_uint8((UINT8)*(--THIS->stack_ptr), 0, (UINT8)(GRAPHICS_HEIGHT - 1));
    box(x0, y0, x1, y1, fill ? M_FILL : M_NOFILL);
}

void vm_gotoxy(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 x = clamp_uint8((UINT8)*(--THIS->stack_ptr), 0, (UINT8)(DEVICE_SCREEN_WIDTH - 1));
    const UINT8 y = clamp_uint8((UINT8)*(--THIS->stack_ptr), 0, (UINT8)(DEVICE_SCREEN_HEIGHT - 1));
    gotogxy(x, y);
}

void vm_text(SCRIPT_CTX * THIS, BOOLEAN new_line, UINT8 nargs) OLDCALL BANKED {
    print(THIS, new_line, nargs, TEXT_PRINT_MODE_GRAPHICS);
}

void vm_image(SCRIPT_CTX * THIS, UINT8 src) OLDCALL BANKED {
    const UINT8 first = (UINT8)*(--THIS->stack_ptr);
    const UINT8 x     = (UINT8)*(--THIS->stack_ptr);
    const UINT8 y     = (UINT8)*(--THIS->stack_ptr);
    const UINT8 w     = (UINT8)*(--THIS->stack_ptr);
    const UINT8 h     = (UINT8)*(--THIS->stack_ptr);
    const UINT8 layer = (UINT8)*(--THIS->stack_ptr);
    const UINT8 n     = w * h;
    UINT8 bank;
    UINT8 * ptr;
    v_bbp_fn_oldcall func;

    switch (src) {
    case ASSET_SOURCE_READ:
        bank = current_data_bank;
        ptr  = (UINT8 *)current_data_address;
        current_data_address += MAX(n, 1) * 16;

        break;
    case ASSET_SOURCE_DATA:
        bank = THIS->bank;
        ptr  = (UINT8 *)THIS->PC;
        THIS->PC += MAX(n, 1) * 16;

        break;
    case ASSET_SOURCE_FAR:
        bank = get_uint8 (THIS->bank, (UINT8 *)THIS->PC++);
        ptr  = get_ptr_be(THIS->bank, (UINT8 *)THIS->PC  ); THIS->PC += 2;

        break;
    default:
        bank = 0;
        ptr  = NULL;

        break;
    }

    switch (layer) {
    case GRAPHICS_LAYER_MAP:      func = set_bkg_data; break;
    case GRAPHICS_LAYER_WINDOW:   func = set_win_data; break;
    default:                      func = NULL;         break;
    }

    call_v_bbp_oldcall(first, n, bank, ptr, func);
    UINT8 k = first;
    for (UINT8 j = 0; j != h; ++j) {
        for (UINT8 i = 0; i != w; ++i) {
            set_bkg_tile_xy(x + i, y + j, k++);
        }
    }
}

void vm_fill_tile(SCRIPT_CTX * THIS, UINT8 src, UINT8 layer) OLDCALL BANKED {
    const UINT8 first = (UINT8)*(--THIS->stack_ptr);
    const UINT8 n     = (UINT8)*(--THIS->stack_ptr);
    UINT8 bank;
    UINT8 * ptr;
    v_bbp_fn_oldcall func;

    switch (src) {
    case ASSET_SOURCE_READ:
        bank = current_data_bank;
        ptr  = (UINT8 *)current_data_address;
        current_data_address += MAX(n, 1) * 16;

        break;
    case ASSET_SOURCE_DATA:
        bank = THIS->bank;
        ptr  = (UINT8 *)THIS->PC;
        THIS->PC += MAX(n, 1) * 16;

        break;
    case ASSET_SOURCE_FAR:
        bank = get_uint8 (THIS->bank, (UINT8 *)THIS->PC++);
        ptr  = get_ptr_be(THIS->bank, (UINT8 *)THIS->PC  ); THIS->PC += 2;

        break;
    default:
        bank = 0;
        ptr  = NULL;

        break;
    }

    switch (layer) {
    case GRAPHICS_LAYER_MAP:      func = set_bkg_data;    break;
    case GRAPHICS_LAYER_WINDOW:   func = set_win_data;    break;
    case GRAPHICS_LAYER_SPRITE:   func = set_sprite_data; break;
    default:                      func = NULL;            break;
    }

    call_v_bbp_oldcall(first, n, bank, ptr, func);
    gui_tile_filled(layer, first, n, bank, ptr);
}

void vm_def_map(SCRIPT_CTX * THIS, UINT8 src) OLDCALL BANKED {
    const UINT8 x    = (UINT8)*(--THIS->stack_ptr);
    const UINT8 y    = (UINT8)*(--THIS->stack_ptr);
    const UINT8 w    = (UINT8)*(--THIS->stack_ptr);
    const UINT8 h    = (UINT8)*(--THIS->stack_ptr);
    const UINT8 sw   = (UINT8)*(--THIS->stack_ptr);
    const UINT8 base = (UINT8)*(--THIS->stack_ptr);
    INT16 offset     = (INT16)*(--THIS->stack_ptr);
    UINT8 bank;
    UINT8 * ptr;

    switch (src) {
    case ASSET_SOURCE_READ:
        bank = current_data_bank;
        ptr  = (UINT8 *)current_data_address;
        current_data_address += w * h;

        break;
    case ASSET_SOURCE_DATA:
        bank = THIS->bank;
        ptr  = (UINT8 *)THIS->PC;
        THIS->PC += w * h;

        break;
    case ASSET_SOURCE_FAR:
        bank = get_uint8 (THIS->bank, (UINT8 *)THIS->PC++);
        ptr  = get_ptr_be(THIS->bank, (UINT8 *)THIS->PC  ); THIS->PC += 2;

        break;
    default:
        bank = 0;
        ptr  = NULL;

        break;
    }
    if (offset == GRAPHICS_AUTO_OFFSET) offset = -(x + y * sw);

    if (FEATURE_MAP_MOVEMENT_FLAG) {
        FEATURE_MAP_MOVEMENT_CLEAR;
        graphics_put_map();
    }

    FEATURE_SCENE_DISABLE;
    if (offset != 0) ptr += offset;
    GRAPHICS_DEF(bank, ptr, x, y, w, h, sw, base, set_bkg_submap);
}

void vm_map(SCRIPT_CTX * THIS) OLDCALL BANKED {
    graphics_map_x = (INT16)*(--THIS->stack_ptr);
    graphics_map_y = (INT16)*(--THIS->stack_ptr);
    FEATURE_MAP_MOVEMENT_SET;
}

void vm_mget(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 x   = (UINT8)*(--THIS->stack_ptr);
    const UINT8 y   = (UINT8)*(--THIS->stack_ptr);
    const UINT8 ret = get_bkg_tile_xy(x, y);
    *(THIS->stack_ptr++) = ret;
}

void vm_mset(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 x = (UINT8)*(--THIS->stack_ptr);
    const UINT8 y = (UINT8)*(--THIS->stack_ptr);
    const UINT8 t = (UINT8)*(--THIS->stack_ptr);
    set_bkg_tile_xy(x, y, t);
}

void vm_def_window(SCRIPT_CTX * THIS, UINT8 src) OLDCALL BANKED {
    const UINT8 x    = (UINT8)*(--THIS->stack_ptr);
    const UINT8 y    = (UINT8)*(--THIS->stack_ptr);
    const UINT8 w    = (UINT8)*(--THIS->stack_ptr);
    const UINT8 h    = (UINT8)*(--THIS->stack_ptr);
    const UINT8 sw   = (UINT8)*(--THIS->stack_ptr);
    const UINT8 base = (UINT8)*(--THIS->stack_ptr);
    INT16 offset     = (INT16)*(--THIS->stack_ptr);
    UINT8 bank;
    UINT8 * ptr;

    switch (src) {
    case ASSET_SOURCE_READ:
        bank = current_data_bank;
        ptr  = (UINT8 *)current_data_address;
        current_data_address += w * h;

        break;
    case ASSET_SOURCE_DATA:
        bank = THIS->bank;
        ptr  = (UINT8 *)THIS->PC;
        THIS->PC += w * h;

        break;
    case ASSET_SOURCE_FAR:
        bank = get_uint8 (THIS->bank, (UINT8 *)THIS->PC++);
        ptr  = get_ptr_be(THIS->bank, (UINT8 *)THIS->PC  ); THIS->PC += 2;

        break;
    default:
        bank = 0;
        ptr  = NULL;

        break;
    }
    if (offset == GRAPHICS_AUTO_OFFSET) offset = -(x + y * sw);

    if (FEATURE_MAP_MOVEMENT_FLAG) {
        FEATURE_MAP_MOVEMENT_CLEAR;
        graphics_put_map();
    }

    if (offset != 0) ptr += offset;
    GRAPHICS_DEF(bank, ptr, x, y, w, h, sw, base, set_win_submap);
}

void vm_window(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 x = (UINT8)*(--THIS->stack_ptr);
    const UINT8 y = (UINT8)*(--THIS->stack_ptr);
    move_win(x, y);
}

void vm_wget(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 x   = (UINT8)*(--THIS->stack_ptr);
    const UINT8 y   = (UINT8)*(--THIS->stack_ptr);
    const UINT8 ret = get_win_tile_xy(x, y);
    *(THIS->stack_ptr++) = ret;
}

void vm_wset(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 x = (UINT8)*(--THIS->stack_ptr);
    const UINT8 y = (UINT8)*(--THIS->stack_ptr);
    const UINT8 t = (UINT8)*(--THIS->stack_ptr);
    set_win_tile_xy(x, y, t);
}

void vm_def_sprite(SCRIPT_CTX * THIS, UINT8 src) OLDCALL BANKED {
    (void)src;
    const UINT8 n = (UINT8)*(--THIS->stack_ptr);
    const UINT8 t = (UINT8)*(--THIS->stack_ptr);
    set_sprite_tile(n, t);
}

void vm_sprite(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 id = (UINT8)*(--THIS->stack_ptr);
    const UINT8 x  = (UINT8)*(--THIS->stack_ptr);
    const UINT8 y  = (UINT8)*(--THIS->stack_ptr);
    move_sprite(id, x, y);
}

void vm_sget(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 n   = (UINT8)*(--THIS->stack_ptr);
    const UINT8 ret = get_sprite_tile(n);
    *(THIS->stack_ptr++) = ret;
}

void vm_sset(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 n = (UINT8)*(--THIS->stack_ptr);
    const UINT8 t = (UINT8)*(--THIS->stack_ptr);
    set_sprite_tile(n, t);
}

void vm_get_sprite_prop(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 n   = (UINT8)*(--THIS->stack_ptr);
    const UINT8 p   = (UINT8)*(--THIS->stack_ptr);
    UINT8 ret = 0;

    switch (p) {
    case PROPERTY_PALETTE:
        if (device_type & DEVICE_TYPE_CGB) {
            ret = get_sprite_prop(n);
            ret &= 0b00000111;
        }

        break;
    case PROPERTY_BANK:
        ret = get_sprite_prop(n);
        ret &= S_BANK;

        break;
    case PROPERTY_OBJPAL:
        ret = get_sprite_prop(n);
        ret &= S_PALETTE;

        break;
    case PROPERTY_HFLIP:
        ret = get_sprite_prop(n);
        ret &= S_FLIPX;

        break;
    case PROPERTY_VFLIP:
        ret = get_sprite_prop(n);
        ret &= S_FLIPY;

        break;
    case PROPERTY_PRIORITY:
        ret = get_sprite_prop(n);
        ret &= S_PRIORITY;

        break;
    case PROPERTY_HIDDEN:
        ret = shadow_OAM[n].y == 0;

        break;
    }

    *(THIS->stack_ptr++) = ret ? TRUE : FALSE;
}

void vm_set_sprite_prop(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 n    = (UINT8)*(--THIS->stack_ptr);
    const UINT8 p    = (UINT8)*(--THIS->stack_ptr);
    const UINT8 val  = (UINT8)*(--THIS->stack_ptr);
    UINT8 prop = 0;

    switch (p) {
    case PROPERTY_PALETTE:
        if (device_type & DEVICE_TYPE_CGB) {
            prop = get_sprite_prop(n);
            prop = (prop & 0b11111000) | (val & 0b00000111);
            set_sprite_prop(n, prop);
        }

        break;
    case PROPERTY_BANK:
        prop = get_sprite_prop(n);
        if (val) prop |= S_BANK; else prop &= ~S_BANK;
        set_sprite_prop(n, prop);

        break;
    case PROPERTY_OBJPAL:
        prop = get_sprite_prop(n);
        if (val) prop |= S_PALETTE; else prop &= ~S_PALETTE;
        set_sprite_prop(n, prop);

        break;
    case PROPERTY_HFLIP:
        prop = get_sprite_prop(n);
        if (val) prop |= S_FLIPX; else prop &= ~S_FLIPX;
        set_sprite_prop(n, prop);

        break;
    case PROPERTY_VFLIP:
        prop = get_sprite_prop(n);
        if (val) prop |= S_FLIPY; else prop &= ~S_FLIPY;
        set_sprite_prop(n, prop);

        break;
    case PROPERTY_PRIORITY:
        prop = get_sprite_prop(n);
        if (val) prop |= S_PRIORITY; else prop &= ~S_PRIORITY;
        set_sprite_prop(n, prop);

        break;
    case PROPERTY_HIDDEN:
        if (val) hide_sprite(n); else shadow_OAM[n].y = 32;

        break;
    }
}
