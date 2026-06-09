#pragma bank 255

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "vm_isr.h"

BANKREF(VM_ISR)

#if USE_ISR

void isr_vbl(void) NONBANKED NAKED {
#if defined __SDCC && defined NINTENDO
__asm
        nop
        nop
        nop
        nop
        ret
__endasm;
#else /* __SDCC && NINTENDO */
#   error "Not implemented."
#endif /* __SDCC && NINTENDO */
}

void isr_lcd(void) NONBANKED NAKED {
#if defined __SDCC && defined NINTENDO
__asm
        nop
        nop
        nop
        nop
        ret
__endasm;
#else /* __SDCC && NINTENDO */
#   error "Not implemented."
#endif /* __SDCC && NINTENDO */
}

#endif /* USE_ISR */
