#pragma bank 255

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include <string.h>

#include "utils/utils.h"

#include "vm_input.h"

BANKREF(VM_INPUT)

UINT8 input_button_pressed;
UINT8 input_button_previous;
UINT8 input_touch_state;
input_handler_t input_handlers[INPUT_HANDLER_COUNT];
UINT8 input_handler_count;
UINT8 input_handler_cursor;

#if USE_SGB_MOUSE
UINT8 input_touch_position_x;
UINT8 input_touch_position_y;
UINT8 input_touch_pressed;
#endif /* USE_SGB_MOUSE */

void input_init(void) BANKED {
    input_button_pressed            = 0;
    input_button_previous           = 0;
    input_touch_state               = 0;
    for (UINT8 i = 0; i != INPUT_HANDLER_COUNT; ++i) {
        input_handlers[i].options   = INPUT_HANDLER_INVALID;
        input_handlers[i].bank      = 0;
        input_handlers[i].address   = NULL;
        input_handlers[i].thread_id = 0;
    }
    input_handler_count             = 0;
    input_handler_cursor            = INPUT_HANDLER_NONE;

#if USE_SGB_MOUSE
    input_touch_position_x = DIV2(DEVICE_SCREEN_PX_WIDTH);
    input_touch_position_y = DIV2(DEVICE_SCREEN_PX_HEIGHT);
    input_touch_pressed = 0;

    if (device_type & DEVICE_TYPE_SGB) {
        joypad_init(4, &joypads);
        sgb_mouse_install();
    }
#endif /* USE_SGB_MOUSE */
}

void vm_btn(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 mask = (UINT8)*(--THIS->stack_ptr);
    *(THIS->stack_ptr++) = INPUT_JOYPAD & mask;
}

void vm_btnd(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 mask = (UINT8)*(--THIS->stack_ptr);
    *(THIS->stack_ptr++) = INPUT_IS_BTN_DOWN(mask);
}

void vm_btnu(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 mask = (UINT8)*(--THIS->stack_ptr);
    *(THIS->stack_ptr++) = INPUT_IS_BTN_UP(mask);
}

void vm_touch(SCRIPT_CTX * THIS, BOOLEAN loc, INT16 idxX, INT16 idxY) OLDCALL BANKED {
    if (device_type & DEVICE_TYPE_WITH_TOUCH_SUPPORT) {
        if (loc) {
            INT16 * X, * Y;
            X = VM_REF_TO_PTR(idxX);
            Y = VM_REF_TO_PTR(idxY);
            *X = INPUT_TOUCH_X;
            *Y = INPUT_TOUCH_Y;
        }
        *(THIS->stack_ptr++) = INPUT_TOUCH_PRESSED;
    } else {
        *(THIS->stack_ptr++) = 0;
    }
}

void vm_touchd(SCRIPT_CTX * THIS, BOOLEAN loc, INT16 idxX, INT16 idxY) OLDCALL BANKED {
    if (device_type & DEVICE_TYPE_WITH_TOUCH_SUPPORT) {
        if (loc) {
            INT16 * X, * Y;
            X = VM_REF_TO_PTR(idxX);
            Y = VM_REF_TO_PTR(idxY);
            *X = INPUT_TOUCH_X;
            *Y = INPUT_TOUCH_Y;
        }
        *(THIS->stack_ptr++) = INPUT_IS_TOUCH_DOWN;
    } else {
        *(THIS->stack_ptr++) = 0;
    }
}

void vm_touchu(SCRIPT_CTX * THIS, BOOLEAN loc, INT16 idxX, INT16 idxY) OLDCALL BANKED {
    if (device_type & DEVICE_TYPE_WITH_TOUCH_SUPPORT) {
        if (loc) {
            INT16 * X, * Y;
            X = VM_REF_TO_PTR(idxX);
            Y = VM_REF_TO_PTR(idxY);
            *X = INPUT_TOUCH_X;
            *Y = INPUT_TOUCH_Y;
        }
        *(THIS->stack_ptr++) = INPUT_IS_TOUCH_UP;
    } else {
        *(THIS->stack_ptr++) = 0;
    }
}

void vm_on_input(SCRIPT_CTX * THIS, UINT8 bank, UINT8 * pc, UINT8 opt) OLDCALL BANKED {
    (void)THIS;

    if (bank) {
        // Update the vector if the callback handler already exists.
        UINT8 i;
        for (i = 0; i != input_handler_count; ++i) {
            if (EVENT_HANDLER_TYPE(input_handlers[i].options) == EVENT_HANDLER_TYPE(opt)) {
                input_handlers[i].options = opt;
                input_handlers[i].bank    = bank;
                input_handlers[i].address = pc;

                return;
            }
        }

        // Add a new callback handler to the vector.
        // `input_handler_count` won't exceed `INPUT_HANDLER_COUNT`.
        i = input_handler_count++;
        input_handlers[i].options = opt;
        input_handlers[i].bank    = bank;
        input_handlers[i].address = pc;
    } else {
        // Removed the callback handler from the vector.
        UINT8 i, j;
        for (i = 0; i != input_handler_count; ++i) {
            if (EVENT_HANDLER_TYPE(input_handlers[i].options) == EVENT_HANDLER_TYPE(opt)) {
                for (j = i; j != input_handler_count - 1; ++j) {
                    input_handlers[j].options = input_handlers[j + 1].options;
                    input_handlers[j].bank    = input_handlers[j + 1].bank;
                    input_handlers[j].address = input_handlers[j + 1].address;
                }
                input_handlers[j].options = INPUT_HANDLER_INVALID;
                input_handlers[j].bank    = 0;
                input_handlers[j].address = NULL;
                if (--input_handler_count == 0)
                    input_handler_cursor = INPUT_HANDLER_NONE;

                return;
            }
        }
    }
}
