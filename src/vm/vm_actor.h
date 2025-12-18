#ifndef __VM_ACTOR_H__
#define __VM_ACTOR_H__

#if defined __SDCC
#   include <gb/metasprites.h>
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include <stdbool.h>

#include "utils/linked_list.h"
#include "utils/utils.h"

#include "vm.h"

BANKREF_EXTERN(VM_ACTOR)

#define ACTOR_IMPLEMENT_MOVE_WITH_FUNCTION_ENABLED         1
#define ACTOR_IMPLEMENT_AUTO_ALIGNING_ON_BLOCKED_ENABLED   1
#define ACTOR_IMPLEMENT_AUTO_ALIGNING_WITH_TURN_ENABLED    1

#define ACTOR_MAX_COUNT                                    21
#define ACTOR_MAX_ANIMATIONS                               8

#define ACTOR_TEMPLATE_NONE                                0xFF
#define ACTOR_TEMPLATE_ANY                                 ACTOR_TEMPLATE_NONE

#define ACTOR_MOTION_MOVE                                  0x0F
#   define ACTOR_MOTION_MOVE_X                             0x03
#       define ACTOR_MOTION_MOVE_LEFT                      0x01
#       define ACTOR_MOTION_MOVE_RIGHT                     0x02
#   define ACTOR_MOTION_MOVE_Y                             0x0C
#       define ACTOR_MOTION_MOVE_UP                        0x04
#       define ACTOR_MOTION_MOVE_DOWN                      0x08
#define ACTOR_MOTION_RELATIVE                              0x40
#define ACTOR_MOTION_NO_BLOCKING                           0x80

#define ACTOR_DEACTIVE_DISTANCE                            2
#define ACTOR_DEFAULT_MOVE_SPEED                           FROM_SCREEN(1)

#define ACTOR_ANIMATION_INTERVAL                           15
#define ACTOR_ANIMATION_PAUSED                             255
#define ACTOR_ANIMATION_DEFAULT_INDEX                      0
#define ACTOR_ANIMATION_TO_IDLE_INDEX                      16
#define ACTOR_ANIMATION_TO_MOVING_INDEX                    17

#define ACTOR_COLLIDED_COOLDOWN_FRAMES                     20

#define ACTOR_DELTA(ACTOR, DM, C)                          ((DM) == 0 ? (ACTOR)->position.C : (ACTOR)->position.C + (DM) * MUL2((ACTOR)->move_speed) /* A little bit ahead. */)

#define ACTOR_DL_PUSH_HEAD(HEAD, TAIL, ITEM) \
    (ITEM)->prev = NULL; \
    (ITEM)->next = (HEAD); \
    if (HEAD) { \
        (HEAD)->prev = (ITEM); \
    } \
    (HEAD) = (ITEM); \
    if (!(TAIL)) { \
        (TAIL) = ITEM; \
    }
#define ACTOR_DL_PUSH_TAIL(HEAD, TAIL, ITEM) \
    (ITEM)->prev = (TAIL); \
    (ITEM)->next = NULL; \
    if (TAIL) { \
        (TAIL)->next = (ITEM); \
    } \
    (TAIL) = (ITEM); \
    if (!(HEAD)) { \
        (HEAD) = ITEM; \
    }
#define ACTOR_DL_REMOVE_ITEM(HEAD, TAIL, ITEM) \
    if ((TAIL) == (ITEM)) { \
        (TAIL) = (ITEM)->prev; \
    } \
    if ((ITEM)->next && (ITEM)->prev) { \
        (ITEM)->prev->next = (ITEM)->next; \
        (ITEM)->next->prev = (ITEM)->prev; \
    } else if ((ITEM)->next) { \
        (ITEM)->next->prev = NULL; \
        (HEAD) = (ITEM)->next; \
    } else if ((ITEM)->prev) { \
        (ITEM)->prev->next = NULL; \
    } else { \
        (HEAD) = NULL; \
    }

typedef struct actor_t {
    // Flags.
    bool instantiated         : 1;     // Whether the actor is instantiated.
    bool active               : 1;     // Whether the actor is active.
    bool enabled              : 1;     // Whether the actor is enabled for refreshing when scene scrolls.
    bool hidden               : 1;     // Whether the actor is hidden or visible.
    bool pinned               : 1;     // Whether the actor is pinned on screen.
    bool persistent           : 1;     // Whether the actor is persistent in scene.
    bool animation_loop       : 1;
    bool movement_interrupt   : 1;
    // Properties.
    UINT8 template;                    // Index of the actor asset.
    upoint16_t position;
    UINT8 direction;
    boundingbox_t bounds;
    // Resources.
    UINT8 base_tile;                   // The base tile index of the resource.
    UINT8 sprite_bank;
    metasprite_t ** sprite_frames;
    UINT8 animation;
    UINT8 animation_interval;
    animation_t animations[ACTOR_MAX_ANIMATIONS];
    UINT8 frame;
    // Behaviour.
    UINT8 motion;                      // For internal motion flags.
    UINT8 move_speed;
    union {
        UINT32 movement;               // Dummy.
        upoint16_t absolute_movement;
        struct {
            point8_t relative_movement;
            UINT8 original_move_speed;
            UINT8 max_move_speed;
        };
    };
    UINT8 behaviour;
    // Collision.
    UINT8 collision_group;
    // Callback.
    UINT16 behave_thread_id;
    UINT8 behave_handler_bank;
    UINT8 * behave_handler_address;
    UINT16 hit_thread_id;
    UINT8 hit_handler_bank;
    UINT8 * hit_handler_address;
    // Linked list.
    struct actor_t * next;
    struct actor_t * prev;
} actor_t;

extern actor_t actors[ACTOR_MAX_COUNT];
extern actor_t * actor_active_head; // Contains instantiated and active.
extern actor_t * actor_active_tail;
extern actor_t * actor_inactive_head; // Contains instantiated and inactive.
extern actor_t * actor_inactive_tail;
extern UINT8 actor_hardware_sprite_count;
extern actor_t * actor_following_target;
extern actor_t * actor_collided_a;
extern actor_t * actor_collided_b;
extern UINT8 actor_collided_counter;

void actor_init(void) BANKED;
void actor_update(void) BANKED; // UPDATE.

actor_t * actor_new(void) BANKED;
void actor_delete(actor_t * actor) BANKED;
void actor_activate(actor_t * actor) BANKED;
void actor_deactivate(actor_t * actor) BANKED;
UINT8 actor_instantiated_count(void) BANKED;
UINT8 actor_free_count(void) BANKED;
UINT8 actor_active_count(void) BANKED;

void actor_ctor(actor_t * actor) BANKED;
UINT8 actor_def(actor_t * actor, UINT16 x, UINT16 y, UINT8 base_tile, UINT8 bank, UINT8 * ptr) BANKED;
BOOLEAN actor_move(actor_t * actor, UINT16 x, UINT16 y) BANKED;

INLINE void actor_move_to_point(actor_t * actor, UINT16 x, UINT16 y) {
    actor->movement_interrupt = FALSE;
    actor->motion = 0;

    SET_FLAG(actor->motion, ACTOR_MOTION_NO_BLOCKING);
    if (x != actor->position.x) {
        actor->absolute_movement.x = x;
        SET_FLAG(
            actor->motion,
            x < actor->position.x ?
                ACTOR_MOTION_MOVE_LEFT : ACTOR_MOTION_MOVE_RIGHT
        );
    }
    if (y != actor->position.y) {
        actor->absolute_movement.y = y;
        SET_FLAG(
            actor->motion,
            y < actor->position.y ?
                ACTOR_MOTION_MOVE_UP : ACTOR_MOTION_MOVE_DOWN
        );
    }
}
INLINE void actor_move_in_direction(actor_t * actor, INT8 dx, INT8 dy) {
    actor->movement_interrupt = FALSE;
    actor->motion = 0;

    SET_FLAG(actor->motion, ACTOR_MOTION_RELATIVE | ACTOR_MOTION_NO_BLOCKING);
    if (dx != 0) {
        actor->relative_movement.x = dx;
        SET_FLAG(
            actor->motion,
            dx < 0 ?
                ACTOR_MOTION_MOVE_LEFT : ACTOR_MOTION_MOVE_RIGHT
        );
    }
    if (dy != 0) {
        actor->relative_movement.y = dy;
        SET_FLAG(
            actor->motion,
            dy < 0 ?
                ACTOR_MOTION_MOVE_UP : ACTOR_MOTION_MOVE_DOWN
        );
    }
}
INLINE void actor_move_in_x_direction(actor_t * actor, INT8 dx) {
    actor->movement_interrupt = FALSE;
    actor->motion = 0;

    SET_FLAG(actor->motion, ACTOR_MOTION_RELATIVE | ACTOR_MOTION_NO_BLOCKING);
    actor->relative_movement.x = dx;
    SET_FLAG(
        actor->motion,
        dx < 0 ?
            ACTOR_MOTION_MOVE_LEFT : ACTOR_MOTION_MOVE_RIGHT
    );
}
INLINE void actor_move_in_y_direction(actor_t * actor, INT8 dy) {
    actor->movement_interrupt = FALSE;
    actor->motion = 0;

    SET_FLAG(actor->motion, ACTOR_MOTION_RELATIVE | ACTOR_MOTION_NO_BLOCKING);
    actor->relative_movement.y = dy;
    SET_FLAG(
        actor->motion,
        dy < 0 ?
            ACTOR_MOTION_MOVE_UP : ACTOR_MOTION_MOVE_DOWN
    );
}
void actor_move_stop(actor_t * actor) BANKED;

void actor_reserve_move_speed(actor_t * actor, UINT8 speed_up) BANKED;
void actor_restore_move_speed(actor_t * actor) BANKED;
INLINE void actor_speed_up(actor_t * actor) {
    if (actor->move_speed != actor->max_move_speed)
        ++actor->move_speed;
}
INLINE void actor_slow_down(actor_t * actor) {
    if (actor->original_move_speed == 0) return; // Not been speed up.
    if (actor->move_speed != actor->original_move_speed)
        --actor->move_speed;
}

INLINE void actor_play_animation(actor_t * actor, UINT8 dir, BOOLEAN moving) {
    const UINT8 anim = moving ? (dir + DIRECTION_COUNT) : dir;
    if (actor->animation == anim) return;
    actor->direction = dir;
    actor->animation = anim;
    actor->frame = actor->animations[anim].begin;
}
INLINE void actor_set_animation(actor_t * actor, UINT8 anim) {
    if (actor->animation == anim) return;
    actor->animation = anim;
    actor->frame = actor->animations[anim].begin;
}
INLINE void actor_pause_animation(actor_t * actor) {
    actor->animation_interval = ACTOR_ANIMATION_PAUSED;
}
INLINE void actor_transfer_animation_to_idle(actor_t * actor) {
    UINT8 anim = actor->animation;
    if (anim < DIRECTION_COUNT) return; // Is idle already.
    anim -= DIRECTION_COUNT;
    actor_set_animation(actor, anim);
}
INLINE void actor_transfer_animation_to_moving(actor_t * actor) {
    UINT8 anim = actor->animation;
    if (anim >= DIRECTION_COUNT) return; // Is moving already.
    anim += DIRECTION_COUNT;
    actor_set_animation(actor, anim);
}

void actor_activate_actors_in_row(UINT8 x, UINT8 y) BANKED;
void actor_activate_actors_in_col(UINT8 x, UINT8 y) BANKED;

actor_t * actor_in_front_of_actor(actor_t * actor, UINT8 forward, UINT8 inc_no_collision) BANKED;
actor_t * actor_hits(const boundingbox_t * bb, const upoint16_t * offset, actor_t * ignore, BOOLEAN inc_no_collision) BANKED;
actor_t * actor_hits_in_group(const boundingbox_t * bb, const upoint16_t * offset, UINT8 collision_group) BANKED;
INLINE void actor_fire_collision(actor_t * actor_a, actor_t * actor_b) {
    actor_collided_a = actor_a;
    actor_collided_b = actor_b;
}
void actor_handle_collision(void) BANKED;

BOOLEAN actor_begin_behave_thread(actor_t * actor, UINT8 bank, UINT8 * pc) BANKED;
void actor_begin_hit_thread(actor_t * actor, actor_t * other) BANKED;
void actor_terminate_thread(UINT16 * id) BANKED;
void actor_detach_thread(UINT16 * id) BANKED;
BOOLEAN actor_try_terminate_thread(UINT16 * id) BANKED;

void vm_new_actor(SCRIPT_CTX * THIS) OLDCALL BANKED;
void vm_del_actor(SCRIPT_CTX * THIS) OLDCALL BANKED;
void vm_def_actor(SCRIPT_CTX * THIS, UINT8 src) OLDCALL BANKED;
void vm_get_actor_prop(SCRIPT_CTX * THIS) OLDCALL BANKED; // Get actor property.
void vm_set_actor_prop(SCRIPT_CTX * THIS, UINT8 src) OLDCALL BANKED; // Set actor property.
void vm_play_actor(SCRIPT_CTX * THIS) OLDCALL BANKED;
void vm_thread_actor(SCRIPT_CTX * THIS, UINT8 op) OLDCALL BANKED;
void vm_move_actor(SCRIPT_CTX * THIS, UINT8 op) OLDCALL BANKED;
void vm_find_actor(SCRIPT_CTX * THIS, UINT8 opt) OLDCALL BANKED;
void vm_on_actor(SCRIPT_CTX * THIS, UINT8 bank, UINT8 * pc, UINT8 opt) OLDCALL BANKED; // Start (hits).

#endif /* __VM_ACTOR_H__ */
