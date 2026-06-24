#pragma bank 255

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "vm_device.h"
#include "vm_scene.h"
#include "vm_scene_transfer.h"

#if USE_SCENE_TRANSFER

#warning "Message: Compiling with scene transfer."

BANKREF(VM_SCENE_TRANSFER)

// Makes blit of a region of scene data to the current scene.
BOOLEAN blit_scene(POINTER THIS, UINT8 start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)THIS; (void)start;

    const UINT8 x = (UINT8)stack_frame[3];
    const UINT8 y = (UINT8)stack_frame[2];
    const UINT8 w = (UINT8)stack_frame[1];
    const UINT8 h = (UINT8)stack_frame[0];

    if (scene.map_bank == 0)
        return TRUE;

    SCENE_LOAD(
        scene.map_bank, scene.map_address,
        scene.attr_bank, scene.attr_address,
        scene_map_x + x, scene_map_y + y,
        w, h,
        scene.width,
        scene.base_tile,
        set_bkg_submap
    );

    return TRUE;
}

// Transitions to another scene with scrolling effect.
BOOLEAN transition_scene(POINTER THIS, UINT8 start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)THIS; (void)start; (void)stack_frame;

    // TODO: scene transfer.

    return TRUE;
}

#endif /* USE_SCENE_TRANSFER */
