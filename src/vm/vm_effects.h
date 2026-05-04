#ifndef __VM_EFFECTS_H__
#define __VM_EFFECTS_H__

#include "utils/utils.h"

#include "vm.h"

BANKREF_EXTERN(VM_EFFECTS)

#ifndef USE_EFFECTS
#   define USE_EFFECTS 1
#endif /* USE_EFFECTS */

#if USE_EFFECTS

#define EFFECTS_PULSE      1
#define EFFECTS_PARALLAX   2
#define EFFECTS_WOBBLE     3

#define EFFECTS_PARALLAX_STEP(START, END, SHIFT) \
    { \
        0, \
        (END) ? ((MUL8(END)) - 1) : 0, \
        (SHIFT) \
    }
#define EFFECTS_PARALLAX_SET(ROW, START, END, SHIFT) \
    do { \
        (ROW).scroll_x = 0; \
        (ROW).next_y = (END) ? ((MUL8(END)) - 1) : 0; \
        (ROW).shift = (SHIFT); \
    } while (0)

typedef struct parallax_row_t {
    UINT8 scroll_x;   // X scroll position for current slice.
    UINT8 next_y;     // Y position of next LYC.
    INT8 shift;       // Shift of scroll position.
} parallax_row_t;

extern UINT8 effects_pulse_bank;
extern POINTER effects_pulse_address;

extern parallax_row_t effects_parallax_rows[3];
extern parallax_row_t * effects_parallax_row;

extern UINT8 effects_wobble;

void effects_init(void) BANKED;

void effects_pulse_update(void) BANKED; // UPDATE.

void effects_parallax_sync(void) NAKED NONBANKED; // UPDATE.

void effects_wobble_sync(void) BANKED; // UPDATE.
void effects_wobble_update(void) BANKED; // UPDATE.

#endif /* USE_EFFECTS */

void vm_fx(SCRIPT_CTX * THIS) OLDCALL BANKED;

#endif /* __VM_EFFECTS_H__ */
