#pragma bank 255

#if defined __SDCC
#   pragma disable_warning 110
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include <string.h>

#include "utils/text.h"
#include "utils/utils.h"

#include "vm_device.h"
#include "vm_graphics.h"
#include "vm_gui.h"

BANKREF(VM_GUI)

#define GUI_BLIT_INTERVAL       10
#define GUI_BLIT_MAX_INTERVAL   0x1F

static const UINT8 GUI_BLIT_GLYPH_PICK_MASKS[8] = {
    0b10000000,
    0b11000000,
    0b11100000,
    0b11110000,
    0b11111000,
    0b11111100,
    0b11111110,
    0b11111111
};

UINT8 gui_widget_category;
UINT8 gui_base_tile;
UINT8 gui_tiles_bank;
UINT8 * gui_tiles_address;
UINT8 gui_width;
UINT8 gui_height;
UINT8 gui_margin;
UINT8 gui_cursor_x;
UINT8 gui_cursor_y;
const glyph_t * gui_text_ptr;
UINT8 gui_interval;
UINT8 gui_ticks;
UINT8 gui_button_previous;
UINT8 gui_extra_uint8;
UINT16 gui_extra_uint16;
UINT8 gui_options;

void gui_init(void) BANKED {
    gui_widget_category = GUI_WIDGET_TYPE_NONE;
    gui_base_tile       = 0;
    gui_tiles_bank      = 0;
    gui_tiles_address   = NULL;
    gui_width           = 0;
    gui_height          = 0;
    gui_margin          = 0x00;
    gui_cursor_x        = 0;
    gui_cursor_y        = 0;
    gui_text_ptr        = NULL;
    gui_interval        = GUI_BLIT_INTERVAL;
    gui_ticks           = gui_interval;
    gui_button_previous = 0x00;
    gui_extra_uint8     = 0;
    gui_extra_uint16    = 0x0000;
    gui_options         = 0x00;
}

void gui_clear(UINT8 base_tile, UINT16 n) BANKED {
    if (gui_tiles_bank) {
        switch (GUI_CATEGORY_TYPE(gui_widget_category)) {
        case GUI_WIDGET_TYPE_LABEL:
            call_v_bbp_oldcall(
                gui_base_tile, gui_width * gui_height,
                gui_tiles_bank, gui_tiles_address,
                set_bkg_data
            );

            break;
        case GUI_WIDGET_TYPE_PROGRESSBAR:
            call_v_bbp_oldcall(
                gui_base_tile, GUI_PROGRESSBAR_TILE_COUNT,
                gui_tiles_bank, gui_tiles_address,
                set_bkg_data
            );

            break;
        case GUI_WIDGET_TYPE_MENU:
            call_v_bbp_oldcall(
                gui_base_tile, gui_width * gui_height,
                gui_tiles_bank, gui_tiles_address,
                set_bkg_data
            );

            break;
        }
    } else {
        const UINT8 BLANK[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        for (UINT16 i = 0; i != n; ++i) {
            const UINT16 idx = i + base_tile;
            set_bkg_1bpp_data(idx, 1, BLANK);
        }
    }
}

void gui_tile_filled(UINT8 layer, UINT8 first, UINT8 n, UINT8 bank, UINT8 * ptr) BANKED {
    if (layer != GRAPHICS_LAYER_MAP && layer != GRAPHICS_LAYER_WINDOW)
        return;
    if (first != gui_base_tile)
        return;

    switch (GUI_CATEGORY_TYPE(gui_widget_category)) {
    case GUI_WIDGET_TYPE_LABEL:
        if (n != gui_width * gui_height)
            return;

        break;
    case GUI_WIDGET_TYPE_PROGRESSBAR:
        if (n != GUI_PROGRESSBAR_TILE_COUNT)
            return;

        break;
    case GUI_WIDGET_TYPE_MENU:
        if (n != gui_width * gui_height)
            return;

        break;
    }

    gui_tiles_bank = bank;
    gui_tiles_address = ptr;
}

void gui_blit_char(UINT8 size, const glyph_t * glyph, const glyph_option_t * option) BANKED {
    // Prepare.
    (void)size;

    // Get the option.
    const BOOLEAN _2bpp = option->two_bits_per_pixel;
    const BOOLEAN inv = option->inverted;

    // Get the pixels of the glyph.
    UINT8 buf[64]; // Sufficient for up to 16x16 pixels.
    const UINT8 w = GUI_GLYPH_WIDTH(*glyph), h = GUI_GLYPH_HEIGHT(*glyph);
    const UINT8 n = GUI_GLYPH_BYTES(w, h);
    if (GUI_GLYPH_IS_SPACE(*glyph)) {
        memset(buf, inv ? 0xFF : 0, sizeof(buf));
    } else {
        get_chunk(buf, glyph->bank, glyph->ptr, _2bpp ? MUL2(n) : n);
    }

    // Iterate all of the glyph's bytes.
    UINT8 sx = 0;                                 // The x coordinate on the source glyph.
    UINT8 x = gui_cursor_x + sx;                  // The x coordinate on the VRAM.
    UINT8 tx = DIV8(x);                           // The tile's x index.
    UINT8 bx = MOD8(x);                           // The tile's x bit.
    UINT8 sy = 0;                                 // The y coordinate on the source glyph.
    UINT8 y = gui_cursor_y + sy;                  // The y coordinate on the VRAM.
    UINT8 ty = DIV8(y);                           // The tile's y index.
    UINT8 by = MOD8(y);                           // The tile's y bit.
    UINT8 byte0 = 0, byte1 = 0;
    for (UINT8 k = 0; k != n; ++k) {
        // Retrieve a byte or a pair of bytes for eight pixels.
        if (_2bpp) {
            byte0 = buf[MUL2(k)];
            byte1 = buf[MUL2(k) + 1];
        } else {
            byte0 = buf[k];
        }

        // Consume the byte(s).
        for (
            INT8 remain = 8,                      // The remain bits to consume.
            take = MIN(w - sx, 8 - bx);           // The bits to take for the current step.
            remain > 0;
            (remain -= take), (take = MIN(MIN(w - sx, 8 - bx), remain))
        ) {
            // Get the pixels on the VRAM.
            const UINT8 base = gui_base_tile + (tx + ty * gui_width);
            UINT8 * addr = VRAM_BASE_TILE_ADDRESS(base) + MUL2(by);
            const UINT8 b0 = get_vram_byte(addr);
            const UINT8 b1 = get_vram_byte(addr + 1);

            // Blit the first bits.
            const UINT8 mask = GUI_BLIT_GLYPH_PICK_MASKS[MIN(take, 8) - 1];
            const UINT8 a0 = byte0 & mask;        // Get the pixels on the glyph.

            const UINT8 p0 = b0 | (a0 >> bx);     // Blit the pixels.
            set_vram_byte(addr, p0);

            byte0 <<= take;                       // Shift the byte(s).

            // Blit the secondary bits.
            if (_2bpp) {
                const UINT8 a1 = byte1 & mask;    // Get the pixels on the glyph.

                const UINT8 p1 = b1 | (a1 >> bx); // Blit the pixels.
                set_vram_byte(addr + 1, p1);

                byte1 <<= take;                   // Shift the byte(s).
            } else {
                const UINT8 p1 = b1 | (a0 >> bx); // Blit the pixels.
                set_vram_byte(addr + 1, p1);
            }

            // Move the consumption window.
            sx += take;
            if (sx >= w) { // Finished a line on the glyph.
                sx = 0;
                ++sy; // Next line.
                y = gui_cursor_y + sy;
                ty = DIV8(y);
                by = MOD8(y);
            }
            x = gui_cursor_x + sx;
            tx = DIV8(x);
            bx = MOD8(x);
        }
    }
    // Move the cursor.
    gui_cursor_x += w;

    // Process line wrapping.
    if (gui_cursor_x + GUI_MARGIN_X(gui_margin) >= MUL8(gui_width)) {
        gui_cursor_x = GUI_MARGIN_X(gui_margin) + GUI_LABEL_X_OFFSET;
        gui_cursor_y += h;
    }
}

void gui_blit_arbitrary_int(INT16 val, UINT8 bank, UINT8 size, const glyph_t * arb, const glyph_option_t * option) BANKED {
    unsigned char display_text[8];
    unsigned char * d = display_text;
    d += int16_to_str(val, d);
    for (unsigned char * c = display_text; c < d; ++c) {
        glyph_t glyph;
        get_chunk((UINT8 *)&glyph, bank, (UINT8 *)(arb + *c), sizeof(glyph_t));
        gui_blit_char(size, &glyph, option);
    }
}

void gui_blit_arbitrary_hex(UINT16 val, UINT8 bank, UINT8 size, const glyph_t * arb, const glyph_option_t * option) BANKED {
    unsigned char display_text[8];
    unsigned char * d = display_text;
    d += uint16_to_hex_full(val, d);
    for (unsigned char * c = display_text; c < d; ++c) {
        glyph_t glyph;
        get_chunk((UINT8 *)&glyph, bank, (UINT8 *)(arb + *c), sizeof(glyph_t));
        gui_blit_char(size, &glyph, option);
    }
}

void gui_blit_new_line(UINT8 size) BANKED {
    gui_cursor_x = GUI_MARGIN_X(gui_margin) + GUI_LABEL_X_OFFSET;
    gui_cursor_y += size;
}

INLINE void gui_def_none(SCRIPT_CTX * THIS) {
    (void)THIS;

    gui_init();
}

INLINE void gui_def_label(SCRIPT_CTX * THIS) {
    const UINT8 type         = GUI_WIDGET_TYPE_LABEL;
    const UINT8 x            = (UINT8)*(--THIS->stack_ptr);
    const UINT8 y            = (UINT8)*(--THIS->stack_ptr);
    gui_width                = (UINT8)*(--THIS->stack_ptr);
    gui_height               = (UINT8)*(--THIS->stack_ptr);
    gui_base_tile            = (UINT8)*(--THIS->stack_ptr);
    const UINT8 layer        = (UINT8)*(--THIS->stack_ptr);
    const UINT8 margin_x     = (UINT8)*(--THIS->stack_ptr) & 0x0F; // Up to 4 bits.
    const UINT8 margin_y     = (UINT8)*(--THIS->stack_ptr) & 0x0F; // Up to 4 bits.
    gui_interval             = (UINT8)*(--THIS->stack_ptr);
    gui_widget_category      = GUI_CATEGORY(type, layer);
    gui_tiles_bank           = 0;
    gui_tiles_address        = NULL;
    gui_margin               = GUI_MARGIN(margin_x, margin_y);
    gui_cursor_x             = margin_x;
    gui_cursor_y             = margin_y;
    gui_text_ptr             = NULL;
    gui_ticks                = gui_interval; if (gui_interval > GUI_BLIT_MAX_INTERVAL) gui_interval = 0;
    gui_button_previous      = 0x00;
    gui_label_touch_states   = 0;
    gui_label_start_position = GUI_LABEL_POSITION(x, y);
    gui_label_x_offset       = GUI_LABEL_X_OFFSET_OF((UINT8)*(--THIS->stack_ptr)); gui_cursor_x += GUI_LABEL_X_OFFSET;

    gui_clear(gui_base_tile, (UINT16)gui_width * (UINT16)gui_height); // Takes width * height tiles.

    UINT8 k = gui_base_tile;
    if (layer == GRAPHICS_LAYER_WINDOW) {
        for (UINT8 j = 0; j != gui_height; ++j) {
            for (UINT8 i = 0; i != gui_width; ++i) {
                set_win_tile_xy(x + i, y + j, k++);
            }
        }
    } else /* if (layer == GRAPHICS_LAYER_MAP) */ {
        for (UINT8 j = 0; j != gui_height; ++j) {
            for (UINT8 i = 0; i != gui_width; ++i) {
                set_bkg_tile_xy(x + i, y + j, k++);
            }
        }
    }
}

INLINE void gui_def_progressbar(SCRIPT_CTX * THIS) {
    const UINT8 type              = GUI_WIDGET_TYPE_PROGRESSBAR;
    gui_cursor_x                  = (UINT8)*(--THIS->stack_ptr);
    gui_cursor_y                  = (UINT8)*(--THIS->stack_ptr);
    gui_width                     = (UINT8)*(--THIS->stack_ptr);
    gui_height                    = 1;
    gui_base_tile                 = (UINT8)*(--THIS->stack_ptr);
    const UINT8 layer             = (UINT8)*(--THIS->stack_ptr);
    UINT8 margin_x                = (UINT8)*(--THIS->stack_ptr) & 0x0F; // Up to 4 bits.
    UINT8 margin_y                = (UINT8)*(--THIS->stack_ptr) & 0x0F; // Up to 4 bits.
    const UINT8 palette_b         = (UINT8)*(--THIS->stack_ptr);
    const UINT8 palette_c         = (UINT8)*(--THIS->stack_ptr);
    gui_widget_category           = GUI_CATEGORY(type, layer);
    gui_tiles_bank                = 0;
    gui_tiles_address             = NULL;
    gui_width                     = MAX(gui_width, 3);
    margin_x                      = MIN(margin_x, 7);
    margin_y                      = MIN(margin_y, 3);
    gui_margin                    = GUI_MARGIN(margin_x, margin_y);
    gui_text_ptr                  = NULL;
    gui_interval                  = GUI_BLIT_INTERVAL;
    gui_ticks                     = gui_interval;
    gui_button_previous           = 0x00;
    gui_extra_uint8               = 0;
    gui_extra_uint16              = 0x0000;
    gui_progressbar_blit_palettes = GUI_PALETTE(palette_b, palette_c);

    gui_clear(gui_base_tile, GUI_PROGRESSBAR_TILE_COUNT); // Takes `GUI_PROGRESSBAR_TILE_COUNT` tiles.

    if (layer == GRAPHICS_LAYER_WINDOW) {
        set_win_tile_xy(gui_cursor_x,                 gui_cursor_y, gui_base_tile);
        set_win_tile_xy(gui_cursor_x + gui_width - 1, gui_cursor_y, gui_base_tile + 4);
        for (UINT8 i = 1; i != gui_width - 1; ++i) {
            set_win_tile_xy(gui_cursor_x + i,         gui_cursor_y, gui_base_tile + 2);
        }
    } else /* if (layer == GRAPHICS_LAYER_MAP) */ {
        set_bkg_tile_xy(gui_cursor_x,                 gui_cursor_y, gui_base_tile);
        set_bkg_tile_xy(gui_cursor_x + gui_width - 1, gui_cursor_y, gui_base_tile + 4);
        for (UINT8 i = 1; i != gui_width - 1; ++i) {
            set_bkg_tile_xy(gui_cursor_x + i,         gui_cursor_y, gui_base_tile + 2);
        }
    }

    gui_blit_progressbar_body_full();
    gui_blit_progressbar_body_empty();
    gui_blit_progressbar_head_body_tail(0, 0, 0);
}

INLINE void gui_def_menu(SCRIPT_CTX * THIS) {
    const UINT8 type         = GUI_WIDGET_TYPE_MENU;
    const UINT8 x            = (UINT8)*(--THIS->stack_ptr);
    const UINT8 y            = (UINT8)*(--THIS->stack_ptr);
    gui_width                = (UINT8)*(--THIS->stack_ptr);
    gui_height               = (UINT8)*(--THIS->stack_ptr);
    gui_base_tile            = (UINT8)*(--THIS->stack_ptr);
    const UINT8 layer        = (UINT8)*(--THIS->stack_ptr);
    const UINT8 margin_x     = (UINT8)*(--THIS->stack_ptr) & 0x0F; // Up to 4 bits.
    const UINT8 margin_y     = (UINT8)*(--THIS->stack_ptr) & 0x0F; // Up to 4 bits.
    gui_widget_category      = GUI_CATEGORY(type, layer);
    gui_tiles_bank           = 0;
    gui_tiles_address        = NULL;
    gui_margin               = GUI_MARGIN(margin_x, margin_y);
    gui_cursor_x             = x;
    gui_cursor_y             = y;
    gui_text_ptr             = NULL;
    gui_menu_item_count      = 0;
    gui_menu_item_cursor     = 0;
    gui_menu_pointer_cursor  = 0xFF;
    gui_menu_handler_bank    = 0;
    gui_menu_handler_address = 0x0000;
    gui_menu_line_height     = 0x00;

    gui_clear(gui_base_tile, (UINT16)gui_width * (UINT16)gui_height); // Takes width * height tiles.

    UINT8 k = gui_base_tile;
    if (layer == GRAPHICS_LAYER_WINDOW) {
        for (UINT8 j = 0; j != gui_height; ++j) {
            for (UINT8 i = 0; i != gui_width; ++i) {
                set_win_tile_xy(x + i, y + j, k++);
            }
        }
    } else /* if (layer == GRAPHICS_LAYER_MAP) */ {
        for (UINT8 j = 0; j != gui_height; ++j) {
            for (UINT8 i = 0; i != gui_width; ++i) {
                set_bkg_tile_xy(x + i, y + j, k++);
            }
        }
    }
}

void vm_def_widget(SCRIPT_CTX * THIS, UINT8 type) OLDCALL BANKED {
    switch (type) {
    case GUI_WIDGET_TYPE_NONE:
        gui_def_none(THIS);

        break;
    case GUI_WIDGET_TYPE_LABEL:
        gui_def_label(THIS);

        break;
    case GUI_WIDGET_TYPE_PROGRESSBAR:
        gui_def_progressbar(THIS);

        break;
    case GUI_WIDGET_TYPE_MENU:
        gui_def_menu(THIS);

        break;
    }
}

void vm_on_widget(SCRIPT_CTX * THIS, UINT8 event_bank, UINT8 * event_ptr) OLDCALL BANKED {
    (void)THIS;

    if (event_bank) {
        gui_widget_handler_bank    = event_bank;
        gui_widget_handler_address = (UINT16)event_ptr;
    } else {
        gui_widget_handler_bank    = 0;
        gui_widget_handler_address = 0x0000;
    }
}
