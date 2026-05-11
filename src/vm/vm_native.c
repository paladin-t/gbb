#pragma bank 255

#if defined __SDCC
#   include <gbdk/console.h>
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include <string.h>

#include "utils/sgb.h"
#include "utils/utils.h"

#include "vm_audio.h"
#include "vm_game.h"
#include "vm_input.h"
#include "vm_native.h"

BANKREF(VM_NATIVE)

// Gets the value at the specific banked memory address.
BOOLEAN peek_banked(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)start; (void)stack_frame;

    SCRIPT_CTX * THIS_ = (SCRIPT_CTX *)THIS;

    const UINT8 bank   = (UINT8)*(--THIS_->stack_ptr);
    const UINT16 addr  = (UINT16)*(--THIS_->stack_ptr);
    const BOOLEAN word = (BOOLEAN)*(--THIS_->stack_ptr);

    const UINT16 val = word ?
        get_uint16(bank, (UINT8 *)addr) :
        get_uint8 (bank, (UINT8 *)addr);
    *(THIS_->stack_ptr++) = val; // Return the result.

    return TRUE;
}

// Clears the screen for the text mode.
BOOLEAN clear_text(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)THIS; (void)start; (void)stack_frame;

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

    if (device_type & DEVICE_TYPE_WITH_TOUCH_SUPPORT) { // Ignore touch handling if touch feature is not supported.
        if (INPUT_IS_TOUCH_UP & TOUCH_BUTTON_0) {
            input_touch_state = 0;

            return TRUE;
        }
        if (!FEATURE_AUTO_UPDATE_ENABLED /* && VM_IS_LOCKED */) { INPUT_ACCEPT_TOUCH; }
    }

    ((SCRIPT_CTX *)THIS)->waitable = TRUE; // No input, wait.

    return FALSE;
}

// Rumbles the cartridge.
BOOLEAN rumble(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)THIS;
    (void)start;
    (void)stack_frame;

    // TODO

    return FALSE;
}

// Sends a packet of bytes to SGB devices.
BOOLEAN send_sgb_packet(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)THIS; (void)start;

    const UINT8 bank  = (UINT8) stack_frame[2];
    const UINT16 addr = (UINT16)stack_frame[1];
    const UINT8 size  = (UINT8) stack_frame[0];

    sgb_send_packet(bank, (UINT8 *)addr, size);

    return TRUE;
}

// Sets border frame for SGB devices.
BOOLEAN set_sgb_border(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)THIS; (void)start;

    const UINT8 palette_bank   = (UINT8) stack_frame[8];
    const UINT16 palette       = (UINT16)stack_frame[7];
    const UINT16 palette_size  = (UINT16)stack_frame[6];
    const UINT8 tiledata_bank  = (UINT8) stack_frame[5];
    const UINT16 tiledata      = (UINT16)stack_frame[4];
    const UINT16 tiledata_size = (UINT16)stack_frame[3];
    const UINT8 tilemap_bank   = (UINT8) stack_frame[2];
    const UINT16 tilemap       = (UINT16)stack_frame[1];
    const UINT16 tilemap_size  = (UINT16)stack_frame[0];

    sgb_set_border(
        palette_bank, (const UINT8 *)palette, palette_size,
        tiledata_bank, (const UINT8 *)tiledata, tiledata_size,
        tilemap_bank, (const UINT8 *)tilemap, tilemap_size
    );

    return TRUE;
}

#if USE_SGB_MOUSE
// Gets whether an SGB mouse has been installed.
BOOLEAN is_sgb_mouse_installed(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)THIS;

    SCRIPT_CTX * THIS_ = (SCRIPT_CTX *)THIS;

    BOOLEAN ret = FALSE;
    if (device_type & DEVICE_TYPE_SGB) { // Use SGB features if available.
        joypad_ex(&joypads);
        ret =
            (joypads.npads == 4) &&
            ((joypads.joy3 & SNES_MOUSE_IS_CONNECTED_MASK) == SNES_MOUSE_IS_CONNECTED);
    }
    *(THIS_->stack_ptr++) = ret; // Return the result.

    return TRUE;
}
#endif /* USE_SGB_MOUSE */

#if USE_SPEECH
// Sets the options of the speech synthesizer module.
BOOLEAN tune(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)THIS; (void)start;

    const UINT8 volume = (UINT8) stack_frame[2];
    const UINT8 speed  = (UINT8) stack_frame[1];
    const UINT16 pitch = (UINT16)stack_frame[0];

    speech_set_volume(volume);
    speech_set_speed(speed);
    speech_set_pitch(pitch);

    return TRUE;
}

// Says something with the speech synthesizer module, installs a necessary ISR.
BOOLEAN say(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)start; (void)stack_frame;

    SCRIPT_CTX * THIS_ = (SCRIPT_CTX *)THIS;

    const UINT8 bank  = THIS_->bank;
    const UINT8 * pc  = THIS_->PC;
    const UINT16 len  = get_uint16(bank, (UINT8 *)pc);
    const UINT8 * str = pc + sizeof(UINT16);

    audio_play_speech(bank, str, len);

    THIS_->PC += sizeof(len) + len;

    return TRUE;
}

// Stops speech playback, and uninstalls its ISR.
BOOLEAN hush(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)THIS; (void)start; (void)stack_frame;

    audio_hush_speech();

    return TRUE;
}
#endif /* USE_SPEECH */

// Triggers an error.
BOOLEAN error(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)THIS; (void)start; (void)stack_frame;

#if defined __SDCC && defined NINTENDO
__asm
        rst 0x38
__endasm;
#else /* __SDCC && NINTENDO */
#   error "Not implemented."
#endif /* __SDCC && NINTENDO */

    return TRUE;
}
