#ifndef __VM_EMOTE_H__
#define __VM_EMOTE_H__

#if defined __SDCC
#   include <gb/metasprites.h>
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "vm.h"
#include "vm_actor.h"

BANKREF_EXTERN(VM_EMOTE)

#ifndef USE_EMOTE
#   define USE_EMOTE 1
#endif /* USE_EMOTE */

#if USE_EMOTE

extern actor_t * emote_actor;

void emote_init(void) BANKED;
void emote_update(void) BANKED;

#endif /* USE_EMOTE */

void vm_emote(SCRIPT_CTX * THIS, UINT8 src) OLDCALL BANKED;

#endif /* __VM_EMOTE_H__ */
