#pragma bank 255

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include <stdlib.h>

#include "controller.h"
#include "navigation.h"

INLINE BOOLEAN controller_is_actor_movable_with_gravity(actor_t * actor, INT8 dx, UINT8 flags) {
    // Prepare.
    BOOLEAN movable = FALSE;

    // Apply gravity if the flag is enabled.
    if (flags == CONTROLLER_MOVABLE_FLAG_FULL) {
        UINT16 y;
        navigation_get_blocking_down_pos(actor, MAX(DIV2(scene.gravity), 1), &y);
        y = FROM_SCREEN(y);
        if (actor->position.y != y) {
            movable = TRUE;
            actor->position.y = y;
        }
    }

    // Move in the x-axis.
    if (dx < 0) {
        if (navigation_get_blocking_left(actor, DIV16(-actor->move_speed))) {
            if (CHK_FLAG(actor->motion, ACTOR_MOTION_MOVE_Y)) {
                movable = TRUE;
            } else {
                actor_transfer_animation_to_idle(actor);
                actor_move_stop(actor);
            }
        } else {
            movable = TRUE;
        }
    } else if (dx > 0) {
        if (navigation_get_blocking_right(actor, DIV16(actor->move_speed))) {
            if (CHK_FLAG(actor->motion, ACTOR_MOTION_MOVE_Y)) {
                movable = TRUE;
            } else {
                actor_transfer_animation_to_idle(actor);
                actor_move_stop(actor);
            }
        } else {
            movable = TRUE;
        }
    }

    // Finish.
    return movable;
}

INLINE BOOLEAN controller_is_actor_movable_without_gravity(actor_t * actor, INT8 dx, INT8 dy) {
    // Prepare.
    BOOLEAN movable = FALSE;

    // Move in the y-axis.
    if (dy < 0) {
        if (!navigation_get_blocking_up(actor, CONTROLLER_NEGATIVE_SPEED_OF(actor)))
            movable = TRUE;
    } else if (dy > 0) {
        if (!navigation_get_blocking_down(actor, CONTROLLER_POSITIVE_SPEED_OF(actor)))
            movable = TRUE;
    }
    // Move in the x-axis.
    else if (dx < 0) {
        if (!navigation_get_blocking_left(actor, CONTROLLER_NEGATIVE_SPEED_OF(actor)))
            movable = TRUE;
    } else if (dx > 0) {
        if (!navigation_get_blocking_right(actor, CONTROLLER_POSITIVE_SPEED_OF(actor)))
            movable = TRUE;
    }

    if (!movable) {
        if (CHK_FLAG(actor->motion, ACTOR_MOTION_MOVE)) {
            actor_transfer_animation_to_idle(actor);
            actor_move_stop(actor);
            movable = TRUE;
        }
    }

    // Check collision with another actor.
    if (movable) {
        actor_t * hit_actor = actor_in_front_of_actor(actor, 1, FALSE);
        if (hit_actor && controller_is_actor_toward_another(actor, hit_actor)) {
            actor_transfer_animation_to_idle(actor);
            actor_move_stop(actor);
        }
    }

    // Finish.
    return movable;
}

BOOLEAN controller_is_actor_movable(actor_t * actor, INT8 dx, INT8 dy, UINT8 flags) BANKED {
    if (scene.gravity)
        return controller_is_actor_movable_with_gravity(actor, dx, flags);
    else
        return controller_is_actor_movable_without_gravity(actor, dx, dy);
}

BOOLEAN controller_is_actor_toward_another(actor_t * actor, actor_t * other) BANKED {
    const INT16 dx = other->position.x - actor->position.x;
    const INT16 dy = other->position.y - actor->position.y;
    if (CHK_FLAG(actor->motion, ACTOR_MOTION_MOVE_LEFT) && (dx < 0)) {
        if (abs(dx) > abs(dy))
            return TRUE;
    } else if (CHK_FLAG(actor->motion, ACTOR_MOTION_MOVE_RIGHT) && (dx > 0)) {
        if (dx > abs(dy))
            return TRUE;
    } else if (CHK_FLAG(actor->motion, ACTOR_MOTION_MOVE_UP) && (dy < 0)) {
        if (abs(dy) > abs(dx))
            return TRUE;
    } else if (CHK_FLAG(actor->motion, ACTOR_MOTION_MOVE_DOWN) && (dy > 0)) {
        if (dy > abs(dx))
            return TRUE;
    }

    return FALSE;
}
