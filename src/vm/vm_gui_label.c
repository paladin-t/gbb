#pragma bank 255

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "utils/utils.h"

#include "vm_graphics.h"
#include "vm_gui.h"
#include "vm_input.h"

BANKREF(VM_GUI_LABEL)

#define GUI_LABEL_IS_TOUCH_PRESSED   (gui_label_touch_states & 0x0F)
#define GUI_LABEL_IS_TOUCH_DOWN      ((gui_label_touch_states & 0x0F) & ~(gui_label_touch_states >> 4))
#define GUI_LABEL_IS_TOUCH_UP        (~(gui_label_touch_states & 0x0F) & (gui_label_touch_states >> 4))
#define GUI_LABEL_ACCEPT_TOUCH       do { gui_label_touch_states = (gui_label_touch_states << 4) | *(UINT8 *)TOUCH_PRESSED_REG; } while (0)

INLINE BOOLEAN gui_label_btn_down(void) {
    const UINT8 j = joypad();
    const UINT8 ret = (j & (J_A)) & ~(gui_button_previous & (J_A));
    gui_button_previous = j;

    if (ret)
        return TRUE;

#if GUI_IMPLEMENT_TOUCH_HANDLING_ENABLED
    do {
        if (!(device_type & DEVICE_TYPE_GBB)) // Ignore touch handling if extension features are not supported.
            break;

        GUI_LABEL_ACCEPT_TOUCH;

        if (!(GUI_LABEL_IS_TOUCH_UP & TOUCH_BUTTON_0))
            break;

#   if GUI_IMPLEMENT_LABEL_ACCURATE_TOUCH_HANDLING_ENABLED
        UINT8 x = *(UINT8 *)TOUCH_X_REG;
        UINT8 y = *(UINT8 *)TOUCH_Y_REG;
        if (GUI_CATEGORY_LAYER(gui_widget_category) == GRAPHICS_LAYER_WINDOW) {
            x -= WX_REG; x += 7;
            y -= WY_REG;
        } else /* if (GUI_CATEGORY_LAYER(gui_widget_category) == GRAPHICS_LAYER_MAP) */ {
            x -= SCX_REG;
            y -= SCY_REG;
        }

        const UINT8 tx = GUI_LABEL_POSITION_X(gui_label_start_position);
        const UINT8 ty = GUI_LABEL_POSITION_Y(gui_label_start_position);
        const boundingbox_t box = { // In pixels.
            .left   = MUL8(tx),
            .right  = MUL8(tx + gui_width) - 1,
            .top    = MUL8(ty),
            .bottom = MUL8(ty + gui_height) - 1
        };
        const upoint16_t pos = { // In pixels.
            .x      = x,
            .y      = y
        };
        if (boundingbox_contains_point(&box, &pos))
            return TRUE;
#   else /* GUI_IMPLEMENT_LABEL_ACCURATE_TOUCH_HANDLING_ENABLED */
        return TRUE;
#   endif /* GUI_IMPLEMENT_LABEL_ACCURATE_TOUCH_HANDLING_ENABLED */
    } while (0);
#endif /* GUI_IMPLEMENT_TOUCH_HANDLING_ENABLED */

    return FALSE;
}

INLINE void gui_label_new_line(UINT8 size) {
    gui_blit_new_line(size);
}

INLINE void gui_label_terminate(SCRIPT_CTX * THIS, BOOLEAN new_line, UINT8 size) {
    if (new_line)
        gui_label_new_line(size);
    ++gui_text_ptr;

    THIS->PC = (const UINT8 *)gui_text_ptr;
    gui_text_ptr = NULL;
    gui_ticks = gui_interval;
}

void vm_label(SCRIPT_CTX * THIS, BOOLEAN new_line, UINT8 nargs, UINT8 font_bank, UINT8 * font_ptr) OLDCALL BANKED {
    // Prepare.
    const INT16 * args = (INT16 *)THIS->PC;
    const glyph_t * arb = (const glyph_t *)GUI_GLYPH_ARBITRARY_ADDRESS(font_ptr);

    // Initialize.
    if (!gui_text_ptr)
        gui_text_ptr = (const glyph_t *)(THIS->PC + MUL2(nargs));

    // Wait for input to turn to next page.
    if (gui_ticks == GUI_WAIT_FOR_NEXT_PAGE) {
        if (gui_label_btn_down()) {
            // Next page.
            gui_cursor_x = GUI_MARGIN_X(gui_margin) + GUI_LABEL_X_OFFSET;
            gui_cursor_y = GUI_MARGIN_Y(gui_margin);
            gui_ticks = gui_interval;
            gui_clear(gui_base_tile, (UINT16)gui_width * (UINT16)gui_height);
        } else {
            // Continue waiting.
            goto wait;
        }
    }

    // Update the next glyph.
    BOOLEAN is_head = TRUE;
_loop:
    if (gui_ticks >= gui_interval) {
        glyph_option_t opt;
        get_chunk((UINT8 *)&opt, font_bank, font_ptr, sizeof(glyph_option_t)); // Option.
        const UINT8 size = get_uint8(font_bank, font_ptr + sizeof(glyph_option_t)); // Safe size.
        if (gui_cursor_y + GUI_MARGIN_Y(gui_margin) + size > MUL8(gui_height)) { // Page is full.
            gui_ticks = GUI_WAIT_FOR_NEXT_PAGE; // Wait for input.

            goto wait;
        }

        glyph_t glyph;
        get_chunk((UINT8 *)&glyph, THIS->bank, (UINT8 *)gui_text_ptr, sizeof(glyph_t));
        if (GUI_GLYPH_IS_ESCAPE_PLACEHOLDER(glyph)) {
            // Escape.
            get_chunk((UINT8 *)&glyph, THIS->bank, (UINT8 *)(++gui_text_ptr), sizeof(glyph_t));
            if (GUI_GLYPH_IS_ESCAPE_INT(glyph)) {
                // Serialize an integer.
                INT16 val = *((INT16 *)VM_REF_TO_PTR(get_int16(THIS->bank, (UINT8 *)args)));
                gui_blit_arbitrary_int(val, font_bank, size, arb, &opt);
                ++args;
            } else if (GUI_GLYPH_IS_ESCAPE_HEX(glyph)) {
                // Serialize an integer in HEX.
                UINT16 val = *((UINT16 *)VM_REF_TO_PTR(get_int16(THIS->bank, (UINT8 *)args)));
                gui_blit_arbitrary_hex(val, font_bank, size, arb, &opt);
                ++args;
            } else if (GUI_GLYPH_IS_ESCAPE_CHAR(glyph)) {
                // Serialize a character.
                glyph_t glyph_;
                INT16 val = *((INT16 *)VM_REF_TO_PTR(get_int16(THIS->bank, (UINT8 *)args)));
                get_chunk((UINT8 *)&glyph_, font_bank, (UINT8 *)(arb + val), sizeof(glyph_t));
                gui_blit_char(size, &glyph_, &opt);
                ++args;
            } else if (GUI_GLYPH_IS_ESCAPE_PERCENT(glyph)) {
                // Serialize '%'.
                glyph_t glyph_;
                get_chunk((UINT8 *)&glyph_, font_bank, (UINT8 *)(arb + '%'), sizeof(glyph_t));
                gui_blit_char(size, &glyph_, &opt);
            }
            is_head = FALSE;
        } else if (GUI_GLYPH_IS_ESCAPE_SPECIAL(glyph)) {
            // Escape.
            get_chunk((UINT8 *)&glyph, THIS->bank, (UINT8 *)(++gui_text_ptr), sizeof(glyph_t));
            if (GUI_GLYPH_IS_ESCAPE_STACK(glyph)) {
                // Serialize the current stack pointer of the current thread.
                UINT16 val = (UINT16)THIS->stack_ptr;
                gui_blit_arbitrary_hex(val, font_bank, size, arb, &opt);
            } else if (GUI_GLYPH_IS_ESCAPE_BACKSLASH(glyph)) {
                // Serialize '\'.
                glyph_t glyph_;
                get_chunk((UINT8 *)&glyph_, font_bank, (UINT8 *)(arb + '\\'), sizeof(glyph_t));
                gui_blit_char(size, &glyph_, &opt);
            }
            is_head = FALSE;
        } else if (GUI_GLYPH_IS_TERMINATION(glyph)) {
            // Terminate the current sequence.
            gui_label_terminate(THIS, new_line, size);

            return;
        } else if (GUI_GLYPH_IS_ESCAPE_RETURN(glyph)) {
            // Carriage return; clear the current page, move the carriage to the beginning of the new page.
            gui_cursor_x = GUI_MARGIN_X(gui_margin) + GUI_LABEL_X_OFFSET;
            gui_cursor_y = GUI_MARGIN_Y(gui_margin);
            gui_clear(gui_base_tile, (UINT16)gui_width * (UINT16)gui_height);
            is_head = TRUE;
        } else if (GUI_GLYPH_IS_ESCAPE_NEW_LINE(glyph)) {
            // New line.
            gui_label_new_line(size);
            is_head = TRUE;
        } else if (GUI_GLYPH_IS_ESCAPE_NEW_PAGE(glyph)) {
            // New page; wait for input, then clear the current page, move the carriage to the beginning of the new page.
            ++gui_text_ptr;
            gui_ticks = GUI_WAIT_FOR_NEXT_PAGE; // Wait for input.
            is_head = TRUE;

            goto wait;
        } else {
            // Process word wrap in advance.
            BOOLEAN ignore = FALSE;
            if (GUI_GLYPH_IS_SPACE(glyph)) {
                const UINT16 adv = GUI_GLYPH_ADVANCE_WIDTH(glyph);
                if (adv != GUI_GLYPH_NO_ADVANCE) {
                    if (adv >= (MUL8(gui_width) - (gui_cursor_x + GUI_MARGIN_X(gui_margin)))) {
                        gui_cursor_x = GUI_MARGIN_X(gui_margin) + GUI_LABEL_X_OFFSET;
                        gui_cursor_y += GUI_GLYPH_HEIGHT(glyph);
                        ignore = TRUE; // Ignore space if it's at the end of a line.
                    } else if (!is_head && gui_cursor_x == GUI_MARGIN_X(gui_margin) + GUI_LABEL_X_OFFSET) {
                        ignore = TRUE; // Ignore space if it's at the beginning of a line.
                    }
                }
            } else {
                is_head = FALSE;
            }

            // Output a regular character.
            if (!ignore) {
                gui_blit_char(size, &glyph, &opt);
            }
        }

        get_chunk((UINT8 *)&glyph, THIS->bank, (UINT8 *)(++gui_text_ptr), sizeof(glyph_t));
        if (GUI_GLYPH_IS_TERMINATION(glyph)) {
            // Terminate the current sequence.
            gui_label_terminate(THIS, new_line, size);

            return;
        }

        if (gui_ticks != GUI_WAIT_FOR_NONE)
            gui_ticks = 0; // Reset the ticks to wait for timeout.
    }

    if (gui_interval == 0)
        goto _loop;

    // Update the ticks.
    if (gui_ticks < gui_interval) {
        if (gui_label_btn_down())
            gui_ticks = GUI_WAIT_FOR_NONE; // Wait for nothing, until termination of the current sequence.
        else
            ++gui_ticks;
    }

    // Wait until timeout for next glyph.
wait:
    THIS->PC -= INSTRUCTION_SIZE + sizeof(new_line) + sizeof(nargs) + sizeof(font_bank) + sizeof(font_ptr);
    THIS->waitable = TRUE;
}
