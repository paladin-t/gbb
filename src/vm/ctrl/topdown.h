#ifndef __TOPDOWN_H__
#define __TOPDOWN_H__

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "../vm.h"
#include "../vm_actor.h"

#ifndef USE_TOPDOWN
#   define USE_TOPDOWN 1
#endif /* USE_TOPDOWN */

#if USE_TOPDOWN

BOOLEAN controller_behave_topdown_player(actor_t * actor) BANKED;
BOOLEAN controller_behave_topdown_player_arbitrary(actor_t * actor, UINT8 rigid) BANKED;

BOOLEAN controller_behave_topdown_move(actor_t * actor) BANKED;
BOOLEAN controller_behave_topdown_move_arbitrary(actor_t * actor, UINT8 rigid) BANKED;

BOOLEAN controller_behave_topdown_idle(actor_t * actor) BANKED;

#endif /* USE_TOPDOWN */

#endif /* __TOPDOWN_H__ */
