#pragma bank 255

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include <stdlib.h>

#include "../vm_input.h"
#include "../vm_scene.h"
#include "../vm_trigger.h"

#include "controller.h"
#include "pointnclick.h"

#if USE_POINTNCLICK

#define POINTNCLICK_ACT_BUTTON                J_A

#define POINTNCLICK_ANIMATION_CURSOR          0
#define POINTNCLICK_ANIMATION_CURSOR_HOVER    1

#define POINTNCLICK_MOVING_MODE_NONE          0
#define POINTNCLICK_MOVING_MODE_WITH_ANGLE    1
#define POINTNCLICK_MOVING_MODE_IMMEDIATELY   2

STATIC UINT8 controller_behave_pointnclick_mouse(actor_t * actor, UINT8 * angle, BOOLEAN * confirmed) {
    UINT8 moving = POINTNCLICK_MOVING_MODE_NONE;

    do {
        const UINT8 x = INPUT_TOUCH_X;
        const UINT8 y = INPUT_TOUCH_Y;
        if (x == 0xFF /* || y == 0xFF */)
            break;

        const INT16 px = (INT16)(TO_SCREEN(actor->position.x) - scene_camera_x);
        const INT16 py = (INT16)(TO_SCREEN(actor->position.y) - scene_camera_y);
        const INT16 dx = (INT16)x - px;
        const INT16 dy = (INT16)y - py;
        if (abs(dx) >= abs(dy)) {
            if (dx < 0) {
                if (dy < 0)
                    *angle = ANGLE_315DEG;
                else if (dy > 0)
                    *angle = ANGLE_225DEG;
                else
                    *angle = ANGLE_270DEG;
                moving = POINTNCLICK_MOVING_MODE_WITH_ANGLE;
            } else if (dx > 0) {
                if (dy < 0)
                    *angle = ANGLE_45DEG;
                else if (dy > 0)
                    *angle = ANGLE_135DEG;
                else
                    *angle = ANGLE_90DEG;
                moving = POINTNCLICK_MOVING_MODE_WITH_ANGLE;
            }
        } else {
            if (dy < 0) {
                *angle = ANGLE_0DEG;
                moving = POINTNCLICK_MOVING_MODE_WITH_ANGLE;
            } else if (dy > 0) {
                *angle = ANGLE_180DEG;
                moving = POINTNCLICK_MOVING_MODE_WITH_ANGLE;
            }
        }

        if (!(INPUT_IS_TOUCH_UP & TOUCH_BUTTON_0))
            break;

        *confirmed = TRUE;
    } while (0);

    return moving;
}

STATIC UINT8 controller_behave_pointnclick_touch(actor_t * actor, UINT16 * out_x, UINT16 * out_y, BOOLEAN * confirmed) {
    UINT8 moving = POINTNCLICK_MOVING_MODE_NONE;

    do {
        const UINT8 x = INPUT_TOUCH_X;
        const UINT8 y = INPUT_TOUCH_Y;
        if (x == 0xFF /* || y == 0xFF */)
            break;

        const INT16 px = (INT16)(TO_SCREEN(actor->position.x) - scene_camera_x);
        const INT16 py = (INT16)(TO_SCREEN(actor->position.y) - scene_camera_y);
        if (x != px || y != py) {
            *out_x = FROM_SCREEN((UINT16)x + scene_camera_x);
            *out_y = FROM_SCREEN((UINT16)y + scene_camera_y);
            moving = POINTNCLICK_MOVING_MODE_IMMEDIATELY;
        }

        if (!(INPUT_IS_TOUCH_UP & TOUCH_BUTTON_0))
            break;

        *confirmed = TRUE;
    } while (0);

    return moving;
}

BOOLEAN controller_behave_pointnclick_player(actor_t * actor, UINT8 pointing) BANKED {
    // Prepare.
    UINT8 angle = ANGLE_0DEG;
    UINT8 moving;

    // Check touch input.
    BOOLEAN confirmed = FALSE;
    if (pointing == CONTROLLER_BEHAVIOUR_POINTNCLICK_PLAYER) {
        moving = POINTNCLICK_MOVING_MODE_NONE;
    } else if (device_type & DEVICE_TYPE_WITH_TOUCH_SUPPORT) { // Ignore touch handling if touch feature is not supported.
        if (pointing == CONTROLLER_BEHAVIOUR_POINTNCLICK_PLAYER_WITH_MOUSE)
            moving = controller_behave_pointnclick_mouse(actor, &angle, &confirmed);
        else /* if (pointing == CONTROLLER_BEHAVIOUR_POINTNCLICK_PLAYER_WITH_TOUCH) */
            moving = controller_behave_pointnclick_touch(actor, &actor->position.x, &actor->position.y, &confirmed);
    } else {
        moving = POINTNCLICK_MOVING_MODE_NONE;
    }

    // Check input and move the cursor.
    if (!moving) {
        if (INPUT_IS_BTN_PRESSED(J_LEFT)) {
            if (INPUT_IS_BTN_PRESSED(J_UP))
                angle = ANGLE_315DEG;
            else if (INPUT_IS_BTN_PRESSED(J_DOWN))
                angle = ANGLE_225DEG;
            else
                angle = ANGLE_270DEG;
            moving = POINTNCLICK_MOVING_MODE_WITH_ANGLE;
        } else if (INPUT_IS_BTN_PRESSED(J_RIGHT)) {
            if (INPUT_IS_BTN_PRESSED(J_UP))
                angle = ANGLE_45DEG;
            else if (INPUT_IS_BTN_PRESSED(J_DOWN))
                angle = ANGLE_135DEG;
            else
                angle = ANGLE_90DEG;
            moving = POINTNCLICK_MOVING_MODE_WITH_ANGLE;
        } else if (INPUT_IS_BTN_PRESSED(J_UP)) {
            angle = ANGLE_0DEG;
            moving = POINTNCLICK_MOVING_MODE_WITH_ANGLE;
        } else if (INPUT_IS_BTN_PRESSED(J_DOWN)) {
            angle = ANGLE_180DEG;
            moving = POINTNCLICK_MOVING_MODE_WITH_ANGLE;
        }
    }
    if (moving == POINTNCLICK_MOVING_MODE_WITH_ANGLE) {
        point_translate_angle(&actor->position, angle, actor->move_speed);
    }

    if (moving) {
        if (scene.width != 0) {
            const UINT16 x = TO_SCREEN(actor->position.x);
            const UINT16 scene_width = MUL8(scene.width);
            if (x + actor->bounds.left > scene_width) {
                actor->position.x = -FROM_SCREEN(actor->bounds.left);
            } else if (x + actor->bounds.right >= scene_width) {
                actor->position.x = FROM_SCREEN(scene_width - actor->bounds.right - 1);
            }
        }
        if (scene.height != 0) {
            const UINT16 y = TO_SCREEN(actor->position.y);
            const UINT16 scene_height = MUL8(scene.height);
            if (y + actor->bounds.top > scene_height) {
                actor->position.y = -FROM_SCREEN(actor->bounds.top);
            } else if (y + actor->bounds.bottom >= scene_height) {
                actor->position.y = FROM_SCREEN(scene_height - actor->bounds.bottom - 1);
            }
        }
    }

#if USE_TRIGGER
    // Check for trigger collisions.
    const UINT8 hit_trigger = trigger_at_intersection(&actor->bounds, &actor->position);
    const BOOLEAN is_hover_trigger = hit_trigger != TRIGGER_NONE && triggers[hit_trigger].hit_handler_bank && triggers[hit_trigger].hit_handler_flags & TRIGGER_HAS_ENTER_SCRIPT;

    // Check collision with another actor.
    actor_t * hit_actor = actor_in_front_of_actor(actor, 0, FALSE);
    const BOOLEAN is_hover_actor = hit_actor && hit_actor->hit_handler_bank != 0;

    // Set cursor animation.
    if (is_hover_trigger || is_hover_actor) {
        actor_set_animation(actor, POINTNCLICK_ANIMATION_CURSOR_HOVER);
    } else {
        actor_set_animation(actor, POINTNCLICK_ANIMATION_CURSOR);
    }

    // Interact with another actor or trigger.
    if (INPUT_IS_BTN_UP(POINTNCLICK_ACT_BUTTON) || confirmed) {
        if (!VM_IS_LOCKED) {
            if (is_hover_actor) {
                if (actor->hit_handler_bank != 0)
                    actor_begin_hit_thread(actor, hit_actor);
                if (hit_actor->hit_handler_bank != 0)
                    actor_begin_hit_thread(hit_actor, actor);
            } else if (is_hover_trigger) {
                script_execute(triggers[hit_trigger].hit_handler_bank, triggers[hit_trigger].hit_handler_address, NULL, 2, EVENT_HANDLER_ENTER, hit_trigger);
            }
            moving = POINTNCLICK_MOVING_MODE_NONE;
        }
    }
#else /* USE_TRIGGER */
    // Check collision with another actor.
    actor_t * hit_actor = actor_in_front_of_actor(actor, 0, FALSE);
    const BOOLEAN is_hover_actor = hit_actor && hit_actor->hit_handler_bank != 0;

    // Set cursor animation.
    if (is_hover_actor) {
        actor_set_animation(actor, POINTNCLICK_ANIMATION_CURSOR_HOVER);
    } else {
        actor_set_animation(actor, POINTNCLICK_ANIMATION_CURSOR);
    }

    // Interact with another actor.
    if (INPUT_IS_BTN_UP(POINTNCLICK_ACT_BUTTON) || confirmed) {
        if (!VM_IS_LOCKED) {
            if (is_hover_actor) {
                if (actor->hit_handler_bank != 0)
                    actor_begin_hit_thread(actor, hit_actor);
                if (hit_actor->hit_handler_bank != 0)
                    actor_begin_hit_thread(hit_actor, actor);
            }
            moving = POINTNCLICK_MOVING_MODE_NONE;
        }
    }
#endif /* USE_TRIGGER */

    // Check whether the camera need to be moved.
    if (moving && actor == actor_following_target)
        return TRUE;

    return FALSE;
}

#endif /* USE_POINTNCLICK */
