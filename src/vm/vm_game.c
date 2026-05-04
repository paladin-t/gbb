#pragma bank 255

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "vm_actor.h"
#include "vm_effects.h"
#include "vm_game.h"
#include "vm_gui.h"
#include "vm_input.h"
#include "vm_projectile.h"
#include "vm_scene.h"

BANKREF(VM_GAME)

UINT8 game_has_input;

void game_init(void) BANKED {
    game_has_input = 0;
}

STATIC void game_update_modules(void) {
    // Update the input states.
#if USE_SGB_MOUSE
    if (device_type & DEVICE_TYPE_SGB) { // Use SGB features if available.
        joypad_ex(&joypads);
        mouse_buttons = 0;
        sgb_mouse_input_update();
        input_touch_pressed = mouse_buttons;
        if      (mouse_x_move < 0) input_touch_position_x -= MIN(MAX(DIV2(-mouse_x_move), 1), input_touch_position_x);
        else if (mouse_x_move > 0) input_touch_position_x += MIN(MAX(DIV2( mouse_x_move), 1), (DEVICE_SCREEN_PX_WIDTH - 1u) - input_touch_position_x);
        if      (mouse_y_move < 0) input_touch_position_y -= MIN(MAX(DIV2(-mouse_y_move), 1), input_touch_position_y);
        else if (mouse_y_move > 0) input_touch_position_y += MIN(MAX(DIV2( mouse_y_move), 1), (DEVICE_SCREEN_PX_HEIGHT - 1u) - input_touch_position_y);
    } else { // Fall to use regular features, and or extension features.
        joypads.joy0 = joypad();
        if (device_type & DEVICE_TYPE_GBB) { // Ignore touch handling if extension features are not supported.
            input_touch_position_x = *(UINT8 *)TOUCH_X_REG;
            input_touch_position_y = *(UINT8 *)TOUCH_Y_REG;
            input_touch_pressed    = *(UINT8 *)TOUCH_PRESSED_REG;
        }
    }
#endif /* USE_SGB_MOUSE */

    INPUT_ACCEPT_BTN;
    game_has_input = input_button_previous ^ input_button_pressed; // Reset the input data, and set if some button's state has changed.
    game_has_input |= input_button_pressed; // Some button has been pressed.
    if (device_type & DEVICE_TYPE_WITH_TOUCH_SUPPORT) {
        INPUT_ACCEPT_TOUCH;
        game_has_input |= input_touch_state; // Touched.
    }

    // Update the actors.
    BOOLEAN has_objects = FALSE;
    if (actor_active_head) {
        actor_update();
        has_objects = TRUE;
    }

    // Update the projectiles.
#if USE_PROJECTILE
    if (projectile_active_head) {
        projectile_update();
        has_objects = TRUE;
    }
#endif /* USE_PROJECTILE */

    // Update the actor collision.
    actor_handle_collision();

    // Process objects.
    if (has_objects) {
        // Hide the unused sprites.
        for (UINT8 i = actor_hardware_sprite_count; i != MAX_HARDWARE_SPRITES; ++i) {
            hide_sprite(i);
        }

        // Reset the sprite base.
        actor_hardware_sprite_count = device_object_sprite_base;
    }

    // Update the GUI.
    if (GUI_CATEGORY_TYPE(gui_widget_category) == GUI_WIDGET_TYPE_MENU) {
        gui_menu_update();
    }

    // Update the graphics effects.
#if USE_EFFECTS
    if (effects_pulse_bank) {
        effects_pulse_update();
    } else if (effects_wobble) {
        effects_wobble_update();
    }
#endif /* USE_EFFECTS */

    // Tick the game time.
    ++game_time;
}

INLINE void game_poll_input_for_auto(void) {
    // Prepare.
    if (input_handler_count == 0) // No registered handler.
        return;

    // Initialize callback processing.
    if (input_handler_cursor == INPUT_HANDLER_NONE) {
        if (game_has_input) {
            // Start from the first registered handler.
            input_handler_cursor = 0;
        } else {
            // Nothing to do if there's no input data.
            return;
        }
    } else {
        if (input_handler_cursor >= input_handler_count) {
            // Cursor overflow, reset it.
            input_handler_cursor = INPUT_HANDLER_NONE;

            return;
        }
    }

    // Process all the event types.
    while (input_handler_cursor < input_handler_count) {
        // Check the event status and get a corresponding registered handler.
        input_handler_t * handler = NULL;
        UINT8 is_touch = 0x00;
        const UINT8 opt = input_handlers[input_handler_cursor].options;
        switch (EVENT_HANDLER_TYPE(opt)) {
        case INPUT_HANDLER_BTN_UP:        if (INPUT_IS_BTN_PRESSED(J_UP))                  handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTN_DOWN:      if (INPUT_IS_BTN_PRESSED(J_DOWN))                handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTN_LEFT:      if (INPUT_IS_BTN_PRESSED(J_LEFT))                handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTN_RIGHT:     if (INPUT_IS_BTN_PRESSED(J_RIGHT))               handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTN_A:         if (INPUT_IS_BTN_PRESSED(J_A))                   handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTN_B:         if (INPUT_IS_BTN_PRESSED(J_B))                   handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTN_SELECT:    if (INPUT_IS_BTN_PRESSED(J_SELECT))              handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTN_START:     if (INPUT_IS_BTN_PRESSED(J_START))               handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTN_ANY:       if (input_button_pressed)                        handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTND_UP:       if (INPUT_IS_BTN_DOWN(J_UP))                     handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTND_DOWN:     if (INPUT_IS_BTN_DOWN(J_DOWN))                   handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTND_LEFT:     if (INPUT_IS_BTN_DOWN(J_LEFT))                   handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTND_RIGHT:    if (INPUT_IS_BTN_DOWN(J_RIGHT))                  handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTND_A:        if (INPUT_IS_BTN_DOWN(J_A))                      handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTND_B:        if (INPUT_IS_BTN_DOWN(J_B))                      handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTND_SELECT:   if (INPUT_IS_BTN_DOWN(J_SELECT))                 handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTND_START:    if (INPUT_IS_BTN_DOWN(J_START))                  handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTND_ANY:      if (INPUT_IS_BTN_DOWN(0xFF))                     handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTNU_UP:       if (INPUT_IS_BTN_UP(J_UP))                       handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTNU_DOWN:     if (INPUT_IS_BTN_UP(J_DOWN))                     handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTNU_LEFT:     if (INPUT_IS_BTN_UP(J_LEFT))                     handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTNU_RIGHT:    if (INPUT_IS_BTN_UP(J_RIGHT))                    handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTNU_A:        if (INPUT_IS_BTN_UP(J_A))                        handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTNU_B:        if (INPUT_IS_BTN_UP(J_B))                        handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTNU_SELECT:   if (INPUT_IS_BTN_UP(J_SELECT))                   handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTNU_START:    if (INPUT_IS_BTN_UP(J_START))                    handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTNU_ANY:      if (INPUT_IS_BTN_UP(0xFF))                       handler = &input_handlers[input_handler_cursor];
            if (!(device_type & DEVICE_TYPE_WITH_TOUCH_SUPPORT)) // Ignore touch handling if touch feature is not supported.
                input_handler_cursor = input_handler_count;

            break;
        case INPUT_HANDLER_TOUCH:         is_touch = INPUT_IS_TOUCH_PRESSED; if (is_touch) handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_TOUCHD:        is_touch = INPUT_IS_TOUCH_DOWN;    if (is_touch) handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_TOUCHU:        is_touch = INPUT_IS_TOUCH_UP;      if (is_touch) handler = &input_handlers[input_handler_cursor]; break;
        }

        // Move to the next handler.
        if (++input_handler_cursor >= input_handler_count)
            input_handler_cursor = INPUT_HANDLER_NONE; // Reset the cursor.

        // Call the handler if matches.
        if (handler) {
            if (EVENT_HANDLER_IS_START(opt)) {
                // Check whether the previous callback has finished.
                if (handler->thread_id != 0 && !VM_IS_TERMINATED(handler->thread_id)) // Still running.
                    continue; // Continue to the next event.

                // Start the thread.
                SCRIPT_CTX * ctx = script_execute(handler->bank, handler->address, &handler->thread_id, 0);
                if (!ctx)
                    break;

                // Push the arguments if necessary.
                if (is_touch) {
                    *(ctx->stack_ptr++) = is_touch;
                    *(ctx->stack_ptr++) = INPUT_TOUCH_Y;
                    *(ctx->stack_ptr++) = INPUT_TOUCH_X;
                }

                break;
            }
        }
    }
}

INLINE BOOLEAN game_poll_input_for_script(SCRIPT_CTX * THIS) {
    // Prepare.
    BOOLEAN wait = TRUE;

    if (input_handler_count == 0) // No registered handler.
        return wait;

    // Initialize callback processing.
    if (input_handler_cursor == INPUT_HANDLER_NONE) {
        if (game_has_input) {
            // Start from the first registered handler.
            input_handler_cursor = 0;
        } else {
            // Nothing to do if there's no input data.
            return wait;
        }
    } else {
        if (input_handler_cursor >= input_handler_count) {
            // Cursor overflow, reset it.
            input_handler_cursor = INPUT_HANDLER_NONE;

            return wait;
        }
    }

    // Process all the event types.
    while (input_handler_cursor < input_handler_count) {
        // Check the event status and get a corresponding registered handler.
        input_handler_t * handler = NULL;
        UINT8 is_touch = 0x00;
        const UINT8 opt = input_handlers[input_handler_cursor].options;
        switch (EVENT_HANDLER_TYPE(opt)) {
        case INPUT_HANDLER_BTN_UP:        if (INPUT_IS_BTN_PRESSED(J_UP))                  handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTN_DOWN:      if (INPUT_IS_BTN_PRESSED(J_DOWN))                handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTN_LEFT:      if (INPUT_IS_BTN_PRESSED(J_LEFT))                handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTN_RIGHT:     if (INPUT_IS_BTN_PRESSED(J_RIGHT))               handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTN_A:         if (INPUT_IS_BTN_PRESSED(J_A))                   handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTN_B:         if (INPUT_IS_BTN_PRESSED(J_B))                   handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTN_SELECT:    if (INPUT_IS_BTN_PRESSED(J_SELECT))              handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTN_START:     if (INPUT_IS_BTN_PRESSED(J_START))               handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTN_ANY:       if (input_button_pressed)                        handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTND_UP:       if (INPUT_IS_BTN_DOWN(J_UP))                     handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTND_DOWN:     if (INPUT_IS_BTN_DOWN(J_DOWN))                   handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTND_LEFT:     if (INPUT_IS_BTN_DOWN(J_LEFT))                   handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTND_RIGHT:    if (INPUT_IS_BTN_DOWN(J_RIGHT))                  handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTND_A:        if (INPUT_IS_BTN_DOWN(J_A))                      handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTND_B:        if (INPUT_IS_BTN_DOWN(J_B))                      handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTND_SELECT:   if (INPUT_IS_BTN_DOWN(J_SELECT))                 handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTND_START:    if (INPUT_IS_BTN_DOWN(J_START))                  handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTND_ANY:      if (INPUT_IS_BTN_DOWN(0xFF))                     handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTNU_UP:       if (INPUT_IS_BTN_UP(J_UP))                       handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTNU_DOWN:     if (INPUT_IS_BTN_UP(J_DOWN))                     handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTNU_LEFT:     if (INPUT_IS_BTN_UP(J_LEFT))                     handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTNU_RIGHT:    if (INPUT_IS_BTN_UP(J_RIGHT))                    handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTNU_A:        if (INPUT_IS_BTN_UP(J_A))                        handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTNU_B:        if (INPUT_IS_BTN_UP(J_B))                        handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTNU_SELECT:   if (INPUT_IS_BTN_UP(J_SELECT))                   handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTNU_START:    if (INPUT_IS_BTN_UP(J_START))                    handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_BTNU_ANY:      if (INPUT_IS_BTN_UP(0xFF))                       handler = &input_handlers[input_handler_cursor];
            if (!(device_type & DEVICE_TYPE_WITH_TOUCH_SUPPORT)) // Ignore touch handling if touch feature is not supported.
                input_handler_cursor = input_handler_count;

            break;
        case INPUT_HANDLER_TOUCH:         is_touch = INPUT_IS_TOUCH_PRESSED; if (is_touch) handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_TOUCHD:        is_touch = INPUT_IS_TOUCH_DOWN;    if (is_touch) handler = &input_handlers[input_handler_cursor]; break;
        case INPUT_HANDLER_TOUCHU:        is_touch = INPUT_IS_TOUCH_UP;      if (is_touch) handler = &input_handlers[input_handler_cursor]; break;
        }

        // Move to the next handler.
        if (++input_handler_cursor >= input_handler_count)
            input_handler_cursor = INPUT_HANDLER_NONE; // Reset the cursor.

        // Call the handler if matches.
        if (handler) {
            if (EVENT_HANDLER_IS_GOSUB(opt)) {
                // Reserve the program pointer.
                if (input_handler_cursor == INPUT_HANDLER_NONE) { // The cursor has been just reset.
                    // Is the last one in the vector, continue stepping onto the next instruction.
                    *(THIS->stack_ptr++) = (UINT16)THIS->PC;

                    // Do not change `wait`.
                } else {
                    // Backtrack the current instruction to simulate loop.
                    *(THIS->stack_ptr++) = (UINT16)THIS->PC - INSTRUCTION_SIZE;

                    // The update is not finished yet, do not wait.
                    wait = FALSE;
                }
                *(THIS->stack_ptr++) = THIS->bank;

                // Jump to the destination address.
                THIS->PC   = handler->address;
                THIS->bank = handler->bank;

                // Push the arguments if necessary.
                if (is_touch) {
                    *(THIS->stack_ptr++) = is_touch;
                    *(THIS->stack_ptr++) = INPUT_TOUCH_Y;
                    *(THIS->stack_ptr++) = INPUT_TOUCH_X;
                }

                break;
            } else if (EVENT_HANDLER_IS_GOTO(opt)) {
                // Jump to the destination address.
                THIS->PC   = handler->address;
                THIS->bank = handler->bank;

                // Push the arguments if necessary.
                if (is_touch) {
                    *(THIS->stack_ptr++) = is_touch;
                    *(THIS->stack_ptr++) = INPUT_TOUCH_Y;
                    *(THIS->stack_ptr++) = INPUT_TOUCH_X;
                }

                break;
            } else /* if (EVENT_HANDLER_IS_START(opt)) */ {
                // Check whether the previous callback has finished.
                if (handler->thread_id != 0 && !VM_IS_TERMINATED(handler->thread_id)) // Still running.
                    continue; // Continue to the next event.

                // Start the thread.
                SCRIPT_CTX * ctx = script_execute(handler->bank, handler->address, &handler->thread_id, 0);
                if (!ctx)
                    break;

                // Push the arguments if necessary.
                if (is_touch) {
                    *(ctx->stack_ptr++) = is_touch;
                    *(ctx->stack_ptr++) = INPUT_TOUCH_Y;
                    *(ctx->stack_ptr++) = INPUT_TOUCH_X;
                }

                break;
            }
        }
    }

    // Finish.
    return wait;
}

void game_update(void) BANKED {
    game_update_modules();
    if (!VM_IS_LOCKED) {
        game_poll_input_for_auto();
    }
}

void vm_update(SCRIPT_CTX * THIS) OLDCALL BANKED {
    game_update_modules();
    if (!VM_IS_LOCKED) {
        if (game_poll_input_for_script(THIS))
            THIS->waitable = TRUE;
    }
}
