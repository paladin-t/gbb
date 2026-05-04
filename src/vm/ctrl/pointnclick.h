#ifndef __POINTNCLICK_H__
#define __POINTNCLICK_H__

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "../vm.h"
#include "../vm_actor.h"

#ifndef USE_POINTNCLICK
#   define USE_POINTNCLICK 1
#endif /* USE_POINTNCLICK */

#if USE_POINTNCLICK

BOOLEAN controller_behave_pointnclick_player(actor_t * actor, UINT8 pointing) BANKED;

#endif /* USE_POINTNCLICK */

#endif /* __POINTNCLICK_H__ */
