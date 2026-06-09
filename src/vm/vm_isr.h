#ifndef __VM_ISR_H__
#define __VM_ISR_H__

#if defined __SDCC
#   include <gbdk/platform.h>
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "vm.h"

#ifndef USE_ISR
#   define USE_ISR 1
#endif /* USE_ISR */

BANKREF_EXTERN(VM_ISR)

#if USE_ISR

void isr_vbl(void) NONBANKED NAKED;
void isr_lcd(void) NONBANKED NAKED;

BOOLEAN enable_vbl_isr(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED;
BOOLEAN disable_vbl_isr(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED;
BOOLEAN enable_lcd_isr(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED;
BOOLEAN disable_lcd_isr(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED;

#endif /* USE_ISR */

#endif /* __VM_ISR_H__ */
