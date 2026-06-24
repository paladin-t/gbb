#ifndef __VM_SCENE_TRANSFER_H__
#define __VM_SCENE_TRANSFER_H__

#include "vm.h"

#ifndef USE_SCENE_TRANSFER
#   define USE_SCENE_TRANSFER 0
#endif /* USE_SCENE_TRANSFER */

#if USE_SCENE_TRANSFER

BANKREF_EXTERN(VM_SCENE_TRANSFER)

BOOLEAN blit_scene(POINTER THIS, UINT8 start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.

BOOLEAN transition_scene(POINTER THIS, UINT8 start, UINT16 * stack_frame) OLDCALL BANKED; // INVOKABLE.

#endif /* USE_SCENE_TRANSFER */

#endif /* __VM_SCENE_TRANSFER_H__ */
