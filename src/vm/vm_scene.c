#pragma bank 255

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include <rand.h>
#include <string.h>

#include "vm_actor.h"
#include "vm_device.h"
#include "vm_effects.h"
#include "vm_game.h"
#include "vm_graphics.h"
#include "vm_gui.h"
#include "vm_input.h"
#include "vm_projectile.h"
#include "vm_scene.h"
#include "vm_trigger.h"

BANKREF(VM_SCENE)

#define SCENE_LOAD(MAP_BANK, MAP_ADDRESS, ATTR_BANK, ATTR_ADDRESS, X, Y, W, H, SW, BASE_TILE, FUNC) \
    do { \
        _submap_tile_offset = (BASE_TILE); \
        call_v_bbbbpb_oldcall((X), (Y), (W), (H), (MAP_BANK), (MAP_ADDRESS), (SW), (FUNC)); \
        _submap_tile_offset = 0; \
        if ((device_type & DEVICE_TYPE_CGB) && (ATTR_BANK)) { \
            VBK_REG = VBK_ATTRIBUTES; \
            call_v_bbbbpb_oldcall((X), (Y), (W), (H), (ATTR_BANK), (ATTR_ADDRESS), (SW), (FUNC)); \
            VBK_REG = VBK_TILES; \
        } \
    } while (0)

#define SCENE_LAYER_MAP        0x01 // Must have.
#define SCENE_LAYER_ATTR       0x02
#define SCENE_LAYER_PROP       0x04
#define SCENE_LAYER_ACTOR      0x08
#define SCENE_LAYER_TRIGGER    0x10
#define SCENE_LAYER_DEF        0x20 // Must have.
#define SCENE_LAYER_ALL        (SCENE_LAYER_MAP | SCENE_LAYER_ATTR | SCENE_LAYER_PROP | SCENE_LAYER_ACTOR | SCENE_LAYER_TRIGGER | SCENE_LAYER_DEF)

#define SCENE_CAMERA_SHAKE_X   1
#define SCENE_CAMERA_SHAKE_Y   2

#define SCENE_DEF_SIZE         9

#define SCENE_HANDLER_MOVE     1

scene_t scene;
INT16 scene_camera_x;
INT16 scene_camera_y;
UINT8 scene_camera_deadzone_x;
UINT8 scene_camera_deadzone_y;
UINT8 scene_camera_shake_x;
UINT8 scene_camera_shake_y;
static UINT8 scene_map_x;
static UINT8 scene_map_y;
static UINT8 scene_old_map_x;
static UINT8 scene_old_map_y;

void scene_init(void) BANKED {
    memset(&scene, 0, sizeof(scene_t));
    scene_camera_x          = 0;
    scene_camera_y          = 0;
    scene_camera_deadzone_x = 0;
    scene_camera_deadzone_y = 0;
    scene_camera_shake_x    = 0;
    scene_camera_shake_y    = 0;
    scene_map_x             = 0;
    scene_map_y             = 0;
    scene_old_map_x         = 0;
    scene_old_map_y         = 0;
}

void scene_camera(INT16 x, INT16 y) BANKED {
    // Update the state.
    scene_camera_x = x;
    scene_camera_y = y;

    // Scroll the scene if necessary.
    if (FEATURE_SCENE_ENABLED) {
        // Scroll in the x-axis.
        scene_map_x = (UINT8)DIV8(CLAMP(scene_camera_x - graphics_map_x, 0, SCENE_CAMERA_MAX_X));
        if (scene_map_x != scene_old_map_x) { // Scrolled.
            if (scene.width > DEVICE_SCREEN_WIDTH) {
                if (scene_map_x == scene_old_map_x + 1) { // Right.
                    const UINT8 i = scene_map_x;
                    const UINT8 x = i + DEVICE_SCREEN_WIDTH;
                    if (x < scene.width) {
                        UINT8 h = MIN(DEVICE_SCREEN_HEIGHT + 1, scene.height - scene_map_y);
                        SCENE_LOAD(
                            scene.map_bank, scene.map_address,
                            scene.attr_bank, scene.attr_address,
                            x, scene_map_y,       // X, y.
                            1, h,                 // Width, height.
                            scene.width,
                            scene.base_tile,
                            set_bkg_submap
                        );
                        actor_activate_actors_in_col(x, scene_map_y);
                    }
                } else if (scene_map_x == scene_old_map_x - 1) { // Left.
                    const UINT8 i = scene_map_x;
                    const UINT8 h = MIN(DEVICE_SCREEN_HEIGHT + 1, scene.height - scene_map_y);
                    SCENE_LOAD(
                        scene.map_bank, scene.map_address,
                        scene.attr_bank, scene.attr_address,
                        i, scene_map_y,           // X, y.
                        1, h,                     // Width, height.
                        scene.width,
                        scene.base_tile,
                        set_bkg_submap
                    );
                    actor_activate_actors_in_col(i, scene_map_y);
                } else { // Column differs by more than 1.
                    const UINT8 w = MIN(DEVICE_SCREEN_WIDTH + 1, scene.width - scene_map_x);
                    const UINT8 h = MIN(DEVICE_SCREEN_HEIGHT + 1, scene.height - scene_map_y);
                    SCENE_LOAD(
                        scene.map_bank, scene.map_address,
                        scene.attr_bank, scene.attr_address,
                        scene_map_x, scene_map_y, // X, y.
                        w, h,                     // Width, height.
                        scene.width,
                        scene.base_tile,
                        set_bkg_submap
                    );
                    for (UINT8 j = 0; j != h; ++j) {
                        actor_activate_actors_in_row(scene_map_x, scene_map_y + j);
                    }
                }
            }
            scene_old_map_x = scene_map_x;
        }

        // Scroll in the y-axis.
        scene_map_y = (UINT8)DIV8(CLAMP(scene_camera_y - graphics_map_y, 0, SCENE_CAMERA_MAX_Y));
        if (scene_map_y != scene_old_map_y) { // Scrolled.
            if (scene.height > DEVICE_SCREEN_HEIGHT) {
                if (scene_map_y == scene_old_map_y + 1) { // Down.
                    const UINT8 j = scene_map_y;
                    const UINT8 y = j + DEVICE_SCREEN_HEIGHT;
                    if (y < scene.height) {
                        UINT8 w = MIN(DEVICE_SCREEN_WIDTH + 1, scene.width - scene_map_x);
                        SCENE_LOAD(
                            scene.map_bank, scene.map_address,
                            scene.attr_bank, scene.attr_address,
                            scene_map_x, y,       // X, y.
                            w, 1,                 // Width, height.
                            scene.width,
                            scene.base_tile,
                            set_bkg_submap
                        );
                        actor_activate_actors_in_row(scene_map_x, y);
                    }
                } else if (scene_map_y == scene_old_map_y - 1) { // Up.
                    const UINT8 j = scene_map_y;
                    const UINT8 w = MIN(DEVICE_SCREEN_WIDTH + 1, scene.width - scene_map_x);
                    SCENE_LOAD(
                        scene.map_bank, scene.map_address,
                        scene.attr_bank, scene.attr_address,
                        scene_map_x, j,           // X, y.
                        w, 1,                     // Width, height.
                        scene.width,
                        scene.base_tile,
                        set_bkg_submap
                    );
                    actor_activate_actors_in_row(scene_map_x, j);
                } else { // Row differs by more than 1.
                    const UINT8 w = MIN(DEVICE_SCREEN_WIDTH + 1, scene.width - scene_map_x);
                    const UINT8 h = MIN(DEVICE_SCREEN_HEIGHT + 1, scene.height - scene_map_y);
                    SCENE_LOAD(
                        scene.map_bank, scene.map_address,
                        scene.attr_bank, scene.attr_address,
                        scene_map_x, scene_map_y, // X, y.
                        w, h,                     // Width, height.
                        scene.width,
                        scene.base_tile,
                        set_bkg_submap
                    );
                    for (UINT8 j = 0; j != h; ++j) {
                        actor_activate_actors_in_row(scene_map_x, scene_map_y + j);
                    }
                }
            }
            scene_old_map_y = scene_map_y;
        }
    }
}

UINT8 scene_get_attr(UINT8 x, UINT8 y) BANKED {
    if (scene.attr_bank == 0)
        return 0;

    UINT16 off = (UINT16)x + (UINT16)y * scene.width;

    return get_uint8(scene.attr_bank, scene.attr_address + off);
}

UINT8 scene_count_actors(void) BANKED {
    if (scene.actor_bank == 0)
        return 0;

    return get_uint8(scene.actor_bank, scene.actor_address);
}

UINT8 scene_get_actor(
    UINT8 idx,
    UINT8 * tiles_count, UINT8 * tiles_bank, UINT8 ** tiles_ptr,
    UINT8 * sprite_count,
    UINT8 * bank, UINT8 ** ptr,
    UINT16 * x, UINT16 * y,
    UINT8 * behave_bank, UINT8 ** behave_address,
    UINT8 * hits_bank, UINT8 ** hits_address
) BANKED {
    if (scene.actor_bank == 0)
        return 0;

    UINT8 * data  = scene.actor_address +
        1 /* actor count */ + 19 /* bytes per ref */ * idx /* which one */;
    *tiles_count        = get_uint8 (scene.actor_bank, data++);
    *tiles_bank         = get_uint8 (scene.actor_bank, data++);
    *tiles_ptr          = get_ptr   (scene.actor_bank, data  ); data += 2;
    *sprite_count       = get_uint8 (scene.actor_bank, data++);
    *bank               = get_uint8 (scene.actor_bank, data++);
    *ptr                = get_ptr   (scene.actor_bank, data  ); data += 2;
    UINT8 template      = get_uint8 (scene.actor_bank, data++);
    *x                  = get_uint16(scene.actor_bank, data  ); data += 2;
    *y                  = get_uint16(scene.actor_bank, data  ); data += 2;
    *behave_bank        = get_uint8 (scene.actor_bank, data++);
    *behave_address     = get_ptr   (scene.actor_bank, data  ); data += 2;
    *hits_bank          = get_uint8 (scene.actor_bank, data++);
    *hits_address       = get_ptr   (scene.actor_bank, data  ); data += 2;

    return template;
}

UINT8 scene_count_triggers(void) BANKED {
    if (scene.trigger_bank == 0)
        return 0;

    return get_uint8(scene.trigger_bank, scene.trigger_address);
}

void scene_get_trigger(
    UINT8 idx,
    UINT8 * x, UINT8 * y, UINT8 * width, UINT8 * height,
    UINT8 * cb_type, UINT8 * cb_bank, UINT8 ** cb_address
) BANKED {
    if (scene.trigger_bank == 0)
        return;

    UINT8 * data = scene.trigger_address +
        1 /* trigger count */ + 8 /* bytes per ref */ * idx /* which one */;
    *x           = get_uint8(scene.trigger_bank, data++);
    *y           = get_uint8(scene.trigger_bank, data++);
    *width       = get_uint8(scene.trigger_bank, data++);
    *height      = get_uint8(scene.trigger_bank, data++);
    *cb_type     = get_uint8(scene.trigger_bank, data++);
    *cb_bank     = get_uint8(scene.trigger_bank, data++);
    *cb_address  = get_ptr  (scene.trigger_bank, data  ); data += 2;
}

// Shake camera.
UINT8 camera_shake(POINTER THIS, UINT8 start, UINT16 * stack_frame) OLDCALL BANKED { // INVOKABLE.
    if (start) *((SCRIPT_CTX *)THIS)->stack_ptr = sys_time;
    if (((UINT16)sys_time - *((SCRIPT_CTX *)THIS)->stack_ptr) < stack_frame[1]) {
        if (stack_frame[0] & SCENE_CAMERA_SHAKE_X) {
            UINT8 value = rand() & 0x0F;
            if (value > 13) value -= 13;
            else if (value > 7) value -= 7;
            scene_camera_shake_x = value - 3;
        }
        if (stack_frame[0] & SCENE_CAMERA_SHAKE_Y) {
            UINT8 value = rand() & 0x0F;
            if (value > 13) value -= 13;
            else if (value > 7) value -= 7;
            scene_camera_shake_y = value - 3;
        }
        FEATURE_MAP_MOVEMENT_SET;
        ((SCRIPT_CTX *)THIS)->waitable = TRUE;

        return FALSE;
    }
    scene_camera_shake_x = scene_camera_shake_y = 0;
    FEATURE_MAP_MOVEMENT_SET;

    return TRUE;
}

void vm_camera(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const INT16 x = (INT16)*(--THIS->stack_ptr);
    const INT16 y = (INT16)*(--THIS->stack_ptr);
    scene_camera(x, y);
    FEATURE_MAP_MOVEMENT_SET;
}

void vm_viewport(SCRIPT_CTX * THIS, INT16 idxX, INT16 idxY) OLDCALL BANKED {
    INT16 * X, * Y;
    X = VM_REF_TO_PTR(idxX);
    Y = VM_REF_TO_PTR(idxY);
    *X = scene_camera_x - graphics_map_x;
    *Y = scene_camera_y - graphics_map_y;
}

STATIC void scene_def(UINT8 w, UINT8 h, UINT8 base, UINT8 def_bank, UINT8 * def_ptr) {
    const INT16 x               = scene_camera_x;
    const INT16 y               = scene_camera_y;
    scene.width                 = w;
    scene.height                = h;
    scene.base_tile             = base;
    scene_camera_x              = 0;
    scene_camera_y              = 0;
    scene_camera_deadzone_x     = 0;
    scene_camera_deadzone_y     = 0;
    scene_camera_shake_x        = 0;
    scene_camera_shake_y        = 0;
    scene_map_x                 = 0;
    scene_map_y                 = 0;
    scene_old_map_x             = 0;
    scene_old_map_y             = 0;

    if (scene.map_bank) {
        FEATURE_SCENE_ENABLE;
        scene.clamp_camera = TRUE;
        const UINT8 h = MIN(DEVICE_SCREEN_HEIGHT + 1, scene.height);
        SCENE_LOAD(
            scene.map_bank, scene.map_address,
            scene.attr_bank, scene.attr_address,
            scene_map_x, scene_map_y,
            MIN(DEVICE_SCREEN_WIDTH + 1, scene.width), h,
            scene.width,
            scene.base_tile,
            set_bkg_submap
        );
        for (UINT8 j = 0; j != h; ++j) {
            actor_activate_actors_in_row(scene_map_x, scene_map_y + j);
        }
    } else {
        FEATURE_SCENE_DISABLE;
    }

    if (def_bank) {
        scene.is_16x16_grid     = get_uint8 (def_bank, def_ptr++);
        scene.is_16x16_player   = get_uint8 (def_bank, def_ptr++); 
        scene.gravity           = get_uint8 (def_bank, def_ptr++);
        scene.jump_gravity      = get_uint8 (def_bank, def_ptr++);
        scene.jump_max_count    = get_uint8 (def_bank, def_ptr++);
        scene.jump_max_ticks    = get_uint8 (def_bank, def_ptr++);
        scene.climb_velocity    = get_uint8 (def_bank, def_ptr++);
        scene_camera_x          = get_uint16(def_bank, def_ptr  ); def_ptr += 2;
        scene_camera_y          = get_uint16(def_bank, def_ptr  ); def_ptr += 2;
        scene_camera_deadzone_x = get_uint8 (def_bank, def_ptr++);
        scene_camera_deadzone_y = get_uint8 (def_bank, def_ptr++);
    }

    if (x != scene_camera_x || y != scene_camera_y) {
        scene_camera(scene_camera_x, scene_camera_y);
        FEATURE_MAP_MOVEMENT_SET;
    }
}

STATIC void scene_load_actors(UINT8 base_tile) {
    const UINT8 n = scene_count_actors();
    UINT8 forward = 0;
    for (UINT8 i = 0; i != n; ++i) {
        UINT8 tiles_count; // Non-zero for new actor index, otherwise for reused.
        UINT8 tiles_bank;
        UINT8 * tiles_ptr;
        UINT8 sprite_count;
        UINT8 bank;
        UINT8 * ptr;
        UINT16 x, y;
        UINT8 behave_bank;
        UINT8 * behave_address;
        UINT8 hits_bank;
        UINT8 * hits_address;
        const UINT8 template = scene_get_actor(
            i,
            &tiles_count, &tiles_bank, &tiles_ptr,
            &sprite_count,
            &bank, &ptr,
            &x, &y,
            &behave_bank, &behave_address,
            &hits_bank, &hits_address
        );

        if (tiles_count) {
            base_tile += forward;
            forward = tiles_count;
            call_v_bbp_oldcall(base_tile, tiles_count, tiles_bank, tiles_ptr, set_sprite_data);
        }
        actor_t * actor = actor_new();
        actor->template = template;
        actor_def(actor, x, y, base_tile, bank, ptr);

        actor->behave_handler_bank    = behave_bank;
        actor->behave_handler_address = behave_address;
        actor->hit_handler_bank       = hits_bank;
        actor->hit_handler_address    = hits_address;
        if (behave_bank)
            actor_begin_behave_thread(actor, behave_bank, behave_address);
    }
}

STATIC void scene_load_triggers(void) {
    const UINT8 n = scene_count_triggers();
    trigger_dim(n);
    for (UINT8 i = 0; i != n; ++i) {
        UINT8 x;
        UINT8 y;
        UINT8 width;
        UINT8 height;
        UINT8 cb_type;
        UINT8 cb_bank;
        UINT8 * cb_address;
        scene_get_trigger(i, &x, &y, &width, &height, &cb_type, &cb_bank, &cb_address);

        triggers[i].x                   = x;
        triggers[i].y                   = y;
        triggers[i].width               = width;
        triggers[i].height              = height;
        triggers[i].hit_handler_flags   = cb_type;
        triggers[i].hit_handler_bank    = cb_bank;
        triggers[i].hit_handler_address = cb_address;
    }
}

void vm_def_scene(SCRIPT_CTX * THIS, UINT8 src) OLDCALL BANKED {
    const UINT8 w                 = (UINT8)*(--THIS->stack_ptr);
    const UINT8 h                 = (UINT8)*(--THIS->stack_ptr);
    const UINT8 base              = (UINT8)*(--THIS->stack_ptr);
    UINT8 layers                  = SCENE_LAYER_MAP;
    UINT8 step                    = 1;

    scene_init();

    switch (src) {
    case ASSET_SOURCE_READ:
        layers                    = get_uint8(current_data_bank, (UINT8 *)current_data_address++);
        scene.map_bank            = current_data_bank;
        scene.map_address         = (UINT8 *)current_data_address;
        current_data_address     += w * h;
        if (layers & SCENE_LAYER_ATTR) {
            scene.attr_bank       = scene.map_bank;
            scene.attr_address    = scene.map_address + (w * h) * (step++);
            current_data_address += w * h;
        } else {
            scene.attr_bank       = 0;
            scene.attr_address    = NULL;
        }
        if (layers & SCENE_LAYER_PROP) {
            scene.prop_bank       = scene.map_bank;
            scene.prop_address    = scene.map_address + (w * h) * (step++);
            current_data_address += w * h;
        } else {
            scene.prop_bank       = 0;
            scene.prop_address    = NULL;
        }
        {
            scene.actor_bank      = 0;
            scene.actor_address   = NULL;
        }
        {
            scene.trigger_bank    = 0;
            scene.trigger_address = NULL;
        }

        break;
    case ASSET_SOURCE_DATA:
        layers                    = get_uint8(THIS->bank, (UINT8 *)THIS->PC++);
        scene.map_bank            = THIS->bank;
        scene.map_address         = (UINT8 *)THIS->PC;
        THIS->PC                 += w * h;
        if (layers & SCENE_LAYER_ATTR) {
            scene.attr_bank       = scene.map_bank;
            scene.attr_address    = scene.map_address + (w * h) * (step++);
            THIS->PC             += w * h;
        } else {
            scene.attr_bank       = 0;
            scene.attr_address    = NULL;
        }
        if (layers & SCENE_LAYER_PROP) {
            scene.prop_bank       = scene.map_bank;
            scene.prop_address    = scene.map_address + (w * h) * (step++);
            THIS->PC             += w * h;
        } else {
            scene.prop_bank       = 0;
            scene.prop_address    = NULL;
        }
        {
            scene.actor_bank      = 0;
            scene.actor_address   = NULL;
        }
        {
            scene.trigger_bank    = 0;
            scene.trigger_address = NULL;
        }

        break;
    case ASSET_SOURCE_FAR: {
            const UINT8 bank      = get_uint8 (THIS->bank, (UINT8 *)THIS->PC++);
            UINT8 * ptr           = get_ptr_be(THIS->bank, (UINT8 *)THIS->PC  ); THIS->PC += 2;
            layers                = get_uint8 (bank,       ptr++);
            scene.map_bank        = get_uint8 (bank,       ptr++);
            scene.map_address     = get_ptr   (bank,       ptr  );               ptr += 2;
            scene.attr_bank       = get_uint8 (bank,       ptr++);
            scene.attr_address    = get_ptr   (bank,       ptr  );               ptr += 2;
            scene.prop_bank       = get_uint8 (bank,       ptr++);
            scene.prop_address    = get_ptr   (bank,       ptr  );               ptr += 2;
            scene.actor_bank      = 0;
            scene.actor_address   = NULL;
            scene.trigger_bank    = 0;
            scene.trigger_address = NULL;
        }

        break;
    default:
        scene.map_bank            = 0;
        scene.map_address         = NULL;
        scene.attr_bank           = 0;
        scene.attr_address        = NULL;
        scene.prop_bank           = 0;
        scene.prop_address        = NULL;
        scene.actor_bank          = 0;
        scene.actor_address       = NULL;
        scene.trigger_bank        = 0;
        scene.trigger_address     = NULL;

        break;
    }

    scene_def(w, h, base, 0, NULL);
}

void vm_load_scene(SCRIPT_CTX * THIS) OLDCALL BANKED {
    // Prepare.
    const UINT8 map_base_tile    = (UINT8)*(--THIS->stack_ptr);
    const UINT8 sprite_base_tile = (UINT8)*(--THIS->stack_ptr);
    const UINT8 clear_objects    = (UINT8)*(--THIS->stack_ptr);
    const UINT8 tiles_count      = get_uint8 (THIS->bank, (UINT8 *)THIS->PC++);
    const UINT8 tiles_bank       = get_uint8 (THIS->bank, (UINT8 *)THIS->PC++);
    UINT8 * tiles_ptr            = get_ptr_be(THIS->bank, (UINT8 *)THIS->PC  ); THIS->PC += 2;
    const UINT8 is_8x16_sprite   = get_uint8 (THIS->bank, (UINT8 *)THIS->PC++);
    const UINT8 bank             = get_uint8 (THIS->bank, (UINT8 *)THIS->PC++);
    UINT8 * ptr                  = get_ptr_be(THIS->bank, (UINT8 *)THIS->PC  ); THIS->PC += 2;
    const UINT8 w                = get_uint8 (THIS->bank, (UINT8 *)THIS->PC++);
    const UINT8 h                = get_uint8 (THIS->bank, (UINT8 *)THIS->PC++);

    UINT8 layers                 = 0;
    if (bank != 0)
        layers                   = get_uint8 (bank,       ptr++);

    // Reset the map states.
    graphics_map_x = 0;
    graphics_map_y = 0;

    // Initialize the scene.
    scene_init();
    FEATURE_MAP_MOVEMENT_SET;

    // Clear the input states.
    input_button_pressed = 0;
    input_button_previous = 0;
    input_touch_state = 0;

#if defined USE_SGB_MOUSE
    joypads.joy0 = 0;
    input_touch_position_x = DIV2(DEVICE_SCREEN_PX_WIDTH);
    input_touch_position_y = DIV2(DEVICE_SCREEN_PX_HEIGHT);
    input_touch_pressed = 0;
#endif /* USE_SGB_MOUSE */

    game_has_input = 0;

    // Clear the actors, triggers, projectiles and widgets.
    if (clear_objects) {
        actor_init();

        trigger_init();

        projectile_init();

        gui_init();
    }

    // Reset the effects.
    effects_init();

    // Just clear the scene and ignore loading if no layer enabled,
    // or always load with all of the layers with the code below.
    if (layers == 0)
        return;

    // Get the layers.
    scene.map_bank               = get_uint8 (bank,       ptr++);
    scene.map_address            = get_ptr   (bank,       ptr  );               ptr += 2;
    scene.attr_bank              = get_uint8 (bank,       ptr++);
    scene.attr_address           = get_ptr   (bank,       ptr  );               ptr += 2;
    scene.prop_bank              = get_uint8 (bank,       ptr++);
    scene.prop_address           = get_ptr   (bank,       ptr  );               ptr += 2;
    scene.actor_bank             = get_uint8 (bank,       ptr++);
    scene.actor_address          = get_ptr   (bank,       ptr  );               ptr += 2;
    scene.trigger_bank           = get_uint8 (bank,       ptr++);
    scene.trigger_address        = get_ptr   (bank,       ptr  );               ptr += 2;
    const UINT8 def_bank         = get_uint8 (bank,       ptr++);
    UINT8 * def_address          = get_ptr   (bank,       ptr  );               ptr += 2;

    // Fill the tiles.
    SHOW_BKG;
    call_v_bbp_oldcall(map_base_tile, tiles_count, tiles_bank, tiles_ptr, set_bkg_data);
    gui_tile_filled(GRAPHICS_LAYER_MAP, map_base_tile, tiles_count, tiles_bank, tiles_ptr);

    // Setup the scene.
    scene_camera(0, 0);
    FEATURE_MAP_MOVEMENT_SET;

    scene_def(w, h, map_base_tile, def_bank, def_address);

    // Setup the actors.
    SHOW_SPRITES;
    if (is_8x16_sprite) { SPRITES_8x16; } else { SPRITES_8x8; }
    scene_load_actors(sprite_base_tile);

    // Setup the triggers.
    scene_load_triggers();
}

void vm_get_scene_prop(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 x = (UINT8)*(--THIS->stack_ptr);
    const UINT8 y = (UINT8)*(--THIS->stack_ptr);
    const UINT8 p = (UINT8)*(--THIS->stack_ptr);
    switch (p) {
    case PROPERTY_IS_16x16_GRID:
        *(THIS->stack_ptr++) = scene.is_16x16_grid ? TRUE : FALSE;

        break;
    case PROPERTY_IS_16x16_PLAYER:
        *(THIS->stack_ptr++) = scene.is_16x16_player ? TRUE : FALSE;

        break;
    case PROPERTY_CLAMP_CAMERA:
        *(THIS->stack_ptr++) = scene.clamp_camera ? TRUE : FALSE;

        break;
    case PROPERTY_GRAVITY:
        *(THIS->stack_ptr++) = scene.gravity;

        break;
    case PROPERTY_JUMP_GRAVITY:
        *(THIS->stack_ptr++) = scene.jump_gravity;

        break;
    case PROPERTY_JUMP_MAX_COUNT:
        *(THIS->stack_ptr++) = scene.jump_max_count;

        break;
    case PROPERTY_JUMP_MAX_TICKS:
        *(THIS->stack_ptr++) = scene.jump_max_ticks;

        break;
    case PROPERTY_CLIMB_VELOCITY:
        *(THIS->stack_ptr++) = scene.climb_velocity;

        break;
    case PROPERTY_SIZE_WIDTH:
        *(THIS->stack_ptr++) = scene.width;

        break;
    case PROPERTY_SIZE_HEIGHT:
        *(THIS->stack_ptr++) = scene.height;

        break;
    case PROPERTY_CAMERA_DEADZONE_X:
        *(THIS->stack_ptr++) = scene_camera_deadzone_x;

        break;
    case PROPERTY_CAMERA_DEADZONE_Y:
        *(THIS->stack_ptr++) = scene_camera_deadzone_y;

        break;
    case PROPERTY_BLOCKING:
        *(THIS->stack_ptr++) = scene_get_prop(x, y) & SCENE_PROPERTY_BLOCKING_ALL;

        break;
    case PROPERTY_BLOCKING_X:
        *(THIS->stack_ptr++) = scene_get_prop(x, y) & SCENE_PROPERTY_BLOCKING_X;

        break;
    case PROPERTY_BLOCKING_LEFT:
        *(THIS->stack_ptr++) = scene_get_prop(x, y) & SCENE_PROPERTY_BLOCKING_LEFT;

        break;
    case PROPERTY_BLOCKING_RIGHT:
        *(THIS->stack_ptr++) = scene_get_prop(x, y) & SCENE_PROPERTY_BLOCKING_RIGHT;

        break;
    case PROPERTY_BLOCKING_Y:
        *(THIS->stack_ptr++) = scene_get_prop(x, y) & SCENE_PROPERTY_BLOCKING_Y;

        break;
    case PROPERTY_BLOCKING_UP:
        *(THIS->stack_ptr++) = scene_get_prop(x, y) & SCENE_PROPERTY_BLOCKING_UP;

        break;
    case PROPERTY_BLOCKING_DOWN:
        *(THIS->stack_ptr++) = scene_get_prop(x, y) & SCENE_PROPERTY_BLOCKING_DOWN;

        break;
    default:
        *(THIS->stack_ptr++) = FALSE;

#if VM_EXCEPTION_ENABLED
        VM_THROW(EXCEPTION_UNKNOWN_PARAMETER, EXCEPTION_SCENE_ERROR, p);
#endif /* VM_EXCEPTION_ENABLED */

        break;
    }
}

void vm_set_scene_prop(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 x = (UINT8)*(--THIS->stack_ptr);
    const UINT8 y = (UINT8)*(--THIS->stack_ptr);
    const UINT8 p = (UINT8)*(--THIS->stack_ptr);
    (void)x;
    (void)y;
    switch (p) {
    case PROPERTY_IS_16x16_GRID:
        scene.is_16x16_grid = *(--THIS->stack_ptr) ? TRUE : FALSE;

        break;
    case PROPERTY_IS_16x16_PLAYER:
        scene.is_16x16_player = *(--THIS->stack_ptr) ? TRUE : FALSE;

        break;
    case PROPERTY_CLAMP_CAMERA:
        scene.clamp_camera = *(--THIS->stack_ptr) ? TRUE : FALSE;

        break;
    case PROPERTY_GRAVITY:
        scene.gravity = (UINT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_JUMP_GRAVITY:
        scene.jump_gravity = (UINT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_JUMP_MAX_COUNT:
        scene.jump_max_count = (UINT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_JUMP_MAX_TICKS:
        scene.jump_max_ticks = (UINT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_CLIMB_VELOCITY:
        scene.climb_velocity = (UINT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_SIZE_WIDTH:
        scene.width = (UINT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_SIZE_HEIGHT:
        scene.height = (UINT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_CAMERA_DEADZONE:
        scene_camera_deadzone_x = scene_camera_deadzone_y = (UINT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_CAMERA_DEADZONE_X:
        scene_camera_deadzone_x = (UINT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_CAMERA_DEADZONE_Y:
        scene_camera_deadzone_y = (UINT8)*(--THIS->stack_ptr);

        break;
    default:
        --THIS->stack_ptr;

#if VM_EXCEPTION_ENABLED
        VM_THROW(EXCEPTION_UNKNOWN_PARAMETER, EXCEPTION_SCENE_ERROR, p);
#endif /* VM_EXCEPTION_ENABLED */

        break;
    }
}
