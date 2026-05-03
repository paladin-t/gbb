#ifndef __VM_PROJECTILE_H__
#define __VM_PROJECTILE_H__

#if defined __SDCC
#   include <gb/metasprites.h>
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include <stdbool.h>

#include "utils/utils.h"

#include "vm.h"

BANKREF_EXTERN(VM_PROJECTILE)

#ifndef PROJECTILE_DEF_MAX_COUNT
#   define PROJECTILE_DEF_MAX_COUNT         5
#endif /* PROJECTILE_DEF_MAX_COUNT */
#ifndef PROJECTILE_MAX_COUNT
#   define PROJECTILE_MAX_COUNT             5
#endif /* PROJECTILE_MAX_COUNT */

#define PROJECTILE_MAX_ANIMATIONS           4

#define PROJECTILE_FLAG_NONE                0x00
#define PROJECTILE_FLAG_ANIMATION_NO_LOOP   0x01
#define PROJECTILE_FLAG_STRONG              0x02

typedef struct projectile_def_t {
    // Properties.
    boundingbox_t bounds;
    // Resources.
    UINT8 base_tile;
    UINT8 sprite_bank;
    metasprite_t ** sprite_frames;
    UINT8 animation_interval;
    animation_t animations[PROJECTILE_MAX_ANIMATIONS];
    // Behaviour.
    UINT8 life_time;
    UINT8 move_speed;
    UINT16 initial_offset;
    // Collision.
    UINT8 collision_group;
} projectile_def_t;

typedef struct projectile_t {
    // Flags.
    bool animation_no_loop   : 1;
    bool strong              : 1;
    bool reserved1           : 1;
    bool reserved2           : 1;
    bool reserved3           : 1;
    bool reserved4           : 1;
    bool reserved5           : 1;
    bool reserved6           : 1;
    // Properties.
    upoint16_t position;
    point16_t movement;
    // Resources.
    UINT8 frame;
    UINT8 animation;
    // Definition.
    projectile_def_t def;
    // Linked list.
    struct projectile_t * next;
} projectile_t;

extern projectile_def_t projectile_defs[PROJECTILE_DEF_MAX_COUNT];
extern projectile_t projectiles[PROJECTILE_MAX_COUNT];
extern projectile_t * projectile_active_head;

void projectile_init(void) BANKED;
void projectile_update(void) BANKED; // UPDATE.
void projectile_render(void) BANKED; // UPDATE.

UINT8 projectile_active_count(void) BANKED;
UINT8 projectile_free_count(void) BANKED;

UINT8 projectile_def(UINT8 type, UINT8 base_tile, UINT8 bank, UINT8 * ptr) BANKED;
projectile_t * projectile_start(UINT8 index, UINT16 x, UINT16 y, UINT8 angle, UINT8 flags) BANKED;

void vm_def_projectile(SCRIPT_CTX * THIS, UINT8 src) OLDCALL BANKED;
void vm_start_projectile(SCRIPT_CTX * THIS, UINT8 op) OLDCALL BANKED;
void vm_get_projectile_prop(SCRIPT_CTX * THIS) OLDCALL BANKED; // Get projectile property.
void vm_set_projectile_prop(SCRIPT_CTX * THIS, UINT8 src) OLDCALL BANKED; // Set projectile property.

#endif /* __VM_PROJECTILE_H__ */
