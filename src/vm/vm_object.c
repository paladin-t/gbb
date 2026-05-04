#pragma bank 255

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "vm_actor.h"
#include "vm_projectile.h"

BANKREF(VM_OBJECT)

void vm_object_is(SCRIPT_CTX * THIS, UINT8 type) OLDCALL BANKED {
    const UINT16 ptr = (UINT16)*(--THIS->stack_ptr);
    switch (type) {
    case OBJECT_TYPE_ACTOR:
        *(THIS->stack_ptr++) = ptr >= (UINT16)(&actors[0]) && ptr <= (UINT16)(&actors[ACTOR_MAX_COUNT - 1]);

        break;
    case OBJECT_TYPE_PROJECTILE:
#if USE_PROJECTILE
        *(THIS->stack_ptr++) = ptr >= (UINT16)(&projectiles[0]) && ptr <= (UINT16)(&projectiles[PROJECTILE_MAX_COUNT - 1]);
#else /* USE_PROJECTILE */
        *(THIS->stack_ptr++) = FALSE;
#endif /* USE_PROJECTILE */

        break;
    default:
        *(THIS->stack_ptr++) = FALSE;

        break;
    }
}
