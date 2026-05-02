#pragma bank 255

#if defined __SDCC
#   pragma disable_warning 110
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
#include "shooting.h"

#if defined USE_SHOOTING
#define SHOOTING_ACT_BUTTON   J_A

UINT8 shooter_direction;
UINT8 shooter_scroll_speed = 16;
UINT16 shooter_dest;
BOOLEAN shooter_reached_end;

// Initializes the shooting controller.
BOOLEAN def_shooting(POINTER THIS, BOOLEAN start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    (void)start;
    (void)stack_frame;

    SCRIPT_CTX * THIS_ = (SCRIPT_CTX *)THIS;
    const UINT8 dir    = (UINT8)*(THIS_->stack_ptr - 1);
    const UINT8 speed  = (UINT8)*(THIS_->stack_ptr - 2);
    const UINT16 dest  = (UINT16)*(THIS_->stack_ptr - 3);

    shooter_direction = dir;
    shooter_scroll_speed = speed;
    shooter_dest = FROM_SCREEN(dest);
    shooter_reached_end = FALSE;

    return TRUE;
}

BOOLEAN controller_behave_shooting_player(actor_t * actor) BANKED {
    // Prepare.
    BOOLEAN moving = FALSE;
    UINT8 new_dir = DIRECTION_NONE;

    // Check input to set actor movement.
    if (IS_DIRECTION_HORIZONTAL(shooter_direction)) {
        if (INPUT_IS_BTN_PRESSED(J_UP)) {
            moving = TRUE;
            new_dir = DIRECTION_UP;
        } else if (INPUT_IS_BTN_PRESSED(J_DOWN)) {
            moving = TRUE;
            new_dir = DIRECTION_DOWN;
        } else {
            new_dir = shooter_direction;
        }
    } else /* if (IS_DIRECTION_VERTICAL(shooter_direction)) */ {
        if (INPUT_IS_BTN_PRESSED(J_LEFT)) {
            moving = TRUE;
            new_dir = DIRECTION_LEFT;
        } else if (INPUT_IS_BTN_PRESSED(J_RIGHT)) {
            moving = TRUE;
            new_dir = DIRECTION_RIGHT;
        } else {
            new_dir = shooter_direction;
        }
    }

    // Set animation if direction has changed.
    if (new_dir != actor->direction) {
        actor_play_animation(actor, new_dir, moving);
    }

    // Move the actor.
    if (moving) {
        upoint16_t new_pos;
        new_pos.x = actor->position.x;
        new_pos.y = actor->position.y;
        point_translate_dir(&new_pos, actor->direction, actor->move_speed);
        if (IS_DIRECTION_HORIZONTAL(shooter_direction)) {
            UINT16 y;
            if (actor->direction == DIRECTION_DOWN) {
                navigation_get_blocking_down_pos_in_scene(actor, DIV16(actor->move_speed), &y);
            } else /* if (actor->direction == DIRECTION_UP) */ {
                navigation_get_blocking_up_pos_in_scene(actor, DIV16(-actor->move_speed), &y);
            }
            y = FROM_SCREEN(y);
            if (actor->position.y != y) {
                actor->position.y = y;
            }
        } else /* if (IS_DIRECTION_VERTICAL(shooter_direction)) */ {
            UINT16 x;
            if (actor->direction == DIRECTION_RIGHT) {
                navigation_get_blocking_right_pos_in_scene(actor, DIV16(actor->move_speed), &x);
            } else /* if (actor->direction == DIRECTION_LEFT) */ {
                navigation_get_blocking_left_pos_in_scene(actor, DIV16(-actor->move_speed), &x);
            }
            x = FROM_SCREEN(x);
            if (actor->position.x != x) {
                actor->position.x = x;
            }
        }
    }

    // Scroll the background automatically.
    if (!shooter_reached_end) {
        moving = TRUE;
        point_translate_dir(&actor->position, shooter_direction, shooter_scroll_speed);

        // Check whether has reached the end of screen.
        if ((shooter_direction == DIRECTION_RIGHT) && (actor->position.x > shooter_dest)) {
            actor->position.x = shooter_dest;
            shooter_reached_end = TRUE;
        } else if ((shooter_direction == DIRECTION_LEFT) && (actor->position.x < shooter_dest)) {
            actor->position.x = shooter_dest;
            shooter_reached_end = TRUE;
        } else if ((shooter_direction == DIRECTION_DOWN) && (actor->position.y > shooter_dest)) {
            actor->position.y = shooter_dest;
            shooter_reached_end = TRUE;
        } else if ((shooter_direction == DIRECTION_UP) && (actor->position.y < shooter_dest)) {
            actor->position.y = shooter_dest;
            shooter_reached_end = TRUE;
        }
    }

    if (IS_FRAME_ODD) {
        // Check for trigger collisions.
        if (trigger_activate_at_intersection(&actor->bounds, &actor->position, FALSE))
            return FALSE;

        // Check for actor collisions.
        actor_t * hit_actor = actor_hits(&actor->bounds, &actor->position, actor, FALSE);
        if (
            hit_actor &&
            (actor->collision_group & hit_actor->collision_group)                  // Same collision group.
        ) {
            actor_fire_collision(actor, hit_actor);
        } else if (INPUT_IS_BTN_UP(SHOOTING_ACT_BUTTON)) {
            if (!hit_actor) {
                hit_actor = actor_in_front_of_actor(actor, 4, TRUE);
            }
            if (
                hit_actor &&
                (actor->collision_group & hit_actor->collision_group) == 0 &&      // Different collision group.
                (actor->hit_handler_bank != 0 || hit_actor->hit_handler_bank != 0) // Has collision handler(s).
            ) {
                actor_begin_hit_thread(actor, hit_actor);
                actor_begin_hit_thread(hit_actor, actor);
            }
        }
    }

    // Check whether the camera need to be moved.
    if (moving && actor == actor_following_target)
        return TRUE;

    return FALSE;
}

BOOLEAN controller_behave_shooting_move(actor_t * actor) BANKED {
    // Prepare.
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
    if (!moving) {
        if (CHK_FLAG(actor->motion, ACTOR_MOTION_MOVE)) {
            actor_transfer_animation_to_idle(actor);
            actor_move_stop(actor);
        }
    }

    // Check whether the camera need to be moved.
    if (moving && actor == actor_following_target)
        return TRUE;

    return FALSE;
}
#endif /* USE_SHOOTING */
