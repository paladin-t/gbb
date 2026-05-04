#ifndef __SHOOTING_H__
#define __SHOOTING_H__

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "../vm.h"
#include "../vm_actor.h"

#ifndef USE_SHOOTING
#   define USE_SHOOTING 0
#endif /* USE_SHOOTING */

#if USE_SHOOTING

BOOLEAN controller_behave_shooting_player(actor_t * actor) BANKED;
BOOLEAN controller_behave_shooting_move(actor_t * actor) BANKED;
#define controller_behave_shooting_idle controller_behave_topdown_idle // Reuse top-down idle.

#endif /* USE_SHOOTING */

#endif /* __SHOOTING_H__ */
