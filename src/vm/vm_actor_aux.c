#pragma bank 255

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "vm_actor.h"
#include "vm_device.h"
#include "vm_projectile.h"
#include "vm_scene.h"

BANKREF(VM_ACTOR_AUX)

actor_t * actor_new(void) BANKED {
    // Prepare.
    actor_t * actor = actor_inactive_head;
    for (UINT8 i = 0; i != ACTOR_MAX_COUNT; ++i) {
        actor_t * actor_ = &actors[i];
        if (!actor_->instantiated) {
            actor = actor_;

            break;
        }
    }
    if (!actor)
        return NULL;

    // Initialize the actor.
    ACTOR_DL_PUSH_TAIL(actor_inactive_head, actor_inactive_tail, actor);

    actor_ctor(actor);
    actor->instantiated = TRUE;
    actor->position.x = FROM_SCREEN(scene_camera_x - DIV2(DEVICE_SCREEN_PX_WIDTH));
    actor->position.y = FROM_SCREEN(scene_camera_y - DIV2(DEVICE_SCREEN_PX_HEIGHT));

    // Finish.
    return actor;
}

void actor_delete(actor_t * actor) BANKED {
    // Prepare.
    if (!actor->instantiated)
        return;

    // Terminate the update thread if any.
    actor_terminate_thread(&actor->behave_thread_id);
    actor_detach_thread(&actor->hit_thread_id);

    // Reset the collision states.
    if (actor == actor_collided_a || actor == actor_collided_b) {
        actor_collided_a       = NULL;
        actor_collided_b       = NULL;
        actor_collided_counter = ACTOR_COLLIDED_COOLDOWN_FRAMES;
    }

    // Dispose the actor.
    actor->instantiated = FALSE;
    if (actor->active) {
        actor->active = FALSE;

        ACTOR_DL_REMOVE_ITEM(actor_active_head, actor_active_tail, actor);
    } else {
        ACTOR_DL_REMOVE_ITEM(actor_inactive_head, actor_inactive_tail, actor);
    }

    // Reset the hardware sprites if there's no object left.
    if (
        !actor_active_head
#if USE_PROJECTILE
         && !projectile_active_head
#endif /* USE_PROJECTILE */
    ) {
        actor_hardware_sprite_count = device_object_sprite_base;
        for (UINT8 i = actor_hardware_sprite_count; i != MAX_HARDWARE_SPRITES; ++i) {
            hide_sprite(i);
        }
    }
}

void actor_activate(actor_t * actor) BANKED {
    ACTOR_DL_REMOVE_ITEM(actor_inactive_head, actor_inactive_tail, actor);
    ACTOR_DL_PUSH_HEAD(actor_active_head, actor_active_tail, actor);

    actor->active = TRUE;
    actor_set_animation(actor, actor->direction);

    actor->behave_thread_id = SCRIPT_TERMINATED | SCRIPT_FINISHED;
    if (actor->behave_handler_bank) {
        SCRIPT_CTX * ctx = script_execute(actor->behave_handler_bank, actor->behave_handler_address, &actor->behave_thread_id, 0);

        if (ctx)
            *(ctx->stack_ptr++) = (UINT16)actor; // Initialize thread locals with the actor's handle.
    }
    actor->hit_thread_id = SCRIPT_TERMINATED | SCRIPT_FINISHED;
}

void actor_deactivate(actor_t * actor) BANKED {
    actor_terminate_thread(&actor->behave_thread_id);
    actor_detach_thread(&actor->hit_thread_id);

    actor->active = FALSE;

    ACTOR_DL_REMOVE_ITEM(actor_active_head, actor_active_tail, actor);
    ACTOR_DL_PUSH_TAIL(actor_inactive_head, actor_inactive_tail, actor);
}

UINT8 actor_instantiated_count(void) BANKED {
    UINT8 ret = 0;
    for (UINT8 i = 0; i != ACTOR_MAX_COUNT; ++i) {
        if (actors[i].instantiated)
            ++ret;
    }

    return ret;
}

UINT8 actor_free_count(void) BANKED {
    UINT8 ret = 0;
    for (UINT8 i = 0; i != ACTOR_MAX_COUNT; ++i) {
        if (!actors[i].instantiated)
            ++ret;
    }

    return ret;
}

UINT8 actor_active_count(void) BANKED {
    UINT8 ret = 0;
    for (UINT8 i = 0; i != ACTOR_MAX_COUNT; ++i) {
        if (actors[i].active)
            ++ret;
    }

    return ret;
}

BOOLEAN actor_move(actor_t * actor, UINT16 x, UINT16 y) BANKED {
    // Determine movement.
    if (actor->motion == 0) {
        actor->movement_interrupt = FALSE;
        if (x != actor->position.x) {
            SET_FLAG(
                actor->motion,
                x < actor->position.x ?
                    ACTOR_MOTION_MOVE_LEFT : ACTOR_MOTION_MOVE_RIGHT
            );
        }
        if (y != actor->position.y) {
            SET_FLAG(
                actor->motion,
                y < actor->position.y ?
                    ACTOR_MOTION_MOVE_UP : ACTOR_MOTION_MOVE_DOWN
            );
        }
    }

    // Interrupt movement.
    if (actor->movement_interrupt) {
        actor->movement_interrupt = FALSE;

        return FALSE;
    }

    // Move in the x-axis.
    if (CHK_FLAG(actor->motion, ACTOR_MOTION_MOVE_X)) {
        UINT8 dir;
        if         (x < actor->position.x)    dir = DIRECTION_LEFT;
        else /* if (x > actor->position.x) */ dir = DIRECTION_RIGHT;
        point_translate_dir(&actor->position, dir, actor->move_speed);
        if (
            (dir == DIRECTION_LEFT && actor->position.x <= x) ||
            (dir == DIRECTION_RIGHT && actor->position.x >= x)
        ) {
            actor->position.x = x;
            CLR_FLAG(
                actor->motion,
                ACTOR_MOTION_MOVE_X
            );
        }
    }

    // Move in the y-axis.
    if (CHK_FLAG(actor->motion, ACTOR_MOTION_MOVE_Y)) {
        UINT8 dir;
        if         (y < actor->position.y)    dir = DIRECTION_UP;
        else /* if (y > actor->position.y) */ dir = DIRECTION_DOWN;
        point_translate_dir(&actor->position, dir, actor->move_speed);
        if (
            (dir == DIRECTION_UP && actor->position.y <= y) ||
            (dir == DIRECTION_DOWN && actor->position.y >= y)
        ) {
            actor->position.y = y;
            CLR_FLAG(
                actor->motion,
                ACTOR_MOTION_MOVE_Y
            );
        }
    }

    // Check whether the actor has reached the destination.
    if (!CHK_FLAG(actor->motion, ACTOR_MOTION_MOVE)) {
        return FALSE;
    }

    // Finish.
    return TRUE;
}

void actor_move_stop(actor_t * actor) BANKED {
    if (actor->motion == 0) return; // Is already stopped.
    actor->movement_interrupt = TRUE;
    actor->motion = 0;
    actor->movement = 0; // This line clears the `movement` union.
}

void actor_reserve_move_speed(actor_t * actor, UINT8 speed_up) BANKED {
    if (actor->original_move_speed != 0) return; // Already reserved.
    actor->original_move_speed = actor->move_speed;
    actor->max_move_speed = actor->move_speed + speed_up;
}

void actor_restore_move_speed(actor_t * actor) BANKED {
    if (actor->original_move_speed == 0) return; // Not yet reserved.
    actor->move_speed = actor->original_move_speed;
}

void actor_activate_actors_in_row(UINT8 x, UINT8 y) BANKED {
    actor_t * actor = actor_inactive_tail;

    while (actor) {
        const UINT8 ty = TO_SCREEN_TILE(actor->position.y);
        if (ty == y) {
            const UINT8 tx = TO_SCREEN_TILE(actor->position.x);
            if (tx >= x && tx < x + DEVICE_SCREEN_WIDTH) {
                actor_t * prev = actor->prev;
                if (actor->enabled)
                    actor_activate(actor);
                actor = prev;

                continue;
            }
        }

        actor = actor->prev;
    }
}

void actor_activate_actors_in_col(UINT8 x, UINT8 y) BANKED {
    actor_t * actor = actor_inactive_tail;

    while (actor) {
        const UINT16 sx = TO_SCREEN(actor->position.x);
        const UINT16 sy = TO_SCREEN(actor->position.y);
        if (
            (UINT8)DIV8(sx + actor->bounds.left  ) <= x &&
            (UINT8)DIV8(sx + actor->bounds.right ) >= x &&
            (UINT8)DIV8(sy + actor->bounds.top   ) <= y + DEVICE_SCREEN_HEIGHT - 1 &&
            (UINT8)DIV8(sy + actor->bounds.bottom) >= y
        ) {
            actor_t * prev = actor->prev;
            if (actor->enabled)
                actor_activate(actor);
            actor = prev;

            continue;
        }

        actor = actor->prev;
    }
}

actor_t * actor_in_front_of_actor(actor_t * actor, UINT8 forward, UINT8 inc_no_collision) BANKED {
    upoint16_t off = {
        .x = actor->position.x,
        .y = actor->position.y
    };
    point_translate_dir_uint16(&off, actor->direction, FROM_SCREEN(forward));

    return actor_hits(&actor->bounds, &off, actor, inc_no_collision);
}

actor_t * actor_hits(const boundingbox_t * bb, const upoint16_t * offset, actor_t * ignore, BOOLEAN inc_no_collision) BANKED {
    const upoint16_t off = {
        .x = (UINT16)TO_SCREEN(offset->x),
        .y = (UINT16)TO_SCREEN(offset->y)
    };

    actor_t * actor = actor_active_tail;
    while (actor) {
        if (actor == ignore || (!inc_no_collision && actor->collision_group == 0)) {
            actor = actor->prev;

            continue;
        }

        const upoint16_t pos = {
            .x = (UINT16)TO_SCREEN(actor->position.x),
            .y = (UINT16)TO_SCREEN(actor->position.y)
        };
        if (boundingbox_intersects(bb, &off, &actor->bounds, &pos)) {
            return actor;
        }

        actor = actor->prev;
    }

    return NULL;
}

actor_t * actor_hits_in_group(const boundingbox_t * bb, const upoint16_t * offset, UINT8 collision_group) BANKED {
    const upoint16_t off = {
        .x = (UINT16)TO_SCREEN(offset->x),
        .y = (UINT16)TO_SCREEN(offset->y)
    };

    actor_t * actor = actor_active_tail;
    while (actor) {
        if ((actor->collision_group & collision_group) == 0) {
            actor = actor->prev;

            continue;
        }

        const upoint16_t pos = {
            .x = (UINT16)TO_SCREEN(actor->position.x),
            .y = (UINT16)TO_SCREEN(actor->position.y)
        };
        if (boundingbox_intersects(bb, &off, &actor->bounds, &pos)) {
            return actor;
        }

        actor = actor->prev;
    }

    return NULL;
}

void actor_handle_collision(void) BANKED {
    if (actor_collided_a != NULL /* && actor_collided_b != NULL */) {
        if (actor_collided_counter == 0) {
            actor_begin_hit_thread(actor_collided_a, actor_collided_b);
            actor_begin_hit_thread(actor_collided_b, actor_collided_a);

            actor_collided_a = NULL;
            actor_collided_b = NULL;
            actor_collided_counter = ACTOR_COLLIDED_COOLDOWN_FRAMES;

            return;
        }
        actor_collided_a = NULL;
        actor_collided_b = NULL;
    }
    if (actor_collided_counter != 0) {
        --actor_collided_counter;
    }
}

BOOLEAN actor_begin_behave_thread(actor_t * actor, UINT8 bank, UINT8 * pc) BANKED {
    if (!actor->active)
        return FALSE;

    SCRIPT_CTX * ctx = script_execute(bank, pc, &actor->behave_thread_id, 0);
    if (!ctx)
        return FALSE;

    *(ctx->stack_ptr++) = (UINT16)actor; // Initialize thread locals with the actor's handle.

    actor->motion = 0; // Clear the actor's motion.

    return TRUE;
}

void actor_begin_hit_thread(actor_t * actor, actor_t * other) BANKED {
    if (actor->hit_handler_bank == 0)
        return;
    if (actor->hit_thread_id != 0 && !VM_IS_TERMINATED(actor->hit_thread_id))
        return;

    SCRIPT_CTX * ctx = script_execute(actor->hit_handler_bank, actor->hit_handler_address, &actor->hit_thread_id, 0);
    if (!ctx)
        return;

    if (FEATURE_ACTOR_HIT_WITH_DETAILS_ENABLED) {
        UINT8 dirx = DIRECTION_NONE, diry = DIRECTION_NONE;
        UINT16 dx = 0, dy = 0;
        if (actor->position.x < other->position.x) {
            dirx = DIRECTION_RIGHT;
            dx = other->position.x - actor->position.x;
        } else if (actor->position.x > other->position.x) {
            dirx = DIRECTION_LEFT;
            dx = actor->position.x - other->position.x;
        }
        if (actor->position.y < other->position.y) {
            diry = DIRECTION_DOWN;
            dy = other->position.y - actor->position.y;
        } else if (actor->position.y > other->position.y) {
            diry = DIRECTION_UP;
            dy = actor->position.y - other->position.y;
        }
        const UINT8 dir = (dx > dy + FROM_SCREEN(3)) ? dirx : diry;
        *(ctx->stack_ptr++) = (UINT16)dir; // Initialize thread local with the hit direction.
        *(ctx->stack_ptr++) = (UINT16)other->collision_group; // Initialize thread local with the hit target's collision group.
    }
    *(ctx->stack_ptr++) = (UINT16)other; // Initialize thread locals with the other actor's handle.
    *(ctx->stack_ptr++) = (UINT16)actor; // Initialize thread locals with the actor's handle.
}

void actor_terminate_thread(UINT16 * id) BANKED {
    if (!(*id))
        return;

    if (!VM_IS_TERMINATED(*id))
        script_terminate((UINT8)(*id), TRUE);

    *id = 0;
}

void actor_detach_thread(UINT16 * id) BANKED {
    if (!(*id))
        return;

    if (!VM_IS_TERMINATED(*id))
        script_detach_hthread((UINT8)(*id));

    *id = 0;
}

BOOLEAN actor_try_terminate_thread(UINT16 * id) BANKED {
    if (!(*id))
        return TRUE;

    if (!VM_IS_TERMINATED(*id))
        script_terminate((UINT8)(*id), FALSE);

    if (!VM_IS_FINISHED(*id))
        return FALSE;

    *id = 0;

    return TRUE;
}
