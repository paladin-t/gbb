#pragma bank 255

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "utils/utils.h"

#include "vm_graphics.h"
#include "vm_gui.h"
#include "vm_input.h"

BANKREF(VM_GUI_MENU)

void gui_menu_update(void) BANKED {
    BOOLEAN moved = FALSE;
    BOOLEAN confirmed = FALSE;
    if (INPUT_IS_BTN_UP(J_DOWN)) {
        if (++gui_menu_item_cursor >= gui_menu_item_count)
            gui_menu_item_cursor = 0;
        moved = TRUE;
    } else if (INPUT_IS_BTN_UP(J_UP)) {
        if (--gui_menu_item_cursor == 0xFF)
            gui_menu_item_cursor = gui_menu_item_count - 1;
        moved = TRUE;
    } else if (INPUT_IS_BTN_UP(J_A)) {
        confirmed = TRUE;
    }

#if GUI_IMPLEMENT_TOUCH_HANDLING_ENABLED
    BOOLEAN entered = FALSE;
    BOOLEAN left = FALSE;
    do {
        if (!(device_type & DEVICE_TYPE_WITH_TOUCH_SUPPORT)) // Ignore touch handling if touch feature is not supported.
            break;

        const UINT8 margin_y = GUI_MARGIN_Y(gui_margin);
        const UINT8 line_height = GUI_MENU_LINE_HEIGHT;
        UINT8 x = INPUT_TOUCH_X;
        UINT8 y = INPUT_TOUCH_Y;
        if (GUI_CATEGORY_LAYER(gui_widget_category) == GRAPHICS_LAYER_WINDOW) {
            x -= WX_REG; x += 7;
            y -= WY_REG;
        } else /* if (GUI_CATEGORY_LAYER(gui_widget_category) == GRAPHICS_LAYER_MAP) */ {
            x -= SCX_REG;
            y -= SCY_REG;
        }

        const boundingbox_t box = { // In pixels.
            .left   = MUL8(gui_cursor_x),
            .right  = MUL8(gui_cursor_x + gui_width) - 1,
            .top    = MUL8(gui_cursor_y) + margin_y,
            .bottom = MUL8(gui_cursor_y) + margin_y + line_height * gui_menu_item_count
        };
        const upoint16_t pos = { // In pixels.
            .x      = x,
            .y      = y
        };

        const UINT8 line = (y - box.top) / line_height;
        if (gui_menu_pointer_cursor != line) {
            if (line < gui_menu_item_count && gui_menu_pointer_cursor >= gui_menu_item_count)
                entered = TRUE;
            else if (line >= gui_menu_item_count && gui_menu_pointer_cursor < gui_menu_item_count)
                left = TRUE;
            gui_menu_pointer_cursor = line;
        }

        if (!boundingbox_contains_point(&box, &pos))
            break;

        if (line < gui_menu_item_count) {
            if (gui_menu_item_cursor != line) {
                gui_menu_item_cursor = line;
                moved = TRUE;
            }

            if (!(INPUT_IS_TOUCH_UP & TOUCH_BUTTON_0))
                break;

            confirmed = TRUE;
        }
    } while (0);

    if (moved)
        script_execute(gui_menu_handler_bank, (UINT8 *)gui_menu_handler_address, NULL, 2, EVENT_HANDLER_CHANGE, gui_menu_item_cursor);
    else if (confirmed)
        script_execute(gui_menu_handler_bank, (UINT8 *)gui_menu_handler_address, NULL, 2, EVENT_HANDLER_CONFIRM, gui_menu_item_cursor);
    else if (entered)
        script_execute(gui_menu_handler_bank, (UINT8 *)gui_menu_handler_address, NULL, 2, EVENT_HANDLER_ENTER, gui_menu_item_cursor);
    else if (left)
        script_execute(gui_menu_handler_bank, (UINT8 *)gui_menu_handler_address, NULL, 2, EVENT_HANDLER_LEAVE, gui_menu_item_cursor);
#else /* GUI_IMPLEMENT_TOUCH_HANDLING_ENABLED */
    if (moved)
        script_execute(gui_menu_handler_bank, (UINT8 *)gui_menu_handler_address, NULL, 2, EVENT_HANDLER_CHANGE, gui_menu_item_cursor);
    else if (confirmed)
        script_execute(gui_menu_handler_bank, (UINT8 *)gui_menu_handler_address, NULL, 2, EVENT_HANDLER_CONFIRM, gui_menu_item_cursor);
#endif /* GUI_IMPLEMENT_TOUCH_HANDLING_ENABLED */
}

void vm_menu(SCRIPT_CTX * THIS, UINT8 nlines, UINT8 nargs, UINT8 font_bank, UINT8 * font_ptr) OLDCALL BANKED {
    // Prepare.
    if (!nlines) { // Clear.
        gui_text_ptr = NULL;
        gui_menu_item_count = 0;
        gui_menu_item_cursor = 0;
        GUI_MENU_CLEAR_LINE_HEIGHT;

        gui_clear(gui_base_tile, (UINT16)gui_width * (UINT16)gui_height); // Takes width * height tiles.

        return;
    }

    const INT16 * args = (INT16 *)THIS->PC;
    const glyph_t * arb = (const glyph_t *)GUI_GLYPH_ARBITRARY_ADDRESS(font_ptr);

    // Initialize.
    gui_menu_item_count = nlines;
    gui_menu_item_cursor = 0;

    gui_text_ptr = (const glyph_t *)(THIS->PC + MUL2(nargs));

    glyph_option_t opt;
    get_chunk((UINT8 *)&opt, font_bank, font_ptr, sizeof(glyph_option_t)); // Option.
    const UINT8 size = get_uint8(font_bank, font_ptr + sizeof(glyph_option_t)); // Safe size.

    GUI_MENU_SET_LINE_HEIGHT(size);

    // Preserve the cursor position.
    const UINT8 pos_x = gui_cursor_x;
    const UINT8 pos_y = gui_cursor_y;

    // Output all glyphs.
    gui_cursor_x = GUI_MARGIN_X(gui_margin);
    gui_cursor_y = GUI_MARGIN_Y(gui_margin);
    for (UINT8 i = 0; i != nlines; ++i) {
#if GUI_IMPLEMENT_MENU_PAGING_ENABLED
        if (gui_cursor_y + GUI_MARGIN_Y(gui_margin) + size > MUL8(gui_height)) { // Page is full.
            for (; ; ) {
                glyph_t glyph;
                get_chunk((UINT8 *)&glyph, THIS->bank, (UINT8 *)(++gui_text_ptr), sizeof(glyph_t));
                if (GUI_GLYPH_IS_TERMINATION(glyph)) {
                    THIS->PC = (const UINT8 *)(++gui_text_ptr);

                    break;
                }
            }

            break;
        }
#endif /* GUI_IMPLEMENT_MENU_PAGING_ENABLED */

        for (; ; ) {
            glyph_t glyph;
            get_chunk((UINT8 *)&glyph, THIS->bank, (UINT8 *)gui_text_ptr, sizeof(glyph_t));
            if (GUI_GLYPH_IS_ESCAPE_PLACEHOLDER(glyph)) {
                // Escape.
                get_chunk((UINT8 *)&glyph, THIS->bank, (UINT8 *)(++gui_text_ptr), sizeof(glyph_t));
                if (GUI_GLYPH_IS_ESCAPE_INT(glyph)) {
                    // Serialize an integer.
                    const INT16 ref = get_int16(THIS->bank, (UINT8 *)args);
                    const INT16 val = *((INT16 *)VM_REF_TO_PTR(ref));
                    gui_blit_arbitrary_int(val, font_bank, size, arb, &opt);
                    ++args;
                } else if (GUI_GLYPH_IS_ESCAPE_HEX(glyph)) {
                    // Serialize an integer in HEX.
                    const INT16 ref = get_int16(THIS->bank, (UINT8 *)args);
                    const UINT16 val = *((UINT16 *)VM_REF_TO_PTR(ref));
                    gui_blit_arbitrary_hex(val, font_bank, size, arb, &opt);
                    ++args;
                } else if (GUI_GLYPH_IS_ESCAPE_CHAR(glyph)) {
                    // Serialize a character.
                    glyph_t glyph_;
                    const INT16 ref = get_int16(THIS->bank, (UINT8 *)args);
                    const UINT16 val = *((UINT16 *)VM_REF_TO_PTR(ref));
                    get_chunk((UINT8 *)&glyph_, font_bank, (UINT8 *)(arb + val), sizeof(glyph_t));
                    gui_blit_char(size, &glyph_, &opt);
                    ++args;
                } else if (GUI_GLYPH_IS_ESCAPE_PERCENT(glyph)) {
                    // Serialize '%'.
                    glyph_t glyph_;
                    get_chunk((UINT8 *)&glyph_, font_bank, (UINT8 *)(arb + '%'), sizeof(glyph_t));
                    gui_blit_char(size, &glyph_, &opt);
                }
            } else if (GUI_GLYPH_IS_ESCAPE_SPECIAL(glyph)) {
                // Escape.
                get_chunk((UINT8 *)&glyph, THIS->bank, (UINT8 *)(++gui_text_ptr), sizeof(glyph_t));
                if (GUI_GLYPH_IS_ESCAPE_STACK(glyph)) {
                    // Serialize the current stack pointer of the current thread.
                    const UINT16 val = (UINT16)THIS->stack_ptr;
                    gui_blit_arbitrary_hex(val, font_bank, size, arb, &opt);
                } else if (GUI_GLYPH_IS_ESCAPE_BACKSLASH(glyph)) {
                    // Serialize '\'.
                    glyph_t glyph_;
                    get_chunk((UINT8 *)&glyph_, font_bank, (UINT8 *)(arb + '\\'), sizeof(glyph_t));
                    gui_blit_char(size, &glyph_, &opt);
                }
            } else if (GUI_GLYPH_IS_TERMINATION(glyph)) {
                // Terminate the current sequence.
                THIS->PC = (const UINT8 *)(++gui_text_ptr);
                gui_blit_new_line(size);

                break;
            } else {
                // Output a regular character.
                gui_blit_char(size, &glyph, &opt);
            }

            get_chunk((UINT8 *)&glyph, THIS->bank, (UINT8 *)(++gui_text_ptr), sizeof(glyph_t));
            if (GUI_GLYPH_IS_TERMINATION(glyph)) {
                // Terminate the current sequence.
                THIS->PC = (const UINT8 *)(++gui_text_ptr);
                gui_blit_new_line(size);

                break;
            }
        }
    }

    // Restore the cursor position.
    gui_cursor_x = pos_x;
    gui_cursor_y = pos_y;
}
