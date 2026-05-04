#ifndef __PLATFORMER_H__
#define __PLATFORMER_H__

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "../vm.h"
#include "../vm_actor.h"

#ifndef USE_PLATFORMER
#   define USE_PLATFORMER 1
#endif /* USE_PLATFORMER */

#if USE_PLATFORMER

BOOLEAN controller_behave_platformer_player(actor_t * actor) BANKED;

BOOLEAN controller_behave_platformer_move(actor_t * actor) BANKED;

BOOLEAN controller_behave_platformer_idle(actor_t * actor) BANKED;

#endif /* USE_PLATFORMER */

#endif /* __PLATFORMER_H__ */
