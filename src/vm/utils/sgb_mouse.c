#pragma bank 255

#if defined __SDCC
#   pragma disable_warning 110
#   include <gb/sgb.h>
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include <stdint.h>

#include "sgb_mouse.h"

#if defined USE_SGB_MOUSE

#warning "Message: Compiling using SGB mouse."

#define SGB_PACKET_SIZE   16u
#define SGB_PAYLOAD_LEN  (SGB_PACKET_SIZE - 1u)

// MouseHook:
//                                   RTS
//                                   ||
// Power-up: 60 A8 74 5A 2D FA BC FC 60 5A AF 08 E7 C3 F7 1B
//
// Modified: 60 A8 74 5A 2D FA BC FC 4C 02 09 08 E7 C3 F7 1B
//                                   |------|
//                                    JMP ....
//
// https://github.com/bbbbbr/sgb-testbed/blob/a10011c71f6ed66e3f16a7c097611a9e899b3eb5/snes_src/hacks/mouseHook.s#L3
//
// .org $808
// 
// PreGBMainLoopHook:
//     jmp $902
//
//                                   DATA_SND,Addr-Low/Hi, Bank, Len,  Payload Data.....................................................
const UINT8 sgb_mouse_hook[]       = { 0x79, 0x08, 0x08, 0x00, 0x03, 0x4c, 0x02, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const UINT8 sgb_mouse_hook_reset[] = { 0x79, 0x08, 0x08, 0x00, 0x03, 0x60, 0x5A, 0xAF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// SGB Command $0F — DATA_SND
// Byte  Content
// 0     Command*8+Length    (fixed length=1)
// 1     SNES Destination Address, low
// 2     SNES Destination Address, high
// 3     SNES Destination Address, bank number
// 4     Number of bytes to write ($01-$0B)
// 5     Data Byte #1
// 6     Data Byte #2 (if any)
// 7     Data Byte #3 (if any)
// etc.

// Modified from original version which had SNES cursor overlays and sent absolute screen positions to GB
//
// MouseInner:
// https://github.com/bbbbbr/sgb-testbed/blob/a10011c71f6ed66e3f16a7c097611a9e899b3eb5/snes_src/hacks/mouseInner.s#L16
//
//                                  DATA_SND,Addr-Low/Hi, Bank, Len,  Payload Data...................................................

#define SGB_MOUSE_SUPPRESS_LR_SGB_MENU_OPEN

#ifdef SGB_MOUSE_SUPPRESS_LR_SGB_MENU_OPEN
// Very hard to tell: does this add a little bit of latency and loss of granularity?
// https://github.com/bbbbbr/sgb-snes-mouse-gbdk/commit/e92f3e580b56f6dff2b413114d8e8cdaf97daad6
const UINT8 sgb_mouse_handler_0[] = { 0x79, 0x02, 0x09, 0x00, 0x0b, 0xa9, 0x28, 0x8d, 0x45, 0x0c, 0xad, 0x39, 0x0f, 0x49, 0xff, 0x8f };
const UINT8 sgb_mouse_handler_1[] = { 0x79, 0x0d, 0x09, 0x00, 0x0b, 0x05, 0x60, 0x00, 0xad, 0x37, 0x0f, 0x49, 0xff, 0x8f, 0x06, 0x60 };
const UINT8 sgb_mouse_handler_2[] = { 0x79, 0x18, 0x09, 0x00, 0x0b, 0x00, 0xad, 0x31, 0x0f, 0x29, 0x01, 0xd0, 0x05, 0xa9, 0x00, 0x4c };
const UINT8 sgb_mouse_handler_3[] = { 0x79, 0x23, 0x09, 0x00, 0x0b, 0x3b, 0x09, 0xa9, 0xff, 0xed, 0x4f, 0x0c, 0xf0, 0x02, 0xa9, 0x04 };
const UINT8 sgb_mouse_handler_4[] = { 0x79, 0x2e, 0x09, 0x00, 0x0b, 0x8d, 0x00, 0x09, 0xad, 0x3b, 0x0f, 0x29, 0x03, 0x09, 0xa0, 0x0d };
const UINT8 sgb_mouse_handler_5[] = { 0x79, 0x39, 0x09, 0x00, 0x0b, 0x00, 0x09, 0x49, 0xff, 0x8f, 0x07, 0x60, 0x00, 0xa2, 0x00, 0xaf };
const UINT8 sgb_mouse_handler_6[] = { 0x79, 0x44, 0x09, 0x00, 0x0b, 0xdb, 0xff, 0x00, 0xf0, 0x08, 0x20, 0xa0, 0xbc, 0x68, 0x68, 0x4c };
const UINT8 sgb_mouse_handler_7[] = { 0x79, 0x4f, 0x09, 0x00, 0x0a, 0xaa, 0xba, 0x20, 0xa3, 0xbc, 0x68, 0x68, 0x4c, 0xad, 0xba, 0x00 };
#else /* SGB_MOUSE_SUPPRESS_LR_SGB_MENU_OPEN */
const UINT8 sgb_mouse_handler_0[] = { 0x79, 0x02, 0x09, 0x00, 0x0b, 0xad, 0x39, 0x0f, 0x49, 0xff, 0x8f, 0x05, 0x60, 0x00, 0xad, 0x37 };
const UINT8 sgb_mouse_handler_1[] = { 0x79, 0x0d, 0x09, 0x00, 0x0b, 0x0f, 0x49, 0xff, 0x8f, 0x06, 0x60, 0x00, 0xad, 0x31, 0x0f, 0x29 };
const UINT8 sgb_mouse_handler_2[] = { 0x79, 0x18, 0x09, 0x00, 0x0b, 0x01, 0xd0, 0x05, 0xa9, 0x00, 0x4c, 0x36, 0x09, 0xa9, 0xff, 0xed };
const UINT8 sgb_mouse_handler_3[] = { 0x79, 0x23, 0x09, 0x00, 0x0b, 0x4f, 0x0c, 0xf0, 0x02, 0xa9, 0x04, 0x8d, 0x00, 0x09, 0xad, 0x3b };
const UINT8 sgb_mouse_handler_4[] = { 0x79, 0x2e, 0x09, 0x00, 0x0b, 0x0f, 0x29, 0x03, 0x09, 0xa0, 0x0d, 0x00, 0x09, 0x49, 0xff, 0x8f };
const UINT8 sgb_mouse_handler_5[] = { 0x79, 0x39, 0x09, 0x00, 0x0b, 0x07, 0x60, 0x00, 0xa2, 0x00, 0xaf, 0xdb, 0xff, 0x00, 0xf0, 0x08 };
const UINT8 sgb_mouse_handler_6[] = { 0x79, 0x44, 0x09, 0x00, 0x0b, 0x20, 0xa0, 0xbc, 0x68, 0x68, 0x4c, 0xaa, 0xba, 0x20, 0xa3, 0xbc };
const UINT8 sgb_mouse_handler_7[] = { 0x79, 0x4f, 0x09, 0x00, 0x05, 0x68, 0x68, 0x4c, 0xad, 0xba, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
#endif /* SGB_MOUSE_SUPPRESS_LR_SGB_MENU_OPEN */

joypads_t joypads;
INT8 mouse_x_move = 0;
INT8 mouse_y_move = 0;
UINT8 mouse_buttons = 0u;

// After installation the mouse data will be copied
// into P2-4 (X -> 2, Y -> 3, Buttons -> 4).
void sgb_mouse_install(void) BANKED {
    // First uninstall the hook in case it was previously running and this is
    // a soft-reset, such as pushing menu button on an EverDrive to load a new ROM.
    sgb_transfer(sgb_mouse_hook_reset);

    // Then install the mouse handler before enabling the hook.
    sgb_transfer(sgb_mouse_handler_0);
    sgb_transfer(sgb_mouse_handler_1);
    sgb_transfer(sgb_mouse_handler_2);
    sgb_transfer(sgb_mouse_handler_3);
    sgb_transfer(sgb_mouse_handler_4);
    sgb_transfer(sgb_mouse_handler_5);
    sgb_transfer(sgb_mouse_handler_6);
    sgb_transfer(sgb_mouse_handler_7);

    // Then install the hook.
    sgb_transfer(sgb_mouse_hook);
}

// Expects joypad_ex(&joypads) to have been called to poll SGB input data beforehand.
BOOLEAN sgb_mouse_input_update(void) BANKED {
    if (joypads.npads == 4) {
        // Mouse.
        if ((joypads.joy3 & SNES_MOUSE_IS_CONNECTED_MASK) == SNES_MOUSE_IS_CONNECTED) {
            // Don't use mouse data while SGB menu is open/visible
            // since it may result in unintended mouse movement and clicking.
            if (joypads.joy3 & SNES_MOUSE_SGB_MENU_OPEN)
                return false;

            // ===== RELATIVE MODE USING MOUSE IN SGB MOUSE HARDWARE FORMAT =====
            mouse_x_move = (joypads.joy1 & SNES_MOUSE_X_MASK);
            if (joypads.joy1 & SNES_MOUSE_X_DIR)
                mouse_x_move *= -1;
            mouse_y_move = (joypads.joy2 & SNES_MOUSE_Y_MASK);
            if (joypads.joy2 & SNES_MOUSE_Y_DIR)
                mouse_y_move *= -1;

            mouse_buttons = 0;
            if (joypads.joy3 & SNES_MOUSE_BUTTON_LEFT)
                mouse_buttons |= MOUSE_BUTTON_LEFT;
            if (joypads.joy3 & SNES_MOUSE_BUTTON_RIGHT)
                mouse_buttons |= MOUSE_BUTTON_RIGHT;

            return true;
        }
    }

    // Mouse not currently connected (may change if hot-plugged).
    return false;
}

#endif /* USE_SGB_MOUSE */
