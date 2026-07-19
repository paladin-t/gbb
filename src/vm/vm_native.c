#pragma bank 255

#if defined __SDCC
#   include <gbdk/console.h>
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include <string.h>

#include "utils/rumble.h"
#include "utils/sgb.h"
#include "utils/utils.h"

#include "vm_audio.h"
#include "vm_game.h"
#include "vm_gui.h"
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

// Waits until the A or Start button has been pressed or anywhere of the screen has been tapped.
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

#if USE_KEYBOARD_FUNCTIONS
// Waits for keyboard input, returns the key code.
BOOLEAN wait_for_key_code(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)start; (void)stack_frame;

    SCRIPT_CTX * THIS_ = (SCRIPT_CTX *)THIS;

    if(!(device_type & DEVICE_TYPE_GBB)) { // Ignore key handling if extension features are not supported.
        *(THIS_->stack_ptr++) = 0;

        return TRUE;
    }

    const UINT8 code = *(UINT8 *)KEY_CODE_REG; // Get the key code.
    if (code) { // If a key code is available.
        const UINT8 mod = *(UINT8 *)KEY_MODIFIER_FLAGS_REG; // Get the key modifiers.
        *(UINT8 *)KEY_CODE_REG = 0; // Clear the key code, and acknowledge to accept more key codes.

        *(THIS_->stack_ptr++) = ((mod << 8) | code); // Return the result.

        return TRUE;
    }

    THIS_->waitable = TRUE; // No input, wait.

    return FALSE;
}

// Waits for keyboard input, returns the key ASCII or control code.
BOOLEAN wait_for_key_ascii(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)start; (void)stack_frame;

    SCRIPT_CTX * THIS_ = (SCRIPT_CTX *)THIS;

    if(!(device_type & DEVICE_TYPE_GBB)) { // Ignore key handling if extension features are not supported.
        *(THIS_->stack_ptr++) = 0;

        return TRUE;
    }

    const UINT8 code = *(UINT8 *)KEY_CODE_REG; // Get the key code.
    if (code) { // If a key code is available.
        const UINT8 shift = *(UINT8 *)KEY_MODIFIER_FLAGS_REG & 0x02; // Get whether the Shift key is being pressed.
        *(UINT8 *)KEY_CODE_REG = 0; // Clear the key code, and acknowledge to accept more key codes.

        UINT8 ascii;
        if (code >= 4 && code <= 29) {
            ascii = code - 4;
            ascii += shift ? 'A' : 'a';
        } else if (code >= 30 && code <= 39) {
            const UINT8 NUM[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0' };
            const UINT8 NUMS[] = { '!', '@', '#', '$', '%', '^', '&', '*', '(', ')' };
            ascii = code - 30;
            ascii = shift ? NUMS[ascii] : NUM[ascii];
        } else if (code >= 40 && code <= 56) {
            const UINT8 SYM[] = { 13, 27, 8, ' ', ' ', '-', '=', '[', ']', '\\', ' ', ';', '\'', '`', ',', '.', '/' };
            const UINT8 SYMS[] = { 13, 27, 8, ' ', ' ', '_', '+', '{', '}', '|', ' ', ':', '"', '~', '<', '>', '?' };
            ascii = code - 40;
            ascii = shift ? SYMS[ascii] : SYM[ascii];
        } else if (code >= 79 && code <= 82) {
            const UINT8 DIR[] = { 0x04, 0x13, 0x18, 0x05 };
            ascii = code - 79;
            ascii = DIR[ascii];
        } else if (code >= 224 && code <= 231) {
            ascii = code;
        } else {
            ascii = 0;
        }

        *(THIS_->stack_ptr++) = ascii; // Return the result.

        return TRUE;
    }

    THIS_->waitable = TRUE; // No input, wait.

    return FALSE;
}
#endif /* USE_KEYBOARD_FUNCTIONS */

#if USE_RUMBLE_FUNCTIONS
// Rumbles the cartridge.
BOOLEAN rumble(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    SCRIPT_CTX * THIS_ = (SCRIPT_CTX *)THIS;

    if (start) {
        const UINT16 frames = stack_frame[1];
        *THIS_->stack_ptr = frames + 1;
    }

    const UINT8 mask = (UINT8)stack_frame[0];
    rumble_start(mask);

    --*THIS_->stack_ptr;
    if (*THIS_->stack_ptr != 0) {
        THIS_->waitable = TRUE;

        return FALSE;
    }

    rumble_stop();

    return TRUE;
}
#endif /* USE_RUMBLE_FUNCTIONS */

#if USE_BLIT_TEXT_FUNCTIONS
// Blits a buffer of arbitrary indices as text to the screen.
BOOLEAN blit_text(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED {
    (void)THIS; (void)start;

    const UINT8 bank       = (UINT8)   stack_frame[4];
    const UINT16 * addr    = (UINT16 *)stack_frame[3];
    const UINT8 count      = (UINT8)   stack_frame[2];
    const UINT8 font_bank  = (UINT8)   stack_frame[1];
    const UINT8 * font_ptr = (UINT8 *) stack_frame[0];

    const glyph_t * arb = (const glyph_t *)GUI_GLYPH_ARBITRARY_ADDRESS(font_ptr);
    glyph_option_t opt;
    get_chunk((UINT8 *)&opt, font_bank, font_ptr, sizeof(glyph_option_t));
    const UINT8 size = get_uint8(font_bank, (UINT8 *)font_ptr + sizeof(glyph_option_t));
    for (UINT8 i = 0; i != count; ++i, ++addr) {
        const UINT16 val = (bank == 0) ? *addr : get_uint16(bank, (UINT8 *)addr);
        glyph_t glyph;
        get_chunk((UINT8 *)&glyph, font_bank, (UINT8 *)(arb + val), sizeof(glyph_t));
        gui_blit_char(size, &glyph, &opt);
    }

    return TRUE;
}
#endif /* USE_BLIT_TEXT_FUNCTIONS */

#if USE_SGB_FUNCTIONS
// Sends a packet of bytes to SGB device.
BOOLEAN send_sgb_packet(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)THIS; (void)start;

    const UINT8 bank  = (UINT8) stack_frame[2];
    const UINT16 addr = (UINT16)stack_frame[1];
    const UINT8 size  = (UINT8) stack_frame[0];

    sgb_send_packet(bank, (UINT8 *)addr, size);

    return TRUE;
}

// Sets border frame for SGB device.
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
#endif /* USE_SGB_FUNCTIONS */

#if USE_SGB_MOUSE
// Gets whether an SGB mouse has been installed.
BOOLEAN is_sgb_mouse_installed(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)start; (void) stack_frame;

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

#if USE_ERROR_FUNCTIONS
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
#endif /* USE_ERROR_FUNCTIONS */
