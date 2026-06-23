#ifndef __VM_NATIVE_H__
#define __VM_NATIVE_H__

#if defined __SDCC
#   include <gbdk/platform.h>
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "drv/mouse/sgb_mouse.h"
#include "drv/speech/speech.h"

#include "vm.h"

#ifndef USE_KEYBOARD_FUNCTIONS
#   define USE_KEYBOARD_FUNCTIONS 1
#endif /* USE_KEYBOARD_FUNCTIONS */
#ifndef USE_RUMBLE_FUNCTIONS
#   define USE_RUMBLE_FUNCTIONS 1
#endif /* USE_RUMBLE_FUNCTIONS */
#ifndef USE_SGB_FUNCTIONS
#   define USE_SGB_FUNCTIONS 1
#endif /* USE_SGB_FUNCTIONS */

BANKREF_EXTERN(VM_NATIVE)

BOOLEAN peek_banked(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.

BOOLEAN clear_text(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.

BOOLEAN wait_for(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.
BOOLEAN wait_until_confirm(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.
#if USE_KEYBOARD_FUNCTIONS
BOOLEAN wait_for_key_code(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.
BOOLEAN wait_for_key_ascii(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.
#endif /* USE_KEYBOARD_FUNCTIONS */

#if USE_RUMBLE_FUNCTIONS
BOOLEAN rumble(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.
#endif /* USE_RUMBLE_FUNCTIONS */

#if USE_SGB_FUNCTIONS
BOOLEAN send_sgb_packet(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.
BOOLEAN set_sgb_border(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.
#endif /* USE_SGB_FUNCTIONS */

#if USE_SGB_MOUSE
BOOLEAN is_sgb_mouse_installed(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.
#endif /* USE_SGB_MOUSE */

#if USE_SPEECH
BOOLEAN tune(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.
BOOLEAN say(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.
BOOLEAN hush(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.
#endif /* USE_SPEECH */

BOOLEAN error(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.

#endif /* __VM_NATIVE_H__ */
