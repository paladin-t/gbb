#pragma bank 255

#if defined __SDCC
#   pragma disable_warning 94
#   pragma disable_warning 110
#   pragma disable_warning 126
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "../vm_input.h"
#include "../vm_scene.h"
#include "../vm_trigger.h"

#include "navigation.h"
#include "platformer.h"

#if USE_PLATFORMER

#define PLATFORMER_ACT_BUTTON    J_A
#define PLATFORMER_JUMP_BUTTON   J_A
#define PLATFORMER_RUN_BUTTON    J_B

#define PLATFORMER_SPEED_UP      16

BOOLEAN controller_behave_platformer_player(actor_t * actor) BANKED {
    // Prepare.
    BOOLEAN moving = FALSE;

    // Check for trigger collisions.
#if USE_TRIGGER
    if (trigger_activate_at_intersection(&actor->bounds, &actor->position, FALSE))
        return FALSE;
#endif /* USE_TRIGGER */

    // Move, walk and climb.
    BOOLEAN grounded = FALSE;
    if (scene.player_on_ladder) {
        // Move on a ladder.
        const UINT8 actor_half_width = DIV2(actor->bounds.right - actor->bounds.left);
        const UINT8 x_mid = DIV8(TO_SCREEN(actor->position.x) + actor->bounds.left + actor_half_width);
        SCENE_CLEAR_PLAYER_VELOCITY_Y;
        if (INPUT_IS_BTN_PRESSED(J_UP)) {
            if (navigation_get_ladder_up(actor, x_mid)) {
                SCENE_APPLY_PLAYER_VELOCITY_Y(-DIV2(scene.climb_velocity));
                actor_play_animation(actor, DIRECTION_UP, TRUE);
                moving = TRUE;
            }
        } else if (INPUT_IS_BTN_PRESSED(J_DOWN)) {
            if (navigation_get_ladder_down(actor, x_mid)) {
                SCENE_APPLY_PLAYER_VELOCITY_Y(scene.climb_velocity);
                actor_play_animation(actor, DIRECTION_DOWN, TRUE);
                moving = TRUE;
            }
        } else if (INPUT_IS_BTN_PRESSED(J_LEFT)) {
            if (!navigation_get_ladder_blocking_left(actor, x_mid)) {
                scene.player_on_ladder = FALSE;
                if (!navigation_get_blocking_left(actor, DIV16(-actor->move_speed))) {
                    actor_reserve_move_speed(actor, PLATFORMER_SPEED_UP);
                    actor_move_in_direction(actor, -1, actor->relative_movement.y);
                    actor_play_animation(actor, DIRECTION_LEFT, TRUE);
                    moving = TRUE;
                }
            }
        } else if (INPUT_IS_BTN_PRESSED(J_RIGHT)) {
            if (!navigation_get_ladder_blocking_right(actor, x_mid)) {
                scene.player_on_ladder = FALSE;
                if (!navigation_get_blocking_right(actor, DIV16(actor->move_speed))) {
                    actor_reserve_move_speed(actor, PLATFORMER_SPEED_UP);
                    actor_move_in_direction(actor, 1, actor->relative_movement.y);
                    actor_play_animation(actor, DIRECTION_RIGHT, TRUE);
                    moving = TRUE;
                }
            }
        } else {
            actor_transfer_animation_to_idle(actor);
        }

        // Move in the y-axis.
        if (scene.player_velocity_y < 0) {
            UINT16 y;
            if (navigation_get_ladder_blocking_up_pos(actor, x_mid, DIV16(scene.player_velocity_y), &y)) {
                SCENE_CLEAR_PLAYER_VELOCITY_Y;
            }
            y = FROM_SCREEN(y);
            if (actor->position.y != y) {
                moving = TRUE;
                actor->position.y = y;
            }
        } else if (scene.player_velocity_y > 0) {
            UINT16 y;
            if (navigation_get_ladder_blocking_down_pos(actor, x_mid, DIV16(scene.player_velocity_y), &y)) {
                SCENE_CLEAR_PLAYER_VELOCITY_Y;
            }
            y = FROM_SCREEN(y);
            if (actor->position.y != y) {
                moving = TRUE;
                actor->position.y = y;
            }
        }
    } else {
        // Up, grab a ladder.
        if (INPUT_IS_BTN_PRESSED(J_UP)) {
            const UINT8 actor_half_width = DIV2(actor->bounds.right - actor->bounds.left);
            UINT16 x;
            if (navigation_find_ladder_up(actor, actor_half_width, &x)) {
                SCENE_RESET_PLAYER_JUMP_COUNT;
                SCENE_RESET_PLAYER_JUMP_TICKS;
                SCENE_CLEAR_PLAYER_VELOCITY_Y;
                actor->position.x = FROM_SCREEN(x);
                CLR_FLAG(actor->motion, ACTOR_MOTION_MOVE_X);
                scene.player_on_ladder = TRUE;
                actor_restore_move_speed(actor);
                moving = TRUE;

                goto end;
            }
        }

        // Down, grab a ladder.
        if (INPUT_IS_BTN_PRESSED(J_DOWN)) {
            const UINT8 actor_half_width = DIV2(actor->bounds.right - actor->bounds.left);
            UINT16 x;
            if (navigation_find_ladder_down(actor, actor_half_width, &x)) {
                SCENE_RESET_PLAYER_JUMP_COUNT;
                SCENE_RESET_PLAYER_JUMP_TICKS;
                SCENE_CLEAR_PLAYER_VELOCITY_Y;
                actor->position.x = FROM_SCREEN(x);
                CLR_FLAG(actor->motion, ACTOR_MOTION_MOVE_X);
                scene.player_on_ladder = TRUE;
                actor_restore_move_speed(actor);
                moving = TRUE;

                goto end;
            }
        }

        // Right.
        if (INPUT_IS_BTN_PRESSED(J_RIGHT) && !CHK_FLAG(actor->motion, ACTOR_MOTION_MOVE_RIGHT)) {
            if (!navigation_get_blocking_right(actor, DIV16(actor->move_speed))) {
                actor_reserve_move_speed(actor, PLATFORMER_SPEED_UP);
                actor_move_in_direction(actor, 1, actor->relative_movement.y);
                moving = TRUE;
            }
            actor_play_animation(actor, DIRECTION_RIGHT, TRUE);
        } else if (INPUT_IS_BTN_UP(J_RIGHT)) {
            actor_restore_move_speed(actor);
            if (CHK_FLAG(actor->motion, ACTOR_MOTION_MOVE_Y)) {
                actor_move_in_y_direction(actor, actor->relative_movement.y);
            } else {
                actor_move_stop(actor);
            }
            actor_play_animation(actor, DIRECTION_RIGHT, FALSE);
            moving = TRUE;
        } else if (CHK_FLAG(actor->motion, ACTOR_MOTION_MOVE_RIGHT)) {
            if (navigation_get_blocking_right(actor, DIV16(actor->move_speed))) {
                actor_restore_move_speed(actor);
                actor_move_stop(actor);
            } else {
                moving = TRUE;
            }
        }
        if (moving) goto run;

        // Left.
        if (INPUT_IS_BTN_PRESSED(J_LEFT) && !CHK_FLAG(actor->motion, ACTOR_MOTION_MOVE_LEFT)) {
            if (!navigation_get_blocking_left(actor, DIV16(-actor->move_speed))) {
                actor_reserve_move_speed(actor, PLATFORMER_SPEED_UP);
                actor_move_in_direction(actor, -1, actor->relative_movement.y);
                moving = TRUE;
            }
            actor_play_animation(actor, DIRECTION_LEFT, TRUE);
        } else if (INPUT_IS_BTN_UP(J_LEFT)) {
            actor_restore_move_speed(actor);
            if (CHK_FLAG(actor->motion, ACTOR_MOTION_MOVE_Y)) {
                actor_move_in_y_direction(actor, actor->relative_movement.y);
            } else {
                actor_move_stop(actor);
            }
            actor_play_animation(actor, DIRECTION_LEFT, FALSE);
            moving = TRUE;
        } else if (CHK_FLAG(actor->motion, ACTOR_MOTION_MOVE_LEFT)) {
            if (navigation_get_blocking_left(actor, DIV16(-actor->move_speed))) {
                actor_restore_move_speed(actor);
                actor_move_stop(actor);
            } else {
                moving = TRUE;
            }
        }

run:
        // Run.
        if (INPUT_IS_BTN_PRESSED(PLATFORMER_RUN_BUTTON) && CHK_FLAG(actor->motion, ACTOR_MOTION_MOVE_X)) {
            if (IS_FRAME_4)
                actor_speed_up(actor);
        } else {
            if (IS_FRAME_2)
                actor_slow_down(actor);
        }

        // Move in the y-axis.
        if (scene.player_velocity_y < 0) { // Jump.
            UINT16 y;
            if (navigation_get_blocking_up_pos(actor, DIV16(scene.player_velocity_y), &y)) {
                // Hit something.
                SCENE_CLEAR_PLAYER_JUMP_COUNT;
                SCENE_CLEAR_PLAYER_JUMP_TICKS;
                SCENE_CLEAR_PLAYER_VELOCITY_Y;
            }
            y = FROM_SCREEN(y);
            if (actor->position.y != y) {
                moving = TRUE;
                actor->position.y = y;
            }
        } else if (scene.player_velocity_y > 0) { // Fall.
            UINT16 y;
            if (navigation_get_blocking_fall_pos(actor, DIV16(scene.player_velocity_y), &y)) {
                // On ground.
                grounded = TRUE;
                if (scene.player_can_jump != scene.jump_max_count) { // Jumped.
                    SCENE_RESET_PLAYER_JUMP_COUNT;
                    SCENE_RESET_PLAYER_JUMP_TICKS;
                    SCENE_CLEAR_PLAYER_VELOCITY_Y;
                }
            }
            y = FROM_SCREEN(y);
            if (actor->position.y != y) {
                moving = TRUE;
                actor->position.y = y;
            }
        }
    }

    // Interact with another actor.
    BOOLEAN can_jump = TRUE;
    actor_t * hit_actor = actor_hits(&actor->bounds, &actor->position, actor, FALSE);
    if (hit_actor) {
        if (
            (actor->collision_group & hit_actor->collision_group) &&           // Same collision group.
            (actor->hit_handler_bank != 0 || hit_actor->hit_handler_bank != 0) // Has collision handler(s).
        ) {
            actor_restore_move_speed(actor);
            actor_fire_collision(actor, hit_actor);
        }
    } else if (INPUT_IS_BTN_DOWN(PLATFORMER_ACT_BUTTON)) {
        hit_actor = actor_in_front_of_actor(actor, 2, TRUE);
        if (
            hit_actor &&
            (actor->collision_group & hit_actor->collision_group) == 0 &&      // Different collision group.
            (actor->hit_handler_bank != 0 || hit_actor->hit_handler_bank != 0) // Has collision handler(s).
        ) {
            actor_restore_move_speed(actor);
            actor_begin_hit_thread(actor, hit_actor);
            actor_begin_hit_thread(hit_actor, actor);
            can_jump = FALSE;
        }
    }

    // Jump.
    if (INPUT_IS_BTN_DOWN(PLATFORMER_JUMP_BUTTON)) {                           // Pressed down the jump button.
        if (can_jump && scene.player_can_jump > 0) {
            --scene.player_can_jump;
            scene.player_on_ladder = FALSE;
            SCENE_RESET_PLAYER_JUMP_TICKS;
            SCENE_APPLY_PLAYER_VELOCITY_Y(-scene.jump_gravity);
        }
    } else if (INPUT_IS_BTN_UP(PLATFORMER_JUMP_BUTTON)) {                      // Released the jump button.
        SCENE_CLEAR_PLAYER_JUMP_TICKS;
    } else if (
        INPUT_IS_BTN_PRESSED(PLATFORMER_JUMP_BUTTON) &&                        // The jump button is being held.
        can_jump &&
        scene.player_can_jump < scene.jump_max_count &&                        // In air.
        scene.player_jump_ticks > 0                                            // Can still go upward.
    ) {
        --scene.player_jump_ticks;
        SCENE_APPLY_PLAYER_VELOCITY_Y(-scene.jump_gravity);
    } else {                                                                   // No jump.
        if (!grounded) {
            SCENE_APPLY_PLAYER_VELOCITY_Y(scene.player_velocity_y + scene.gravity);
        }
    }

    // Check whether the camera need to be moved.
end:
    if (moving && actor == actor_following_target)
        return TRUE;

    return FALSE;
}

BOOLEAN controller_behave_platformer_move(actor_t * actor) BANKED {
    // Prepare.
    BOOLEAN moving = FALSE;

    // Apply gravity.
    UINT16 y;
    navigation_get_blocking_down_pos(actor, MAX(DIV2(scene.gravity), 1), &y);
    y = FROM_SCREEN(y);
    if (actor->position.y != y) {
        moving = TRUE;
        actor->position.y = y;
    }

    // Move in the x-axis.
    switch (actor->direction) {
    case DIRECTION_LEFT:
        if (navigation_get_blocking_left(actor, DIV16(-actor->move_speed))) {
            if (CHK_FLAG(actor->motion, ACTOR_MOTION_MOVE_Y)) {
                actor_move_in_y_direction(actor, actor->relative_movement.y);
                moving = TRUE;
            } else {
                actor_move_stop(actor);
            }
        } else {
            actor_move_in_x_direction(actor, -1);
            moving = TRUE;
        }
        actor_play_animation(actor, DIRECTION_LEFT, TRUE);

        break;
    case DIRECTION_RIGHT:
        if (navigation_get_blocking_right(actor, DIV16(actor->move_speed))) {
            if (CHK_FLAG(actor->motion, ACTOR_MOTION_MOVE_Y)) {
                actor_move_in_y_direction(actor, actor->relative_movement.y);
                moving = TRUE;
            } else {
                actor_move_stop(actor);
            }
        } else {
            actor_move_in_x_direction(actor, 1);
            moving = TRUE;
        }
        actor_play_animation(actor, DIRECTION_RIGHT, TRUE);

        break;
    }

    // Check whether the camera need to be moved.
    if (moving && actor == actor_following_target)
        return TRUE;

    return FALSE;
}

BOOLEAN controller_behave_platformer_idle(actor_t * actor) BANKED {
    // Prepare.
    BOOLEAN moving = FALSE;

    // Apply gravity.
    UINT16 y;
    navigation_get_blocking_down_pos(actor, MAX(DIV2(scene.gravity), 1), &y);
    y = FROM_SCREEN(y);
    if (actor->position.y != y) {
        moving = TRUE;
        actor->position.y = y;
    }

    // Animate.
    switch (actor->direction) {
    case DIRECTION_LEFT:
        actor_play_animation(actor, DIRECTION_LEFT, FALSE);

        break;
    case DIRECTION_RIGHT:
        actor_play_animation(actor, DIRECTION_RIGHT, FALSE);

        break;
    }

    // Check whether the camera need to be moved.
    if (moving && actor == actor_following_target)
        return TRUE;

    return FALSE;
}

#endif /* USE_PLATFORMER */
