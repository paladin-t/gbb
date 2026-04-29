#ifndef __VM_NATIVE_H__
#define __VM_NATIVE_H__

#if defined __SDCC
#   include <gbdk/platform.h>
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "vm.h"

BANKREF_EXTERN(VM_NATIVE)

BOOLEAN peek_banked(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.

BOOLEAN clear_text(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.

BOOLEAN wait_for(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.
BOOLEAN wait_until_confirm(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.

BOOLEAN send_sgb_packet(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.
BOOLEAN set_sgb_border(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.

#if defined USE_SPEECH
BOOLEAN tune(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.
BOOLEAN say(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.
BOOLEAN hush(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.
#endif /* USE_SPEECH */

BOOLEAN error(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.

#endif /* __VM_NATIVE_H__ */
