#pragma bank 255

#if defined __SDCC
#   include <gbdk/console.h>
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "utils/utils.h"

#include "vm_device.h"
#include "vm_game.h"
#include "vm_input.h"
#include "vm_native.h"

BANKREF(VM_NATIVE)

BOOLEAN peek_banked(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)start;
    (void)stack_frame;

    SCRIPT_CTX * THIS_ = (SCRIPT_CTX *)THIS;
    const UINT8 bank   = (UINT8)*(--THIS_->stack_ptr);
    const UINT16 addr  = (UINT16)*(--THIS_->stack_ptr);
    const BOOLEAN word = (BOOLEAN)*(--THIS_->stack_ptr);

    const UINT16 val = word ?
        get_uint16(bank, (UINT8 *)addr) :
        get_uint8 (bank, (UINT8 *)addr);
    *(THIS_->stack_ptr++) = val;

    return TRUE;
}

// Clears the screen for the text mode.
BOOLEAN clear_text(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)THIS;
    (void)start;
    (void)stack_frame;

    cls();

    return TRUE;
}

// Waits for the specific number of frames.
BOOLEAN wait_for(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    // Allocate one local variable (just write ahead of VM stack pointer, we
    // have no interrupts, our local variables won't get spoiled).
    if (start) *((SCRIPT_CTX *)THIS)->stack_ptr = stack_frame[0] + 1;

    // Check the wait condition.
    return ((--*((SCRIPT_CTX *)THIS)->stack_ptr) != 0) ? ((SCRIPT_CTX *)THIS)->waitable = TRUE, (BOOLEAN)FALSE : (BOOLEAN)TRUE;
}

// Waits until the A button has been pressed or anywhere of the screen has been tapped.
BOOLEAN wait_until_confirm(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)stack_frame;

    if (start) {
        input_button_pressed = input_button_previous = 0;
        input_touch_state = 0;
    }

    if (INPUT_IS_BTN_UP(J_A | J_START)) {
        input_button_pressed = input_button_previous = 0;

        return TRUE;
    }
    if (!FEATURE_AUTO_UPDATE_ENABLED /* && VM_IS_LOCKED */) { INPUT_ACCEPT_BTN; }

    if (device_type & DEVICE_TYPE_GBB) { // Ignore touch handling if extension features are not supported.
        if (INPUT_IS_TOUCH_UP & TOUCH_BUTTON_0) {
            input_touch_state = 0;

            return TRUE;
        }
        if (!FEATURE_AUTO_UPDATE_ENABLED /* && VM_IS_LOCKED */) { INPUT_ACCEPT_TOUCH; }
    }

    ((SCRIPT_CTX *)THIS)->waitable = TRUE; // No input, wait.

    return FALSE;
}

// Triggers an error.
BOOLEAN error(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)THIS;
    (void)start;
    (void)stack_frame;

#if defined __SDCC && defined NINTENDO
__asm
        rst 0x38
__endasm;
#else /* __SDCC && NINTENDO */
#   error "Not implemented."
#endif /* __SDCC && NINTENDO */

    return TRUE;
}
