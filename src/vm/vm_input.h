#ifndef __VM_INPUT_H__
#define __VM_INPUT_H__

#include "vm.h"
#include "vm_device.h"

BANKREF_EXTERN(VM_INPUT)

#if defined USE_SGB_MOUSE
#   define INPUT_JOYPAD             joypads.joy0
#   define INPUT_TOUCH_X            input_touch_position_x
#   define INPUT_TOUCH_Y            input_touch_position_y
#   define INPUT_TOUCH_PRESSED      input_touch_pressed
#else /* USE_SGB_MOUSE */
#   define INPUT_JOYPAD             joypad()
#   define INPUT_TOUCH_X            (*(UINT8 *)TOUCH_X_REG)
#   define INPUT_TOUCH_Y            (*(UINT8 *)TOUCH_Y_REG)
#   define INPUT_TOUCH_PRESSED      (*(UINT8 *)TOUCH_PRESSED_REG)
#endif /* USE_SGB_MOUSE */

#define INPUT_IS_BTN_PRESSED(B)     (input_button_pressed & (B))
#define INPUT_IS_BTN_DOWN(B)        ((input_button_pressed & (B)) & ~(input_button_previous & (B)))
#define INPUT_IS_BTN_UP(B)          (~(input_button_pressed & (B)) & (input_button_previous & (B)))
#define INPUT_IS_BTN_HOLD(B)        ((input_button_pressed & (B)) & (input_button_previous & (B)))
#define INPUT_ACCEPT_BTN            do { input_button_previous = input_button_pressed; input_button_pressed = INPUT_JOYPAD; } while (0)
#define INPUT_CLEAR_BTN(B)          do { input_button_pressed &= ~(B); input_button_previous &= ~(B); } while (0)

#define INPUT_IS_TOUCH_PRESSED      (input_touch_state & 0x0F)
#define INPUT_IS_TOUCH_DOWN         ((input_touch_state & 0x0F) & ~(input_touch_state >> 4))
#define INPUT_IS_TOUCH_UP           (~(input_touch_state & 0x0F) & (input_touch_state >> 4))
#define INPUT_ACCEPT_TOUCH          do { input_touch_state = (input_touch_state << 4) | INPUT_TOUCH_PRESSED; } while (0)

#define INPUT_HANDLER_BTN_UP        0
#define INPUT_HANDLER_BTN_DOWN      1
#define INPUT_HANDLER_BTN_LEFT      2
#define INPUT_HANDLER_BTN_RIGHT     3
#define INPUT_HANDLER_BTN_A         4
#define INPUT_HANDLER_BTN_B         5
#define INPUT_HANDLER_BTN_SELECT    6
#define INPUT_HANDLER_BTN_START     7
#define INPUT_HANDLER_BTN_ANY       8
#define INPUT_HANDLER_BTND_UP       9
#define INPUT_HANDLER_BTND_DOWN     10
#define INPUT_HANDLER_BTND_LEFT     11
#define INPUT_HANDLER_BTND_RIGHT    12
#define INPUT_HANDLER_BTND_A        13
#define INPUT_HANDLER_BTND_B        14
#define INPUT_HANDLER_BTND_SELECT   15
#define INPUT_HANDLER_BTND_START    16
#define INPUT_HANDLER_BTND_ANY      17
#define INPUT_HANDLER_BTNU_UP       18
#define INPUT_HANDLER_BTNU_DOWN     19
#define INPUT_HANDLER_BTNU_LEFT     20
#define INPUT_HANDLER_BTNU_RIGHT    21
#define INPUT_HANDLER_BTNU_A        22
#define INPUT_HANDLER_BTNU_B        23
#define INPUT_HANDLER_BTNU_SELECT   24
#define INPUT_HANDLER_BTNU_START    25
#define INPUT_HANDLER_BTNU_ANY      26
#define INPUT_HANDLER_TOUCH         27
#define INPUT_HANDLER_TOUCHD        28
#define INPUT_HANDLER_TOUCHU        29
#define INPUT_HANDLER_COUNT         30
#define INPUT_HANDLER_NONE          INPUT_HANDLER_COUNT
#define INPUT_HANDLER_INVALID       0xFF

typedef struct input_handler_t {
    UINT8 options;
    UINT8 bank;
    UINT8 * address;
    UINT16 thread_id;
} input_handler_t;

extern UINT8 input_button_pressed;
extern UINT8 input_button_previous;
extern UINT8 input_touch_state;
extern input_handler_t input_handlers[INPUT_HANDLER_COUNT];
extern UINT8 input_handler_count;
extern UINT8 input_handler_cursor;

#if defined USE_SGB_MOUSE
extern joypads_t joypads; // Defined in the SGB mouse module.
extern UINT8 input_touch_position_x;
extern UINT8 input_touch_position_y;
extern UINT8 input_touch_pressed;
#endif /* USE_SGB_MOUSE */

void input_init(void) BANKED;

void vm_btn(SCRIPT_CTX * THIS) OLDCALL BANKED;
void vm_btnd(SCRIPT_CTX * THIS) OLDCALL BANKED;
void vm_btnu(SCRIPT_CTX * THIS) OLDCALL BANKED;
void vm_touch(SCRIPT_CTX * THIS, BOOLEAN loc, INT16 idxX, INT16 idxY) OLDCALL BANKED; // GBB EXTENSION.
void vm_touchd(SCRIPT_CTX * THIS, BOOLEAN loc, INT16 idxX, INT16 idxY) OLDCALL BANKED; // GBB EXTENSION.
void vm_touchu(SCRIPT_CTX * THIS, BOOLEAN loc, INT16 idxX, INT16 idxY) OLDCALL BANKED; // GBB EXTENSION.
void vm_on_input(SCRIPT_CTX * THIS, UINT8 bank, UINT8 * pc, UINT8 opt) OLDCALL BANKED; // Gosub/goto/start.

#endif /* __VM_INPUT_H__ */
