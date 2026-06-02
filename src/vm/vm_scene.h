#ifndef __VM_SCENE_H__
#define __VM_SCENE_H__

#include <stdbool.h>

#include "utils/utils.h"

#include "vm.h"

BANKREF_EXTERN(VM_SCENE)

#define SCENE_PROPERTY_EMPTY_GRID              0x00
#define SCENE_PROPERTY_BLOCKING_ALL            0x0F
#   define SCENE_PROPERTY_BLOCKING_X           0x03
#       define SCENE_PROPERTY_BLOCKING_LEFT    0x01
#       define SCENE_PROPERTY_BLOCKING_RIGHT   0x02
#   define SCENE_PROPERTY_BLOCKING_Y           0x0C
#       define SCENE_PROPERTY_BLOCKING_UP      0x04
#       define SCENE_PROPERTY_BLOCKING_DOWN    0x08
#define SCENE_PROPERTY_LADDER                  0x10

#define SCENE_PLAYER_MAX_VELOCITY_Y            110

#define SCENE_RESET_PLAYER_JUMP_COUNT          do { scene.player_can_jump = MAX(scene.jump_max_count, 1); } while (0)
#define SCENE_CLEAR_PLAYER_JUMP_COUNT          do { scene.player_can_jump = 0; } while (0)
#define SCENE_RESET_PLAYER_JUMP_TICKS          do { scene.player_jump_ticks = scene.jump_max_ticks; } while (0)
#define SCENE_CLEAR_PLAYER_JUMP_TICKS          do { scene.player_jump_ticks = 0; } while (0)
#define SCENE_APPLY_PLAYER_VELOCITY_Y(V)       do { scene.player_velocity_y = MIN((V), SCENE_PLAYER_MAX_VELOCITY_Y); } while (0)
#define SCENE_CLEAR_PLAYER_VELOCITY_Y          do { scene.player_velocity_y = 0; } while (0)

#define SCENE_CAMERA_MAX_X                     (MUL8(scene.width - DEVICE_SCREEN_WIDTH))
#define SCENE_CAMERA_MAX_Y                     (MUL8(scene.height - DEVICE_SCREEN_HEIGHT))

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

typedef struct scene_t {
    // Flags.
    bool is_16x16_grid          : 1; // True for 16x16px aligned, otherwise 8x8px aligned. Compile-time determined.
    bool is_16x16_player        : 1; // True for 16x16px-based auto aligning, otherwise 8x8px-based. Compile-time determined.
    bool clamp_camera           : 1;
    bool player_on_ladder       : 1;
    bool reserved1              : 1;
    bool reserved2              : 1;
    bool reserved3              : 1;
    bool reserved4              : 1;
    // Player properties.
    UINT8 gravity;                   // Downward gravity.
    UINT8 jump_gravity;              // Upward gravity (jump).
    UINT8 jump_max_count;            // Max count the player can jump.
    UINT8 jump_max_ticks;            // Max ticks the player can respond to jump input.
    UINT8 climb_velocity;            // Gravity for climbing.
    UINT8 player_can_jump;           // The player's jump counter.
    UINT8 player_jump_ticks;         // The player's jump ticks.
    INT16 player_velocity_y;         // The player's velocity in the y-axis.
    // Resources.
    UINT8 width;                     // In tiles.
    UINT8 height;                    // In tiles.
    UINT8 base_tile;
    UINT8 map_bank;
    UINT8 * map_address;
    UINT8 attr_bank;
    UINT8 * attr_address;
    UINT8 prop_bank;
    UINT8 * prop_address;
    UINT8 actor_bank;
    UINT8 * actor_address;
    UINT8 trigger_bank;
    UINT8 * trigger_address;
} scene_t;

extern scene_t scene;
extern INT16 scene_camera_x;
extern INT16 scene_camera_y;
extern UINT8 scene_camera_deadzone_x;
extern UINT8 scene_camera_deadzone_y;
extern UINT8 scene_camera_shake_x;
extern UINT8 scene_camera_shake_y;

void scene_init(void) BANKED;

void scene_camera(INT16 x, INT16 y) BANKED;
UINT8 scene_get_attr(UINT8 x, UINT8 y) BANKED;
INLINE UINT8 scene_get_prop(UINT8 x, UINT8 y) {
    if (scene.prop_bank == 0)
        return 0;

    UINT16 off = (UINT16)x + (UINT16)y * scene.width;

    return get_uint8(scene.prop_bank, scene.prop_address + off);
}
UINT8 scene_count_actors(void) BANKED;
UINT8 scene_get_actor(
    UINT8 idx,
    UINT8 * tiles_count, UINT8 * tiles_bank, UINT8 ** tiles_ptr,
    UINT8 * sprite_count,
    UINT8 * bank, UINT8 ** ptr,
    UINT16 * x, UINT16 * y,
    UINT8 * behave_bank, UINT8 ** behave_address,
    UINT8 * hits_bank, UINT8 ** hits_address
) BANKED;
UINT8 scene_count_triggers(void) BANKED;
void scene_get_trigger(
    UINT8 idx,
    UINT8 * x, UINT8 * y, UINT8 * width, UINT8 * height,
    UINT8 * cb_type, UINT8 * cb_bank, UINT8 ** cb_address
) BANKED;

void vm_camera(SCRIPT_CTX * THIS) OLDCALL BANKED;
void vm_viewport(SCRIPT_CTX * THIS, INT16 idxX, INT16 idxY) OLDCALL BANKED;
void vm_def_scene(SCRIPT_CTX * THIS, UINT8 src) OLDCALL BANKED;
void vm_load_scene(SCRIPT_CTX * THIS) OLDCALL BANKED;
void vm_get_scene_prop(SCRIPT_CTX * THIS) OLDCALL BANKED; // Get scene property.
void vm_set_scene_prop(SCRIPT_CTX * THIS) OLDCALL BANKED; // Set scene property.

#endif /* __VM_SCENE_H__ */
