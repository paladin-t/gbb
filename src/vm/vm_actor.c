#pragma bank 255

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include <string.h>

#include "ctrl/controller.h"

#include "vm_actor.h"
#include "vm_device.h"
#include "vm_emote.h"
#include "vm_graphics.h"
#include "vm_scene.h"

#if !USE_PLATFORMER && !USE_TOPDOWN && !USE_POINTNCLICK && !USE_SHOOTING
#   pragma disable_warning 110
#   pragma disable_warning 126
#endif /* !USE_PLATFORMER && !USE_TOPDOWN && !USE_POINTNCLICK && !USE_SHOOTING */

BANKREF(VM_ACTOR)

#define ACTOR_THREADING_BEGIN       1
#define ACTOR_THREADING_JOIN        2
#define ACTOR_THREADING_TERMINATE   3
#define ACTOR_THREADING_WAIT        4

#define ACTOR_MOTION_IN_DIRECTION   1
#define ACTOR_MOTION_TO_POINT       2
#define ACTOR_MOTION_STOP           3

#define ACTOR_FILTER_BY_TEMPLATE    1
#define ACTOR_FILTER_BY_BEHAVIOUR   2

#define ACTOR_HANDLER_HITS          1

// Seealso: <gb/metasprites.h>.
#define ACTOR_ALLOCATED             (actor_hardware_sprite_count)
#define ACTOR_ALLOCATE(N)           (actor_hardware_sprite_count += (N))
#if ACTOR_IMPLEMENT_MOVE_WITH_FUNCTION_ENABLED
STATIC void ACTOR_MOVE(actor_t * actor) NONBANKED {
    if (!actor->sprite_bank) return;
    const UINT8 _save = CURRENT_BANK;
    SWITCH_ROM_BANK(actor->sprite_bank);
    __current_metasprite = *(actor->sprite_frames + actor->frame);
    __current_base_tile = actor->base_tile;
    __current_base_prop = 0;
    UINT8 x = TO_SCREEN(actor->position.x);
    UINT8 y = TO_SCREEN(actor->position.y);
    if (!actor->pinned) {
        x -= scene_camera_x;
        y -= scene_camera_y;
    }
    ACTOR_ALLOCATE(
        __move_metasprite(
            ACTOR_ALLOCATED,
            (y << 8) | x
        )
    );
    SWITCH_ROM_BANK(_save);
}
#else /* ACTOR_IMPLEMENT_MOVE_WITH_FUNCTION_ENABLED */
#define ACTOR_MOVE_(ACTOR) \
    do { \
        if (!(ACTOR)->sprite_bank) break; \
        __current_metasprite = (metasprite_t *)get_ptr( \
            (ACTOR)->sprite_bank, (UINT8 *)(ACTOR)->sprite_frames + \
                sizeof(metasprite_t *) * (ACTOR)->frame \
        ); \
        __current_base_tile = (ACTOR)->base_tile; \
        __current_base_prop = 0; \
        UINT8 X = TO_SCREEN((ACTOR)->position.x); \
        UINT8 Y = TO_SCREEN((ACTOR)->position.y); \
        if (!actor->pinned) { \
            X -= scene_camera_x; \
            Y -= scene_camera_y; \
        } \
        ACTOR_ALLOCATE( \
            call_b_bw( \
                ACTOR_ALLOCATED, \
                (Y << 8) | X, \
                (ACTOR)->sprite_bank, \
                (b_bw_fn)__move_metasprite \
            ) \
        ); \
    } while (0)
#endif /* ACTOR_IMPLEMENT_MOVE_WITH_FUNCTION_ENABLED */

actor_t actors[ACTOR_MAX_COUNT];
actor_t * actor_active_head;
actor_t * actor_active_tail;
actor_t * actor_inactive_head;
actor_t * actor_inactive_tail;
UINT8 actor_hardware_sprite_count;
actor_t * actor_following_target;
actor_t * actor_collided_a;
actor_t * actor_collided_b;
UINT8 actor_collided_counter;

void actor_init(void) BANKED {
    actor_active_head           = NULL;
    actor_active_tail           = NULL;
    actor_inactive_head         = NULL;
    actor_inactive_tail         = NULL;
    actor_hardware_sprite_count = device_object_sprite_base;
    actor_following_target      = NULL;
    actor_collided_a            = NULL;
    actor_collided_b            = NULL;
    actor_collided_counter      = ACTOR_COLLIDED_COOLDOWN_FRAMES;

    for (UINT8 i = 0; i != ACTOR_MAX_COUNT; ++i) {
        actor_t * actor = &actors[i];
        actor_ctor(actor);
        actor->prev = NULL;
        actor->next = NULL;
    }
}

INLINE BOOLEAN actor_is_inside_viewport(actor_t * actor) {
    const boundingbox_t box = { // In tiles.
        .left   = 0,
        .right  = (DEVICE_SCREEN_WIDTH - 1) + MUL2(ACTOR_DEACTIVE_DISTANCE),
        .top    = 0,
        .bottom = (DEVICE_SCREEN_HEIGHT - 1) + MUL2(ACTOR_DEACTIVE_DISTANCE)
    };
    const upoint16_t off = { // In tiles.
        .x      = (UINT16)(DIV8(MAX(scene_camera_x - graphics_map_x, 0))),
        .y      = (UINT16)(DIV8(MAX(scene_camera_y - graphics_map_y, 0)))
    };
#if SCENE_MOVE_OUT_OF_BOUNDS_ENABLED
    const upoint16_t pos = { // In tiles.
        .x      = (UINT16)(MAX(TO_SCREEN_TILE((INT16)actor->position.x) + ACTOR_DEACTIVE_DISTANCE - 1, 0)),
        .y      = (UINT16)(MAX(TO_SCREEN_TILE((INT16)actor->position.y) + ACTOR_DEACTIVE_DISTANCE - 1, 0))
    };
#else /* SCENE_MOVE_OUT_OF_BOUNDS_ENABLED */
    const upoint16_t pos = { // In tiles.
        .x      = (UINT16)(MAX(TO_SCREEN_TILE(actor->position.x) + ACTOR_DEACTIVE_DISTANCE - 1, 0)),
        .y      = (UINT16)(MAX(TO_SCREEN_TILE(actor->position.y) + ACTOR_DEACTIVE_DISTANCE - 1, 0))
    };
#endif /* SCENE_MOVE_OUT_OF_BOUNDS_ENABLED */

    return boundingbox_contains(&box, &off, &pos);
}

INLINE BOOLEAN actor_move_camera(actor_t * actor) {
    // Move the camera.
    BOOLEAN camera_moved = FALSE;

#if SCENE_MOVE_OUT_OF_BOUNDS_ENABLED
    INT16 x = TO_SCREEN((INT16)actor->position.x) - DIV2(DEVICE_SCREEN_PX_WIDTH);
    INT16 y = TO_SCREEN((INT16)actor->position.y) - DIV2(DEVICE_SCREEN_PX_HEIGHT) + scene_camera_offset_y;
#else /* SCENE_MOVE_OUT_OF_BOUNDS_ENABLED */
    INT16 x = TO_SCREEN(actor->position.x) - DIV2(DEVICE_SCREEN_PX_WIDTH);
    INT16 y = TO_SCREEN(actor->position.y) - DIV2(DEVICE_SCREEN_PX_HEIGHT) + scene_camera_offset_y;
#endif /* SCENE_MOVE_OUT_OF_BOUNDS_ENABLED */
    if (x < scene_camera_x - scene_camera_deadzone_x) {
        x = x + scene_camera_deadzone_x;
        camera_moved = TRUE;
    } else if (x > scene_camera_x + scene_camera_deadzone_x) {
        x = x - scene_camera_deadzone_x;
        camera_moved = TRUE;
    } else {
        x = scene_camera_x;
    }
    if (y < scene_camera_y - scene_camera_deadzone_y) {
        y = y + scene_camera_deadzone_y;
        camera_moved = TRUE;
    } else if (y > scene_camera_y + scene_camera_deadzone_y) {
        y = y - scene_camera_deadzone_y;
        camera_moved = TRUE;
    } else {
        y = scene_camera_y;
    }
    if (camera_moved) {
        if (scene.clamp_camera) {
            x = CLAMP(x, 0, SCENE_CAMERA_MAX_X);
            y = CLAMP(y, 0, SCENE_CAMERA_MAX_Y);
        }
        scene_camera(x, y);
    }

    // Finish.
    return camera_moved;
}

void actor_update(void) BANKED {
    // Update the emote actor if there's any.
#if USE_EMOTE
    if (emote_actor) {
        emote_update();
    }
#endif /* USE_EMOTE */

    // Update the actors.
    actor_t * actor = actor_active_tail;
    BOOLEAN camera_moved = FALSE;
    while (actor) {
        // Behave.
        BOOLEAN actor_camera_moved = FALSE;
        const UINT8 bhvr = actor->behaviour;
        if (
            VM_IS_LOCKED ?
                (bhvr & CONTROLLER_ALWAYS_BEHAVE) :
                bhvr
        ) {
            actor_camera_moved = controller_behave_actor(actor, bhvr);
        }

        // Move the actor.
        if (CHK_FLAG(actor->motion, ACTOR_MOTION_NO_BLOCKING)) {
            if (CHK_FLAG(actor->motion, ACTOR_MOTION_RELATIVE)) {
                UINT16 x = ACTOR_DELTA(actor, actor->relative_movement.x, x);
                UINT16 y = ACTOR_DELTA(actor, actor->relative_movement.y, y);
#if !SCENE_MOVE_OUT_OF_BOUNDS_ENABLED
                if (x & 0x8000) x = 0; // Clamp left movement greater than 0.
                if (y & 0x8000) y = 0; // Clamp up movement greater than 0.
#endif /* SCENE_MOVE_OUT_OF_BOUNDS_ENABLED */
                if (!actor_move(actor, x, y)) {
                    actor->motion = 0;
                }
            } else {
                const UINT16 x = actor->absolute_movement.x;
                const UINT16 y = actor->absolute_movement.y;
                if (!actor_move(actor, x, y)) {
                    actor->motion = 0;
                }
            }
        }

        // Check whether the actor is inside the viewport.
        if (!actor->pinned && !actor_is_inside_viewport(actor)) {
            if (actor->persistent) {
                actor = actor->prev;

                continue;
            } else {
                actor_t * prev = actor->prev;
                if (!VM_IS_LOCKED)
                    actor_deactivate(actor);
                actor = prev;

                continue;
            }
        }

        // Animate the actor.
        if (actor->animation_interval != ACTOR_ANIMATION_PAUSED && (game_time & actor->animation_interval) == 0) {
            const UINT8 frame_end = actor->animations[actor->animation].end + 1;
            if (++actor->frame >= frame_end) {
                if (actor->animation_loop) {
                    const UINT8 frame_begin = actor->animations[actor->animation].begin;
                    actor->frame = frame_begin;
                } else {
                    --actor->frame;
                }
            }
        }

        // Move the camera following the actor.
        if (actor_camera_moved)
            camera_moved |= actor_move_camera(actor);

        // Skip if the actor is hidden.
        if (actor->hidden) {
            actor = actor->prev;

            continue;
        }

        // Place the actor.
        ACTOR_MOVE(actor);

        // Carry on to the next actor (reversed direction).
        actor = actor->prev;
    }

    // Place the map if necessary.
    if (camera_moved) {
        FEATURE_MAP_MOVEMENT_SET;
    }
}

void actor_ctor(actor_t * actor) BANKED {
    actor_t * prev = actor->prev;
    actor_t * next = actor->next;
    memset(actor, 0, sizeof(actor_t));

    actor->prev               = prev;
    actor->next               = next;
    actor->template           = ACTOR_TEMPLATE_NONE;
    actor->enabled            = TRUE;
    actor->animation_loop     = TRUE;
    actor->animation_interval = ACTOR_ANIMATION_INTERVAL;
    actor->animation          = ACTOR_ANIMATION_DEFAULT_INDEX;
    actor->move_speed         = ACTOR_DEFAULT_MOVE_SPEED;
}

UINT8 actor_def(actor_t * actor, UINT16 x, UINT16 y, UINT8 base_tile, UINT8 bank, UINT8 * ptr) BANKED {
    const UINT8 * old              = ptr;
    actor->position.x              =   FROM_SCREEN(x);
    actor->position.y              =   FROM_SCREEN(y);
    actor->enabled                 = !!get_uint8(bank, ptr++);
    actor->hidden                  = !!get_uint8(bank, ptr++);
    actor->pinned                  = !!get_uint8(bank, ptr++);
    actor->persistent              = !!get_uint8(bank, ptr++);
    const BOOLEAN following        = !!get_uint8(bank, ptr++);
    actor->direction               =   get_uint8(bank, ptr++);
    actor->bounds.left             =   get_int8 (bank, ptr++);
    actor->bounds.right            =   get_int8 (bank, ptr++);
    actor->bounds.top              =   get_int8 (bank, ptr++);
    actor->bounds.bottom           =   get_int8 (bank, ptr++);
    actor->base_tile               =   base_tile;
    actor->sprite_bank             =   get_uint8(bank, ptr++);
    actor->sprite_frames           =   get_ptr  (bank, ptr  ); ptr += 2;
    actor->animation_interval      =   get_uint8(bank, ptr++);
    for (UINT8 i = 0; i != ACTOR_MAX_ANIMATIONS; ++i) {
        actor->animations[i].begin =   get_uint8(bank, ptr++);
        actor->animations[i].end   =   get_uint8(bank, ptr++);
    }
    actor->move_speed              =   get_uint8(bank, ptr++);
    actor->behaviour               =   get_uint8(bank, ptr++);
    actor->collision_group         =   get_uint8(bank, ptr++);
    // No routine is read here.

    if (following)
        actor_following_target = actor;

    if (!actor->active && actor->enabled && actor_is_inside_viewport(actor))
        actor_activate(actor);

    const UINT8 anim = actor->direction;
    actor->animation = anim; // Set the animation.
    actor->frame = actor->animations[anim].begin;

    return (UINT8)(ptr - old);
}

void vm_new_actor(SCRIPT_CTX * THIS) OLDCALL BANKED {
    actor_t * actor = actor_new();
    *(THIS->stack_ptr++) = (UINT16)(UINT8 *)actor;
}

void vm_del_actor(SCRIPT_CTX * THIS) OLDCALL BANKED {
    actor_t * actor = (actor_t *)*(--THIS->stack_ptr);
    if (actor) {
        actor_delete(actor);
    } else {
        while (actor_active_head)
            actor_delete(actor_active_head);
    }
}

void vm_def_actor(SCRIPT_CTX * THIS, UINT8 src) OLDCALL BANKED {
    actor_t * actor        = (actor_t *)*(--THIS->stack_ptr);
    const UINT16 x         = (UINT16)*(--THIS->stack_ptr);
    const UINT16 y         = (UINT16)*(--THIS->stack_ptr);
    const UINT16 base_tile = (UINT16)*(--THIS->stack_ptr);
    UINT8 bank;
    UINT8 * ptr;
    UINT8 size = 0;

    switch (src) {
    case ASSET_SOURCE_READ:
        bank = current_data_bank;
        ptr  = (UINT8 *)current_data_address;
        size = actor_def(actor, x, y, base_tile, bank, ptr);
        current_data_address += size;

        break;
    case ASSET_SOURCE_DATA:
        bank = THIS->bank;
        ptr  = (UINT8 *)THIS->PC;
        size = actor_def(actor, x, y, base_tile, bank, ptr);
        THIS->PC += size;

        break;
    case ASSET_SOURCE_FAR:
        bank = get_uint8 (THIS->bank, (UINT8 *)THIS->PC++);
        ptr  = get_ptr_be(THIS->bank, (UINT8 *)THIS->PC);   THIS->PC += 2;
        actor_def(actor, x, y, base_tile, bank, ptr);

        break;
    default:
        // Do nothing.

        break;
    }
}

void vm_get_actor_prop(SCRIPT_CTX * THIS) OLDCALL BANKED {
    actor_t * actor       = (actor_t *)*(--THIS->stack_ptr);
    const UINT8 p         = (UINT8)*(--THIS->stack_ptr);
    if (!actor->instantiated) {
        *(THIS->stack_ptr++) = 0;

        return;
    }
    switch (p) {
    case PROPERTY_ACTIVE:
        *(THIS->stack_ptr++) = actor->instantiated && actor->active;

        break;
    case PROPERTY_ENABLED:
        *(THIS->stack_ptr++) = actor->enabled;

        break;
    case PROPERTY_HIDDEN:
        *(THIS->stack_ptr++) = actor->hidden;

        break;
    case PROPERTY_PINNED:
        *(THIS->stack_ptr++) = actor->pinned;

        break;
    case PROPERTY_PERSISTENT:
        *(THIS->stack_ptr++) = actor->persistent ? TRUE : FALSE;

        break;
    case PROPERTY_FOLLOWING:
        *(THIS->stack_ptr++) = (actor == actor_following_target) ? TRUE : FALSE;

        break;
    case PROPERTY_ANIMATION_LOOP:
        *(THIS->stack_ptr++) = actor->animation_loop;

        break;
    case PROPERTY_ANIMATION_PAUSED:
        *(THIS->stack_ptr++) = (actor->animation_interval == ACTOR_ANIMATION_PAUSED) ? TRUE : FALSE;

        break;
    case PROPERTY_MOVEMENT_INTERRUPT:
        *(THIS->stack_ptr++) = actor->movement_interrupt;

        break;
    case PROPERTY_POSITION_X:
        *(THIS->stack_ptr++) = TO_SCREEN(actor->position.x);

        break;
    case PROPERTY_POSITION_Y:
        *(THIS->stack_ptr++) = TO_SCREEN(actor->position.y);

        break;
    case PROPERTY_DIRECTION:
        *(THIS->stack_ptr++) = actor->direction;

        break;
    case PROPERTY_ANGLE:
        if (actor->direction == DIRECTION_UP)
            *(THIS->stack_ptr++) = 0;
        else if (actor->direction == DIRECTION_RIGHT)
            *(THIS->stack_ptr++) = 64;
        else if (actor->direction == DIRECTION_DOWN)
            *(THIS->stack_ptr++) = 128;
        else if (actor->direction == DIRECTION_LEFT)
            *(THIS->stack_ptr++) = 192;

        break;
    case PROPERTY_BOUNDS_LEFT:
        *(THIS->stack_ptr++) = actor->bounds.left;

        break;
    case PROPERTY_BOUNDS_RIGHT:
        *(THIS->stack_ptr++) = actor->bounds.right;

        break;
    case PROPERTY_BOUNDS_TOP:
        *(THIS->stack_ptr++) = actor->bounds.top;

        break;
    case PROPERTY_BOUNDS_BOTTOM:
        *(THIS->stack_ptr++) = actor->bounds.bottom;

        break;
    case PROPERTY_BASE_TILE:
        *(THIS->stack_ptr++) = actor->base_tile;

        break;
    case PROPERTY_FRAME_INDEX:
        *(THIS->stack_ptr++) = actor->frame;

        break;
    case PROPERTY_ANIMATION_INTERVAL:
        *(THIS->stack_ptr++) = actor->animation_interval;

        break;
    case PROPERTY_ANIMATION_INDEX:
        *(THIS->stack_ptr++) = actor->animation;

        break;
    case PROPERTY_MOVE_SPEED:
        *(THIS->stack_ptr++) = actor->move_speed;

        break;
    case PROPERTY_BEHAVIOUR:
        *(THIS->stack_ptr++) = actor->behaviour;

        break;
    case PROPERTY_COLLISION_GROUP:
        *(THIS->stack_ptr++) = actor->collision_group;

        break;
    default:
        *(THIS->stack_ptr++) = FALSE;

#if VM_EXCEPTION_ENABLED
        VM_THROW(EXCEPTION_UNKNOWN_PARAMETER, EXCEPTION_ACTOR_ERROR, p);
#endif /* VM_EXCEPTION_ENABLED */

        break;
    }
}

INLINE void actor_get_frames(SCRIPT_CTX * THIS, UINT8 src, UINT8 * bank_, UINT8 ** ptr_, UINT8 * base_) {
    UINT8 bank;
    UINT8 * ptr;
    UINT8 base;

    switch (src) {
    case ASSET_SOURCE_READ:
        bank = current_data_bank;
        ptr  = (UINT8 *)current_data_address;
        base = (UINT8)*(--THIS->stack_ptr); // Retrieve from the stack rather than: `base = get_uint8(bank, ptr++); ++current_data_address;`.
        while (get_uint16(bank, (UINT16 *)current_data_address) != NULL)
            current_data_address += 2;
        current_data_address += 2; // Skip the end mark.

        break;
    case ASSET_SOURCE_DATA:
        bank = THIS->bank;
        ptr  = (UINT8 *)THIS->PC;
        base = get_uint8(bank, ptr++); ++THIS->PC;
        while (get_uint16(bank, (UINT16 *)THIS->PC) != NULL)
            THIS->PC += 2;
        THIS->PC += 2; // Skip the end mark.

        break;
    case ASSET_SOURCE_FAR:
        bank = get_uint8 (THIS->bank, (UINT8 *)THIS->PC++);
        ptr  = get_ptr_be(THIS->bank, (UINT8 *)THIS->PC);   THIS->PC += 2;
        base = (UINT8)*(--THIS->stack_ptr);

        break;
    default:
        bank = 0;
        ptr  = NULL;
        base = 0;

        break;
    }

    *bank_   = bank;
    *ptr_    = ptr;
    *base_   = base;
}

INLINE void actor_get_animations(SCRIPT_CTX * THIS, UINT8 src, UINT8 * bank_, UINT8 ** ptr_, UINT8 * base_, UINT8 * count_) {
    UINT8 bank;
    UINT8 * ptr;
    UINT8 base;
    UINT8 count;

    switch (src) {
    case ASSET_SOURCE_READ:
        bank  = current_data_bank;
        ptr   = (UINT8 *)current_data_address;
        base  = (UINT8)*(--THIS->stack_ptr);
        count = (UINT8)*(--THIS->stack_ptr);
        current_data_address += MUL2(count);

        break;
    case ASSET_SOURCE_DATA:
        bank  = THIS->bank;
        ptr   = (UINT8 *)THIS->PC;
        base  = get_uint8(bank, ptr++); ++THIS->PC;
        count = get_uint8(bank, ptr++); ++THIS->PC;
        THIS->PC += MUL2(count);

        break;
    case ASSET_SOURCE_FAR:
        bank  = get_uint8 (THIS->bank, (UINT8 *)THIS->PC++);
        ptr   = get_ptr_be(THIS->bank, (UINT8 *)THIS->PC);   THIS->PC += 2;
        base  = (UINT8)*(--THIS->stack_ptr);
        count = (UINT8)*(--THIS->stack_ptr);

        break;
    default:
        bank  = 0;
        ptr   = NULL;
        base  = 0;
        count = 0;

        break;
    }

    *bank_    = bank;
    *ptr_     = ptr;
    *base_    = base;
    *count_   = count;
}

INLINE void actor_get_animation(SCRIPT_CTX * THIS, UINT8 src, UINT8 * bank_, UINT8 ** ptr_, UINT8 * base_) {
    UINT8 bank;
    UINT8 * ptr;
    UINT8 base;

    switch (src) {
    case ASSET_SOURCE_READ:
        bank = current_data_bank;
        ptr  = (UINT8 *)current_data_address;
        base = (UINT8)*(--THIS->stack_ptr);
        current_data_address += 2;

        break;
    case ASSET_SOURCE_DATA:
        bank = THIS->bank;
        ptr  = (UINT8 *)THIS->PC;
        base = get_uint8(bank, ptr++); ++THIS->PC;
        THIS->PC += 2;

        break;
    case ASSET_SOURCE_FAR:
        bank = get_uint8 (THIS->bank, (UINT8 *)THIS->PC++);
        ptr  = get_ptr_be(THIS->bank, (UINT8 *)THIS->PC);   THIS->PC += 2;
        base = (UINT8)*(--THIS->stack_ptr);

        break;
    default:
        bank = 0;
        ptr  = NULL;
        base = 0;

        break;
    }

    *bank_   = bank;
    *ptr_    = ptr;
    *base_   = base;
}

void vm_set_actor_prop(SCRIPT_CTX * THIS, UINT8 src) OLDCALL BANKED {
    actor_t * actor = (actor_t *)*(--THIS->stack_ptr);
    const UINT8 p   = (UINT8)*(--THIS->stack_ptr);
    if (!actor->instantiated) {
        --THIS->stack_ptr;

        return;
    }
    switch (p) {
    case PROPERTY_ENABLED:
        actor->enabled = (bool)*(--THIS->stack_ptr);

        if (!actor->active && actor->enabled && actor_is_inside_viewport(actor))
            actor_activate(actor);

        break;
    case PROPERTY_HIDDEN:
        actor->hidden = (bool)*(--THIS->stack_ptr);

        break;
    case PROPERTY_PINNED:
        actor->pinned = (bool)*(--THIS->stack_ptr);

        break;
    case PROPERTY_PERSISTENT:
        actor->persistent = *(--THIS->stack_ptr) ? TRUE : FALSE;

        break;
    case PROPERTY_FOLLOWING:
        if (*(--THIS->stack_ptr)) {
            actor_following_target = actor;
        } else {
            if (actor_following_target == actor)
                actor_following_target = NULL;
        }

        break;
    case PROPERTY_ANIMATION_LOOP:
        actor->animation_loop = (bool)*(--THIS->stack_ptr);

        break;
    case PROPERTY_ANIMATION_PAUSED:
        if ((bool)*(--THIS->stack_ptr)) {
            actor->animation_interval = ACTOR_ANIMATION_PAUSED;
        } else {
            if (actor->animation_interval == ACTOR_ANIMATION_PAUSED)
                actor->animation_interval = ACTOR_ANIMATION_INTERVAL;
        }

        break;
    case PROPERTY_MOVEMENT_INTERRUPT:
        actor->movement_interrupt = (bool)*(--THIS->stack_ptr);

        break;
    case PROPERTY_POSITION:
        actor->position.x = FROM_SCREEN((UINT16)*(--THIS->stack_ptr));
        actor->position.y = FROM_SCREEN((UINT16)*(--THIS->stack_ptr));

        if (!actor->active && actor->enabled && actor_is_inside_viewport(actor))
            actor_activate(actor);

        break;
    case PROPERTY_POSITION_X:
        actor->position.x = FROM_SCREEN((UINT16)*(--THIS->stack_ptr));

        if (!actor->active && actor->enabled && actor_is_inside_viewport(actor))
            actor_activate(actor);

        break;
    case PROPERTY_POSITION_Y:
        actor->position.y = FROM_SCREEN((UINT16)*(--THIS->stack_ptr));

        if (!actor->active && actor->enabled && actor_is_inside_viewport(actor))
            actor_activate(actor);

        break;
    case PROPERTY_DIRECTION:
        actor->direction = (UINT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_BOUNDS:
        actor->bounds.left   = (INT8)*(--THIS->stack_ptr);
        actor->bounds.right  = (INT8)*(--THIS->stack_ptr);
        actor->bounds.top    = (INT8)*(--THIS->stack_ptr);
        actor->bounds.bottom = (INT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_BOUNDS_LEFT:
        actor->bounds.left = (INT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_BOUNDS_RIGHT:
        actor->bounds.right = (INT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_BOUNDS_TOP:
        actor->bounds.top = (INT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_BOUNDS_BOTTOM:
        actor->bounds.bottom = (INT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_BASE_TILE:
        actor->base_tile = (UINT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_FRAMES: {
            UINT8 bank;
            UINT8 * ptr;
            UINT8 base;
            actor_get_frames(THIS, src, &bank, &ptr, &base);
            actor->base_tile          = base;
            actor->sprite_bank        = bank;
            actor->sprite_frames      = (metasprite_t **)ptr;
            actor->animation          = ACTOR_ANIMATION_DEFAULT_INDEX;
            actor->animation_interval = ACTOR_ANIMATION_PAUSED;
            actor->frame              = 0;
        }

        break;
    case PROPERTY_FRAME_INDEX:
        actor->frame = (UINT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_ANIMATION_INTERVAL:
        actor->animation_interval = (UINT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_ANIMATIONS: {
            UINT8 bank;
            UINT8 * ptr;
            UINT8 base;
            UINT8 count;
            actor_get_animations(THIS, src, &bank, &ptr, &base, &count);
            for (UINT8 i = base; i < base + count; ++i) {
                actor->animations[i].begin = get_uint8(bank, ptr++);
                actor->animations[i].end   = get_uint8(bank, ptr++);
            }
            for (UINT8 i = 0; i < base; ++i) {
                actor->animations[i].begin = 0;
                actor->animations[i].end   = 0;
            }
            for (UINT8 i = base + count; i < ACTOR_MAX_ANIMATIONS; ++i) {
                actor->animations[i].begin = 0;
                actor->animations[i].end   = 0;
            }
        }

        break;
    case PROPERTY_ANIMATION: {
            UINT8 bank;
            UINT8 * ptr;
            UINT8 base;
            actor_get_animation(THIS, src, &bank, &ptr, &base);
            actor->animations[base].begin = get_uint8(bank, ptr++);
            actor->animations[base].end   = get_uint8(bank, ptr++);
        }

        break;
    case PROPERTY_ANIMATION_INDEX:
        actor->animation = (UINT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_MOVE_SPEED:
        actor->move_speed = (UINT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_BEHAVIOUR:
        actor->behaviour = (UINT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_COLLISION_GROUP:
        actor->collision_group = (UINT8)*(--THIS->stack_ptr);

        break;
    default:
        --THIS->stack_ptr;

#if VM_EXCEPTION_ENABLED
        VM_THROW(EXCEPTION_UNKNOWN_PARAMETER, EXCEPTION_ACTOR_ERROR, p);
#endif /* VM_EXCEPTION_ENABLED */

        break;
    }
}

void vm_play_actor(SCRIPT_CTX * THIS) OLDCALL BANKED {
    actor_t * actor   = (actor_t *)*(--THIS->stack_ptr);
    const UINT8 index = (UINT8)*(--THIS->stack_ptr);
    if (index == ACTOR_ANIMATION_TO_IDLE_INDEX) {
        actor_transfer_animation_to_idle(actor);
    } else if (index == ACTOR_ANIMATION_TO_MOVING_INDEX) {
        actor_transfer_animation_to_moving(actor);
    } else if (actor->animation != index) {
        actor->frame     = actor->animations[index].begin;
        actor->animation = index;
    }
}

void vm_thread_actor(SCRIPT_CTX * THIS, UINT8 op) OLDCALL BANKED {
    actor_t * actor = (actor_t *)*(--THIS->stack_ptr);
    switch (op) {
    case ACTOR_THREADING_BEGIN: {
            if (!actor_try_terminate_thread(&actor->behave_thread_id)) {
                *(THIS->stack_ptr++) = (UINT16)actor; // Push back to wait until joined.
                THIS->PC -= INSTRUCTION_SIZE + sizeof(op); // Wait.
                THIS->waitable = TRUE;

                break;
            }
            const UINT8 bank = (UINT8)*(--THIS->stack_ptr);
            UINT8 * address  = (UINT8 *)*(--THIS->stack_ptr);
            actor->behave_handler_bank    = bank; // Remember the last update routine.
            actor->behave_handler_address = address;
            actor_begin_behave_thread(actor, bank, address);
        }

        break;
    case ACTOR_THREADING_JOIN:
        if (actor->behave_thread_id) {
            if (!VM_IS_TERMINATED(actor->behave_thread_id)) {
                *(THIS->stack_ptr++) = (UINT16)actor; // Push back to wait until joined.
                THIS->PC -= INSTRUCTION_SIZE + sizeof(op); // Wait.
                THIS->waitable = TRUE;

                break;
            }

            actor->behave_thread_id = 0;
        }

        break;
    case ACTOR_THREADING_TERMINATE:
        if (actor->behave_thread_id) {
            if (!VM_IS_TERMINATED(actor->behave_thread_id))
                script_terminate((UINT8)actor->behave_thread_id, TRUE);
            actor->behave_thread_id = 0;
        }

        break;
    case ACTOR_THREADING_WAIT:
        if (actor->motion) {
            *(THIS->stack_ptr++) = (UINT16)actor; // Push back to wait until joined.
            THIS->PC -= INSTRUCTION_SIZE + sizeof(op); // Wait.
            THIS->waitable = TRUE;
        }

        break;
    }
}

INLINE BOOLEAN actor_move_in_direction_wait(actor_t * actor, INT8 dx, INT8 dy) {
    const UINT16 x = ACTOR_DELTA(actor, dx, x);
    const UINT16 y = ACTOR_DELTA(actor, dy, y);
    if (!actor_move(actor, x, y)) {
        actor->motion = 0;

        return FALSE;
    }

    return TRUE;
}

INLINE BOOLEAN actor_move_to_point_wait(actor_t * actor, UINT16 x, UINT16 y) {
    if (!actor_move(actor, x, y)) {
        actor->motion = 0;

        return FALSE;
    }

    return TRUE;
}

INLINE BOOLEAN actor_move_in_direction_(SCRIPT_CTX * THIS) {
    actor_t * actor       = (actor_t *)*(THIS->stack_ptr - 1);
    UINT8 * wait          = (UINT8 *)(THIS->stack_ptr - 2);
    const INT8 dx         = (INT8)*(INT16*)(THIS->stack_ptr - 3);
    const INT8 dy         = (INT8)*(INT16*)(THIS->stack_ptr - 4);
    const UINT8 flags     = (UINT8)*(THIS->stack_ptr - 5);
    const BOOLEAN movable = !flags || controller_is_actor_movable(actor, dx, dy, flags);
    if (*wait) {
        THIS->waitable = TRUE;
        if (movable && !actor_move_in_direction_wait(actor, dx, dy)) {
            THIS->stack_ptr -= 5;

            return FALSE; // Non-loop.
        }
        if ((--*wait) == 0) {
            THIS->stack_ptr -= 5;

            actor->motion = 0;

            return FALSE; // Non-loop.
        }

        return TRUE; // Loop.
    } else {
        if (movable)
            actor_move_in_direction(actor, dx, dy);
        THIS->stack_ptr -= 5;

        return FALSE; // Non-loop.
    }
}

INLINE BOOLEAN actor_move_to_point_(SCRIPT_CTX * THIS) {
    actor_t * actor    = (actor_t *)*(THIS->stack_ptr - 1);
    const BOOLEAN wait = (BOOLEAN)*(THIS->stack_ptr - 2);
    const UINT16 x     = FROM_SCREEN((UINT16)*(THIS->stack_ptr - 3));
    const UINT16 y     = FROM_SCREEN((UINT16)*(THIS->stack_ptr - 4));
    const UINT8 flags  = (UINT8)*(THIS->stack_ptr - 5);
    BOOLEAN movable    = TRUE;
    if (flags) {
        INT8 dx = 0, dy = 0;
        if      (x < actor->position.x) dx = -1;
        else if (x > actor->position.x) dx =  1;
        if      (y < actor->position.y) dy = -1;
        else if (y > actor->position.y) dy =  1;
        else    goto end; // Already there, no need to move.
        movable = controller_is_actor_movable(actor, dx, dy, flags);
    }
    if (wait) {
        THIS->waitable = TRUE;
        if (movable && !actor_move_to_point_wait(actor, x, y)) {
            THIS->stack_ptr -= 5;

            return FALSE; // Non-loop.
        }

        return TRUE; // Loop.
    }

end:
    actor_move_to_point(actor, x, y);
    THIS->stack_ptr -= 5;

    return FALSE; // Non-loop.
}

INLINE BOOLEAN actor_move_stop_(SCRIPT_CTX * THIS) {
    actor_t * actor = (actor_t *)*(--THIS->stack_ptr);
    if (actor) {
        actor_move_stop(actor);
    } else {
        actor = actor_active_tail;
        while (actor) {
            actor_move_stop(actor);
            actor = actor->prev;
        }
    }

    return FALSE; // Always non-loop.
}

void vm_move_actor(SCRIPT_CTX * THIS, UINT8 op) OLDCALL BANKED {
    BOOLEAN loop = FALSE;
    switch (op) {
    case ACTOR_MOTION_IN_DIRECTION:   loop = actor_move_in_direction_(THIS); break;
    case ACTOR_MOTION_TO_POINT:       loop = actor_move_to_point_(THIS);     break;
    case ACTOR_MOTION_STOP:           loop = actor_move_stop_(THIS);         break;
    }
    if (loop) THIS->PC -= INSTRUCTION_SIZE + sizeof(op);
}

void vm_find_actor(SCRIPT_CTX * THIS, UINT8 opt) OLDCALL BANKED {
    UINT8 val       = (UINT8)*(--THIS->stack_ptr);
    actor_t * start = (actor_t *)*(--THIS->stack_ptr);
    UINT8 i         = 0;
    if (start && start->instantiated) {
        for (UINT8 j = 0; j != ACTOR_MAX_COUNT; ++j) {
            actor_t * actor = &actors[j];
            if (start == actor) {
                i = j + 1;

                break;
            }
        }
    }
    if (i == ACTOR_MAX_COUNT) {
        *(THIS->stack_ptr++) = 0;

        return;
    }

    switch (opt) {
    case ACTOR_FILTER_BY_TEMPLATE:
        for ( ; i != ACTOR_MAX_COUNT; ++i) {
            actor_t * actor = &actors[i];
            if (!actor->instantiated)
                continue;

            if (val == ACTOR_TEMPLATE_ANY || actor->template == val) {
                *(THIS->stack_ptr++) = (UINT16)actor;

                return;
            }
        }

        break;
    case ACTOR_FILTER_BY_BEHAVIOUR:
        val &= ~CONTROLLER_BEHAVIOUR_OPTIONS;
        for ( ; i != ACTOR_MAX_COUNT; ++i) {
            actor_t * actor = &actors[i];
            if (!actor->instantiated)
                continue;

            if ((actor->behaviour & ~CONTROLLER_BEHAVIOUR_OPTIONS) == val) {
                *(THIS->stack_ptr++) = (UINT16)actor;

                return;
            }
        }

        break;
    }

    *(THIS->stack_ptr++) = 0;
}

void vm_on_actor(SCRIPT_CTX * THIS, UINT8 bank, UINT8 * pc, UINT8 opt) OLDCALL BANKED {
    actor_t * actor = (actor_t *)*(--THIS->stack_ptr);
    switch (opt) {
    case ACTOR_HANDLER_HITS:
        actor->hit_handler_bank    = bank;
        actor->hit_handler_address = pc;

        break;
    }
}
