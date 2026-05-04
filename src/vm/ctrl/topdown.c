#pragma bank 255

#if defined __SDCC
#   pragma disable_warning 126
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "../utils/utils.h"

#include "../vm_input.h"
#include "../vm_scene.h"
#include "../vm_trigger.h"

#include "controller.h"
#include "navigation.h"
#include "topdown.h"

#if USE_TOPDOWN

#define TOPDOWN_ACT_BUTTON   J_A

#define TOPDOWN_ALIGNED_TO_TILE(A) \
    ((scene.is_16x16_grid && IS_ON_16PX_GRID((A)->position, 0, 0)) || (!scene.is_16x16_grid && IS_ON_8PX_GRID((A)->position, 0, 0)))
#define TOPDOWN_ALIGN_TO_NEAREST_TILE(A, C, OLD, ENDPOINT) \
    const UINT16 OLD = (A)->position.C; \
    do { \
        if (scene.is_16x16_grid && scene.is_16x16_player) { \
            (A)->position.C = FROM_SCREEN(TO_SCREEN((A)->position.C) & 0xFFF0); \
            if ((ENDPOINT) == NAVIGATION_BLOCKING_ENDPOINT_START && (A)->position.C < OLD) \
                (A)->position.C += FROM_SCREEN(16); \
            else if ((ENDPOINT) == NAVIGATION_BLOCKING_ENDPOINT_END && (A)->position.C > OLD) \
                (A)->position.C -= FROM_SCREEN(16); \
        } else { \
            (A)->position.C = FROM_SCREEN(TO_SCREEN((A)->position.C) & 0xFFF8); \
            if ((ENDPOINT) == NAVIGATION_BLOCKING_ENDPOINT_START && (A)->position.C < OLD) \
                (A)->position.C += FROM_SCREEN(8); \
            else if ((ENDPOINT) == NAVIGATION_BLOCKING_ENDPOINT_END && (A)->position.C > OLD) \
                (A)->position.C -= FROM_SCREEN(8); \
        } \
    } while (0)
#define TOPDOWN_REVERT_ALIGNMENT(A, C, OLD) \
    ((A)->position.C = OLD)
#define TOPDOWN_DIFF_ALIGNMENT(A, C, OLD, DIFF) \
    const INT8 DIFF = ((A)->position.C < OLD ? -1 : 1)

STATIC BOOLEAN controller_behave_topdown_player_update(actor_t * actor) {
    // Prepare.
    BOOLEAN result = FALSE;
    BOOLEAN moving = FALSE;

    // Check input to set actor movement.
    UINT8 dir;
    if (INPUT_IS_BTN_PRESSED(J_LEFT)) {
        dir = DIRECTION_LEFT;
        const UINT8 ep = navigation_get_blocking_left_endpoint(actor, CONTROLLER_NEGATIVE_SPEED_OF(actor));
        if (ep) {
#if ACTOR_IMPLEMENT_AUTO_ALIGNING_ON_BLOCKED_ENABLED
            if (!TOPDOWN_ALIGNED_TO_TILE(actor)) {
                TOPDOWN_ALIGN_TO_NEAREST_TILE(actor, y, old_y, ep);
                if (navigation_get_blocking_left(actor, CONTROLLER_NEGATIVE_SPEED_OF(actor))) {
                    TOPDOWN_REVERT_ALIGNMENT(actor, y, old_y);
                } else {
                    TOPDOWN_DIFF_ALIGNMENT(actor, y, old_y, dy);
                    TOPDOWN_REVERT_ALIGNMENT(actor, y, old_y);
                    moving = TRUE;
#   if ACTOR_IMPLEMENT_AUTO_ALIGNING_WITH_TURN_ENABLED
                    dir = dy < 0 ? DIRECTION_UP : DIRECTION_DOWN;
#   endif /* ACTOR_IMPLEMENT_AUTO_ALIGNING_WITH_TURN_ENABLED */
                    actor_move_in_y_direction(actor, dy);
                }
            }
#endif /* ACTOR_IMPLEMENT_AUTO_ALIGNING_ON_BLOCKED_ENABLED */
        } else {
            moving = TRUE;
            actor_move_in_x_direction(actor, -1);
        }
        actor_play_animation(actor, dir, moving);
    } else if (INPUT_IS_BTN_PRESSED(J_RIGHT)) {
        dir = DIRECTION_RIGHT;
        const UINT8 ep = navigation_get_blocking_right_endpoint(actor, CONTROLLER_POSITIVE_SPEED_OF(actor));
        if (ep) {
#if ACTOR_IMPLEMENT_AUTO_ALIGNING_ON_BLOCKED_ENABLED
            if (!TOPDOWN_ALIGNED_TO_TILE(actor)) {
                TOPDOWN_ALIGN_TO_NEAREST_TILE(actor, y, old_y, ep);
                if (navigation_get_blocking_right(actor, CONTROLLER_POSITIVE_SPEED_OF(actor))) {
                    TOPDOWN_REVERT_ALIGNMENT(actor, y, old_y);
                } else {
                    TOPDOWN_DIFF_ALIGNMENT(actor, y, old_y, dy);
                    TOPDOWN_REVERT_ALIGNMENT(actor, y, old_y);
                    moving = TRUE;
#   if ACTOR_IMPLEMENT_AUTO_ALIGNING_WITH_TURN_ENABLED
                    dir = dy < 0 ? DIRECTION_UP : DIRECTION_DOWN;
#   endif /* ACTOR_IMPLEMENT_AUTO_ALIGNING_WITH_TURN_ENABLED */
                    actor_move_in_y_direction(actor, dy);
                }
            }
#endif /* ACTOR_IMPLEMENT_AUTO_ALIGNING_ON_BLOCKED_ENABLED */
        } else {
            moving = TRUE;
            actor_move_in_x_direction(actor, 1);
        }
        actor_play_animation(actor, dir, moving);
    } else if (INPUT_IS_BTN_PRESSED(J_UP)) {
        dir = DIRECTION_UP;
        const UINT8 ep = navigation_get_blocking_up_endpoint(actor, CONTROLLER_NEGATIVE_SPEED_OF(actor));
        if (ep) {
#if ACTOR_IMPLEMENT_AUTO_ALIGNING_ON_BLOCKED_ENABLED
            if (!TOPDOWN_ALIGNED_TO_TILE(actor)) {
                TOPDOWN_ALIGN_TO_NEAREST_TILE(actor, x, old_x, ep);
                if (navigation_get_blocking_up(actor, CONTROLLER_NEGATIVE_SPEED_OF(actor))) {
                    TOPDOWN_REVERT_ALIGNMENT(actor, x, old_x);
                } else {
                    TOPDOWN_DIFF_ALIGNMENT(actor, x, old_x, dx);
                    TOPDOWN_REVERT_ALIGNMENT(actor, x, old_x);
                    moving = TRUE;
#   if ACTOR_IMPLEMENT_AUTO_ALIGNING_WITH_TURN_ENABLED
                    dir = dx < 0 ? DIRECTION_LEFT : DIRECTION_RIGHT;
#   endif /* ACTOR_IMPLEMENT_AUTO_ALIGNING_WITH_TURN_ENABLED */
                    actor_move_in_x_direction(actor, dx);
                }
            }
#endif /* ACTOR_IMPLEMENT_AUTO_ALIGNING_ON_BLOCKED_ENABLED */
        } else {
            moving = TRUE;
            actor_move_in_y_direction(actor, -1);
        }
        actor_play_animation(actor, dir, moving);
    } else if (INPUT_IS_BTN_PRESSED(J_DOWN)) {
        dir = DIRECTION_DOWN;
        const UINT8 ep = navigation_get_blocking_down_endpoint(actor, CONTROLLER_POSITIVE_SPEED_OF(actor));
        if (ep) {
#if ACTOR_IMPLEMENT_AUTO_ALIGNING_ON_BLOCKED_ENABLED
            if (!TOPDOWN_ALIGNED_TO_TILE(actor)) {
                TOPDOWN_ALIGN_TO_NEAREST_TILE(actor, x, old_x, ep);
                if (navigation_get_blocking_down(actor, CONTROLLER_POSITIVE_SPEED_OF(actor))) {
                    TOPDOWN_REVERT_ALIGNMENT(actor, x, old_x);
                } else {
                    TOPDOWN_DIFF_ALIGNMENT(actor, x, old_x, dx);
                    TOPDOWN_REVERT_ALIGNMENT(actor, x, old_x);
                    moving = TRUE;
#   if ACTOR_IMPLEMENT_AUTO_ALIGNING_WITH_TURN_ENABLED
                    dir = dx < 0 ? DIRECTION_LEFT : DIRECTION_RIGHT;
#   endif /* ACTOR_IMPLEMENT_AUTO_ALIGNING_WITH_TURN_ENABLED */
                    actor_move_in_x_direction(actor, dx);
                }
            }
#endif /* ACTOR_IMPLEMENT_AUTO_ALIGNING_ON_BLOCKED_ENABLED */
        } else {
            moving = TRUE;
            actor_move_in_y_direction(actor, 1);
        }
        actor_play_animation(actor, dir, moving);
    }
    if (moving) {
        result = TRUE;
    } else {
        if (CHK_FLAG(actor->motion, ACTOR_MOTION_MOVE)) {
            actor_transfer_animation_to_idle(actor);
            actor_move_stop(actor);
            result = TRUE;
        }
    }

    // Check for actor overlap.
    if (IS_FRAME_ODD) {
        actor_t * hit_actor = actor_hits(&actor->bounds, &actor->position, actor, FALSE);
        if (
            hit_actor &&
            (actor->collision_group & hit_actor->collision_group)              // Same collision group.
        ) {
            actor_fire_collision(actor, hit_actor);
        }
    }

    // Check collision with another actor.
    if (moving) {
        actor_t * hit_actor = actor_in_front_of_actor(actor, 1, TRUE);
        if (hit_actor && controller_is_actor_toward_another(actor, hit_actor)) {
            actor_transfer_animation_to_idle(actor);
            actor_move_stop(actor);
            if (actor->collision_group & hit_actor->collision_group)           // Same collision group.
                actor_fire_collision(actor, hit_actor);
        }
    }

    // Interact with another actor.
    if (INPUT_IS_BTN_UP(TOPDOWN_ACT_BUTTON)) {
        actor_t * hit_actor = actor_in_front_of_actor(actor, 4, TRUE);
        if (
            hit_actor &&
            (actor->collision_group & hit_actor->collision_group) == 0 &&      // Different collision group.
            (actor->hit_handler_bank != 0 || hit_actor->hit_handler_bank != 0) // Has collision handler(s).
        ) {
            actor_play_animation(hit_actor, FLIP_DIRECTION(actor->direction), FALSE);
            actor_transfer_animation_to_idle(hit_actor);
            actor_move_stop(actor);
            actor_begin_hit_thread(actor, hit_actor);
            actor_begin_hit_thread(hit_actor, actor);
        }
    }

    // Finish.
    return result;
}

BOOLEAN controller_behave_topdown_player(actor_t * actor) BANKED {
    // Check whether the actor is aligned to tile.
    BOOLEAN moving = TRUE;
    if (TOPDOWN_ALIGNED_TO_TILE(actor)) { // Aligned to tile.
        // Check for trigger collisions.
#if USE_TRIGGER
        if (trigger_activate_at_intersection(&actor->bounds, &actor->position, FALSE)) {
            if (!TOPDOWN_ALIGNED_TO_TILE(actor)) { // Not aligned to tile.
                actor->behaviour = CONTROLLER_BEHAVIOUR_TOPDOWN_PLAYER_ARBITRARY; // Transfer to the arbitrary behaviour.
            }
            CLR_FLAG(actor->motion, ACTOR_MOTION_MOVE);

            return FALSE;
        }
#endif /* USE_TRIGGER */

        // Update the actor.
        moving = controller_behave_topdown_player_update(actor);
    }

    // Check whether the camera need to be moved.
    if (moving && actor == actor_following_target)
        return TRUE;

    return FALSE;
}

BOOLEAN controller_behave_topdown_player_arbitrary(actor_t * actor, UINT8 rigid) BANKED {
    // Check for trigger collisions.
#if USE_TRIGGER
    if (trigger_activate_at_intersection(&actor->bounds, &actor->position, FALSE))
        return FALSE;
#endif /* USE_TRIGGER */

    // Update the actor.
    BOOLEAN moving = controller_behave_topdown_player_update(actor);

    // Check whether the actor is right aligned to tile.
    if (TOPDOWN_ALIGNED_TO_TILE(actor)) { // Aligned to tile.
        if (!rigid)
            actor->behaviour = CONTROLLER_BEHAVIOUR_TOPDOWN_PLAYER; // Transfer to the aligned behaviour.
    }

    // Check whether the camera need to be moved.
    if (moving && actor == actor_following_target)
        return TRUE;

    return FALSE;
}

STATIC BOOLEAN controller_behave_topdown_move_update(actor_t * actor, UINT8 forward) {
    // Prepare.
    BOOLEAN result = FALSE;
    BOOLEAN moving = FALSE;

    // Move and animate.
    switch (actor->direction) {
    // Move in the y-axis.
    case DIRECTION_UP:
        if (!navigation_get_blocking_up(actor, CONTROLLER_NEGATIVE_SPEED_OF(actor))) {
            moving = TRUE;
            actor_move_in_y_direction(actor, -1);
            actor_play_animation(actor, DIRECTION_UP, TRUE);
        }

        break;
    case DIRECTION_DOWN:
        if (!navigation_get_blocking_down(actor, CONTROLLER_POSITIVE_SPEED_OF(actor))) {
            moving = TRUE;
            actor_move_in_y_direction(actor, 1);
            actor_play_animation(actor, DIRECTION_DOWN, TRUE);
        }

        break;

    // Move in the x-axis.
    case DIRECTION_LEFT:
        if (!navigation_get_blocking_left(actor, CONTROLLER_NEGATIVE_SPEED_OF(actor))) {
            moving = TRUE;
            actor_move_in_x_direction(actor, -1);
            actor_play_animation(actor, DIRECTION_LEFT, TRUE);
        }

        break;
    case DIRECTION_RIGHT:
        if (!navigation_get_blocking_right(actor, CONTROLLER_POSITIVE_SPEED_OF(actor))) {
            moving = TRUE;
            actor_move_in_x_direction(actor, 1);
            actor_play_animation(actor, DIRECTION_RIGHT, TRUE);
        }

        break;
    }
    if (moving) {
        result = TRUE;
    } else {
        if (CHK_FLAG(actor->motion, ACTOR_MOTION_MOVE)) {
            actor_transfer_animation_to_idle(actor);
            actor_move_stop(actor);
            result = TRUE;
        }
    }

    // Check collision with another actor.
    if (moving) {
        actor_t * hit_actor = actor_in_front_of_actor(actor, forward, FALSE);
        if (hit_actor && controller_is_actor_toward_another(actor, hit_actor)) {
            actor_transfer_animation_to_idle(actor);
            actor_move_stop(actor);
        }
    }

    // Finish.
    return result;
}

BOOLEAN controller_behave_topdown_move(actor_t * actor) BANKED {
    // Check whether the actor is aligned to tile.
    BOOLEAN moving = TRUE;
    if (TOPDOWN_ALIGNED_TO_TILE(actor)) { // Aligned to tile.
        // Update the actor.
        moving = controller_behave_topdown_move_update(actor, 0);
    }

    // Check whether the camera need to be moved.
    if (moving && actor == actor_following_target)
        return TRUE;

    return FALSE;
}

BOOLEAN controller_behave_topdown_move_arbitrary(actor_t * actor, UINT8 rigid) BANKED {
    // Update the actor.
    BOOLEAN moving = controller_behave_topdown_move_update(actor, rigid ? 1 : 0);

    // Check whether the actor is right aligned to tile.
    if (TOPDOWN_ALIGNED_TO_TILE(actor)) { // Aligned to tile.
        if (!rigid)
            actor->behaviour = CONTROLLER_BEHAVIOUR_TOPDOWN_MOVE; // Transfer to the aligned behaviour.
    }

    // Check whether the camera need to be moved.
    if (moving && actor == actor_following_target)
        return TRUE;

    return FALSE;
}

BOOLEAN controller_behave_topdown_idle(actor_t * actor) BANKED {
    // Prepare.
    if (!IS_FRAME_ODD)
        return FALSE;

    // Animate.
    switch (actor->direction) {
    case DIRECTION_UP:
        actor_play_animation(actor, DIRECTION_UP, FALSE);

        break;
    case DIRECTION_DOWN:
        actor_play_animation(actor, DIRECTION_DOWN, FALSE);

        break;
    case DIRECTION_LEFT:
        actor_play_animation(actor, DIRECTION_LEFT, FALSE);

        break;
    case DIRECTION_RIGHT:
        actor_play_animation(actor, DIRECTION_RIGHT, FALSE);

        break;
    }

    // Check whether the camera need to be moved.
    if (actor == actor_following_target)
        return TRUE;

    return FALSE;
}

#endif /* USE_TOPDOWN */
