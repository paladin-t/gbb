#ifndef __SGB_MOUSE_H__
#define __SGB_MOUSE_H__

#if defined __SDCC
#   include <gbdk/platform.h>
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include <stdbool.h>

#if defined USE_SGB_MOUSE

// In SGB Player 2.
#define SNES_MOUSE_X_DIR               0b10000000u // .7: 1=left, 0=right, 6..0: movement.
#define SNES_MOUSE_X_MASK              0b01111111u

// In SGB Player 3.
#define SNES_MOUSE_Y_DIR               0b10000000u // .7: 1=up, 0=down, 6..0: movement.
#define SNES_MOUSE_Y_MASK              0b01111111u

// Status bits in SGB Player 4.
#define SNES_MOUSE_BUTTON_LEFT         0b00000001u
#define SNES_MOUSE_BUTTON_RIGHT        0b00000010u
#define SNES_MOUSE_SGB_MENU_OPEN       0b00000100u
#define SNES_MOUSE_IS_CONNECTED        0b10100000u
#define SNES_MOUSE_IS_CONNECTED_MASK   0b11110000u
#define SNES_MOUSE_BUTTON_BOTH        (SNES_MOUSE_BUTTON_LEFT | SNES_MOUSE_BUTTON_RIGHT)
#define SNES_MOUSE_BUTTON_MASK        (SNES_MOUSE_BUTTON_BOTH)

#define MOUSE_BUTTON_LEFT              0x01u
#define MOUSE_BUTTON_RIGHT             0x02u
#define MOUSE_BUTTON_MASK             (MOUSE_BUTTON_LEFT | MOUSE_BUTTON_RIGHT)

extern joypads_t joypads;
extern INT8 mouse_x_move;
extern INT8 mouse_y_move;
extern UINT8 mouse_buttons;

void sgb_mouse_install(void) BANKED;
BOOLEAN sgb_mouse_input_update(void) BANKED;

#endif /* USE_SGB_MOUSE */

#endif /* __SGB_MOUSE_H__ */
