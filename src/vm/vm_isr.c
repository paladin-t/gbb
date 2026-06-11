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
        ld a, #0xFF                 ; Can be overwritten by compiler. A = bank.
        ld e, a                     ; E = bank.
        ld hl, #0xFFFF              ; Can be overwritten by compiler. HL = fn.
        jp ___sdcc_bcall_ehl        ; Call E:HL.
__endasm;
#else /* __SDCC && NINTENDO */
#   error "Not implemented."
#endif /* __SDCC && NINTENDO */
}

void isr_lcd(void) NONBANKED NAKED {
#if defined __SDCC && defined NINTENDO
__asm
        ld a, #0xFF                 ; Can be overwritten by compiler. A = bank.
        ld e, a                     ; E = bank.
        ld hl, #0xFFFF              ; Can be overwritten by compiler. HL = fn.
        jp ___sdcc_bcall_ehl        ; Call E:HL.
__endasm;
#else /* __SDCC && NINTENDO */
#   error "Not implemented."
#endif /* __SDCC && NINTENDO */
}

// Enables the overridable VBL isr.
BOOLEAN enable_vbl_isr(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)THIS; (void)start; (void)stack_frame;

    CRITICAL {
        add_VBL(isr_vbl);
    }

    return TRUE;
}

// Disables the overridable VBL isr.
BOOLEAN disable_vbl_isr(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)THIS; (void)start; (void)stack_frame;

    CRITICAL {
        remove_VBL(isr_vbl);
    }

    return TRUE;
}

// Enables the overridable LCD isr.
BOOLEAN enable_lcd_isr(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)THIS; (void)start; (void)stack_frame;

    CRITICAL {
        add_LCD(isr_lcd);
    }

    return TRUE;
}

// Disables the overridable LCD isr.
BOOLEAN disable_lcd_isr(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)THIS; (void)start; (void)stack_frame;

    CRITICAL {
        remove_LCD(isr_lcd);
    }

    return TRUE;
}

#endif /* USE_ISR */
